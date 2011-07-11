<?php
/**
 * @param $aid
 * @return unknown_type
 */
function biblio_get_contributor($aid) {
  static $contributor = array();
  if (!isset($contributor[$aid])) {
    $contributor[$aid] = db_query('SELECT * FROM {biblio_contributor_data} WHERE cid = :cid', array(':cid' => $aid))->fetchObject();
  }

  return $contributor[$aid];
}

function biblio_get_contributor_category($contributors, $category) {
  $authors = array();
  foreach ($contributors as $author) {
    if ($author['auth_category'] == $category) {
      $authors[] = $author;
    }
  }
  return $authors;
}

function biblio_get_contributor_by_name($name) {
  $query = db_select('biblio_contributor_data', 'bcd');
  $query->condition('bcd.name', $name);
  return $query->execute()->fetchObject();
}

function biblio_get_first_contributor($vid) {
  static $contributor = array();
  if (!isset($contributor[$vid])) {
    $query = db_select('biblio_contributor', 'bc');
    $query->join('biblio_contributor_data', 'bcd', 'bc.cid=bcd.cid');
    $query->fields('bcd');
    $query->condition('bc.vid', $vid);
    $query->condition('bc.rank', 0);
    $contributor[$vid] = $query->execute()->fetchObject();
  }

  return $contributor[$vid];
}

/**
 * @param $vid
 * @return unknown_type
 */
function biblio_load_contributors($vid) {
  $vids = (isset($vid) ? array($vid) : array());
  return biblio_load_contributors_multiple($vids);
}

function biblio_load_contributors_multiple($vids = array()) {
  $contributors = array();
  if (empty($vids)) return $contributors;

  $query = db_select('biblio_contributor', 'bc');
  $query->innerJoin('biblio_contributor_data', 'bcd', 'bcd.cid = bc.cid');
  $query->fields('bc');
  $query->fields('bcd');
  $query->orderby('bc.vid');
  $query->orderby('bc.rank');
  if (count($vids) == 1) {
    $query->condition('bc.vid', $vids[0]);
  }
  else {
    $query->condition('bc.vid', $vids, 'IN');
  }
  $query->addTag('node_access');
  $result = $query->execute();

  foreach ($result as $creator) {
    $contributors[$creator->vid][] = (array)$creator;
  }

  return $contributors;
}
/**
 * Add separate author named "et al" to the end of the author array
 *
 * @param $authors - author array to augment
 * @param $type - auth_type
 * @return TRUE if author was added, FALSE if "etal" was already there
 */
function biblio_authors_add_etal(&$authors, $type) {
  $etal = "et al"; $max_rank = 0;
  foreach ($authors as $author) { // et al author should be added only once per type
    if ($author['auth_type'] != $type) continue;
    if ($author['name'] == $etal) {
      return FALSE;
    }
    $max_rank = max($max_rank, $author['rank']);
  }
  $authors[] = biblio_parse_author(array('name' => $etal, 'auth_type' => $type, 'lastname' => $etal, 'rank' => $max_rank + 1));
  return TRUE;
}
/**
 * Parse initial contributor array and augment with additional info
 * @param $contributors initial contributor array
 * @return augmented contributor array
 */
function biblio_parse_contributors($contributors) {
  $result = array();
  if (!count($contributors)) return;
  foreach ($contributors as $cat => $authors) {
    $etal = array();
    foreach ($authors as $author) {
      // remove any form of "et al" from name field, because it confuses biblio_parse_author
      $author_cleaned = preg_replace("/et\.?\s+al\.?/", '', $author['name']);
      if ($author_cleaned != $author['name']) { // if "et al" was present:
        $author['name'] = $author_cleaned;  // store cleaned name
        $etal[$author['auth_type']] = TRUE; // mark it as "to be added" in $etal array
      }
      $author['name'] = trim($author['name']);
      if (strlen($author['name'])) {
        $result[$cat][] = biblio_parse_author($author, $cat);
      }
    }
    // add "et al" authors for all neccessary author types
    foreach ($etal as $type => $dummy) {
      if (isset($result[$cat])) { // add "et al" only if plain authors exists
        biblio_authors_add_etal($result[$cat], $type);
      }
    }
  }
  return $result;
}

function biblio_delete_contributors($node) {
  $count = db_delete('biblio_contributor')
            ->condition('nid', $node->nid)
            ->execute();
  biblio_delete_orphan_authors();
  return $count;
}

function biblio_delete_contributors_revision($node) {
  $count = db_delete('biblio_contributor')
            ->condition('vid', $node->vid)
            ->execute();
  biblio_delete_orphan_authors();
  return $count;
}

function biblio_delete_contributor($cid) {
    db_delete('biblio_contributor')
      ->condition('cid', $cid)
      ->execute();

  return db_delete('biblio_contributor_data')
            ->condition('cid', $cid)
            ->execute();

}
function biblio_delete_contributor_revision($cid, $vid) {
  return db_delete('biblio_contributor')
          ->condition('cid', $cid)
          ->condition('vid', $vid)
          ->execute();

}

function biblio_count_orphan_authors() {
  return db_query('SELECT COUNT(*) FROM {biblio_contributor_data} bcd WHERE bcd.cid NOT IN (SELECT DISTINCT(bc.cid) FROM {biblio_contributor} bc )')->fetchField();
}

function biblio_get_orphan_authors() {
  $authors = array();
  $result =  db_query('SELECT bcd.* FROM {biblio_contributor_data} bcd WHERE bcd.cid NOT IN (SELECT DISTINCT(bc.cid) FROM {biblio_contributor} bc )');
  foreach ($result as $author) {
    $authors[] = $author;
  }
  return $authors;
}

function biblio_delete_orphan_authors($force = FALSE) {
  if (variable_get('biblio_auto_orphaned_author_delete', 0) || $force) {
//    db_query('DELETE FROM {biblio_contributor_data} WHERE cid NOT IN (SELECT DISTINCT(cid) FROM {biblio_contributor})');
    $sub_select = db_select('biblio_contributor', 'bc')->fields('bc', array('cid'))->distinct();
    db_delete('biblio_contributor_data')
      ->condition('cid', $sub_select, 'NOT IN')
      ->execute();
  }
}

function biblio_insert_contributors($node) {
  if (!empty($node->biblio_contributors)) {
    return _save_contributors($node->biblio_contributors, $node->nid, $node->vid);
  }
}

function biblio_update_contributors($node) {
  if (!empty($node->biblio_contributors)) {
    _save_contributors($node->biblio_contributors, $node->nid, $node->vid, TRUE);
  }
  biblio_delete_orphan_authors();
  return;

}

function biblio_save_contributor(&$author) {
  return drupal_write_record('biblio_contributor_data', $author);
}

function biblio_update_contributor(&$author) {
  if (!isset($author['cid'])) return FALSE;
  return drupal_write_record('biblio_contributor_data', $author, 'cid');
}

/**
 * Save contributors to the database
 * @param $authors
 * @param $nid
 * @param $vid
 * @param $update
 * @return success of database operations
 */
function _save_contributors(&$contributors, $nid, $vid, $update = FALSE) {
  $md5 = _loadMD5();
  $rank = 0;
  //  db_query('DELETE FROM {biblio_contributor} WHERE nid = :nid AND vid = :vid', array(':nid' => $nid, ':vid' => $vid));
  db_delete('biblio_contributor')
  ->condition(db_and()->condition('nid', $nid)->condition('vid', $vid))
  ->execute();
  foreach ($contributors as  $key => $author) {
 //   foreach ($authors as $key => $author) {
      if (!empty($author['name'])) {
        if (empty($author['lastname'])) {
          $contributors[$key] = $author = biblio_parse_author($author);
        }
        if ($update && !empty($author['cid']))  $author['cid'] = NULL; // null out the cid so we don't do a global change
        if (empty ($author['cid']) && isset($author['md5']) && !empty ($md5)) {
          $author['cid'] = array_search($author['md5'], $md5);
        }
        if (empty ($author['cid'])) {
          biblio_save_contributor($author);
          if (empty ($author['cid'])) return FALSE;
        }

        $link_array = array(
        'nid' => $nid,
        'vid' => $vid,
        'cid' => $author['cid'],
        'rank' => $rank++, //((isset($author['rank']) && is_numeric($author['rank'])) ? $author['rank'] : $key),
        'auth_type' => !empty($author['auth_type']) ? $author['auth_type'] :  $author['auth_category'],
        'auth_category' =>  $author['auth_category'],
        );

        if (!drupal_write_record('biblio_contributor', $link_array)) return FALSE;
      }
   // }
  }
  //TODO  check if it is necessary to reset aka here...
  //  db_query("UPDATE {biblio_contributor_data} SET aka = cid WHERE aka = 0 OR aka IS NULL");
  //  db_update('biblio_contributor_data')
  //    ->fields(array('aka', )
  return TRUE; // successfully saved all contributors
}
/*
 Released through http://bibliophile.sourceforge.net under the GPL licence.
 Do whatever you like with this -- some credit to the author(s) would be appreciated.

 A collection of PHP classes to manipulate bibtex files.

 If you make improvements, please consider contacting the administrators at bibliophile.sourceforge.net so that your improvements can be added to the release package.

 Mark Grimshaw 2004/2005
 http://bibliophile.sourceforge.net

 28/04/2005 - Mark Grimshaw.
 Efficiency improvements.

 11/02/2006 - Daniel Reidsma.
 Changes to preg_matching to account for Latex characters in names such as {\"{o}}
 */
// For a quick command-line test (php -f PARSECREATORS.php) after installation, uncomment these lines:
/***********************
 $authors = "Mark \~N. Grimshaw and Bush III, G.W. & M. C. H{\\'a}mmer Jr. and von Frankenstein, Ferdinand Cecil, P.H. & Charles Louis Xavier Joseph de la Vallee P{\\\"{o}}ussin";
 $creator = new PARSECREATORS();
 $creatorArray = $creator->parse($authors);
 print_r($creatorArray);
 ***********************/
/* Create writer arrays from bibtex input.
 'author field can be (delimiters between authors are 'and' or '&'):
 1. <first-tokens> <von-tokens> <last-tokens>
 2. <von-tokens> <last-tokens>, <first-tokens>
 3. <von-tokens> <last-tokens>, <jr-tokens>, <first-tokens>
 */
/**
 * @param $author_array
 * @return unknown_type
 */
function biblio_parse_author($author_array, $cat = 0) {

  if ($author_array['auth_category'] == 5) {
    $author_array['firstname'] = '';
    $author_array['initials'] = '';
    $author_array['lastname'] = trim($author_array['name']);
    $author_array['prefix'] = '';
  }
  else {
    $value = trim($author_array['name']);
    $appellation = $prefix = $surname = $firstname = $initials = '';
    $prefix = "";
    $value = preg_replace("/\s{2,}/", ' ', $value); // replace multiple white space by single space
    $author = explode(",", $value);
    $size = sizeof($author);
    // No commas therefore something like Mark Grimshaw, Mark Nicholas Grimshaw, M N Grimshaw, Mark N. Grimshaw
    if ($size == 1) {
      // Is complete surname enclosed in {...}, unless the string starts with a backslash (\) because then it is
      // probably a special latex-sign..
      // 2006.02.11 DR: in the last case, any NESTED curly braces should also be taken into account! so second
      // clause rules out things such as author="a{\"{o}}"
      //
      if (preg_match("/(.*) {([^\\\].*)}/", $value, $matches) && !(preg_match("/(.*) {\\\.{.*}.*}/", $value, $matches2))) {
        $author = explode(" ", $matches[1]);
        $surname = $matches[2];
      }
      else {
        $author = explode(" ", $value);
        // last of array is surname (no prefix if entered correctly)
        $surname = array_pop($author);
      }
    }
    // Something like Grimshaw, Mark or Grimshaw, Mark Nicholas  or Grimshaw, M N or Grimshaw, Mark N.
    else
    if ($size == 2) {
      // first of array is surname (perhaps with prefix)
      list ($surname, $prefix) = _grabSurname(array_shift($author));
    }
    // If $size is 3, we're looking at something like Bush, Jr. III, George W
    else {
      // middle of array is 'Jr.', 'IV' etc.
      $appellation = implode(' ', array_splice($author, 1, 1));
      // first of array is surname (perhaps with prefix)
      list ($surname, $prefix) = _grabSurname(array_shift($author));
    }
    $remainder = implode(" ", $author);
    list ($firstname, $initials, $prefix2) = _grabFirstnameInitials($remainder);
    if (!empty ($prefix2))
    $prefix .= $prefix2;
    //var_dump($prefix);
    //$surname = $surname . ' ' . $appellation;
    $author_array['firstname'] = trim($firstname);
    $author_array['initials'] = (strlen(trim($initials)) > 10) ? drupal_substr(trim($initials), 0, 10) : trim($initials);
    $author_array['lastname'] = trim($surname);
    $author_array['prefix'] = trim($prefix);
    $author_array['suffix'] = trim($appellation);
  }
  $author_array['md5'] =  _md5sum($author_array);
  return $author_array;
}
/**
 * @param $creator
 * @return unknown_type
 */
function _md5sum($creator) {
  $string = $creator['firstname'] . $creator['initials'] . $creator['prefix'] . $creator['lastname'];
  $string = str_replace(' ', '', drupal_strtolower($string));
  return md5($string);
}
// grab firstname and initials which may be of form "A.B.C." or "A. B. C. " or " A B C " etc.
/**
 * @param $remainder
 * @return unknown_type
 */
function _grabFirstnameInitials($remainder) {
  $prefix = array();
  $firstname = $initials = '';
  $array = explode(" ", $remainder);
  foreach ($array as $value) {
    $first_char = drupal_substr($value, 0, 1);
    if ((ord($first_char) >= 97) && (ord($first_char) <= 122)) {
    $prefix[] = $value;
    }
    elseif (preg_match("/[a-zA-Z]{2,}/", trim($value))) {
      $firstname_array[] = trim($value);
    }
    else {
      $initials_array[] = trim(str_replace(".", " ", trim($value)));
    }
  }
  if (isset ($initials_array)) {
    $initials = implode(" ", $initials_array);
  }
  if (isset ($firstname_array)) {
    $firstname = implode(" ", $firstname_array);
  }
  if (!empty ($prefix)) {
    $prefix = implode(" ", $prefix);
  }
  return array($firstname, $initials, $prefix);
}
// surname may have title such as 'den', 'von', 'de la' etc. - characterised by first character lowercased.  Any
// uppercased part means lowercased parts following are part of the surname (e.g. Van den Bussche)
/**
 * @param $input
 * @return unknown_type
 */
function _grabSurname($input) {
  $no_prefix = FALSE;
  $surname = FALSE;
  $prefix  = FALSE;

  $surname_array = explode(" ", $input);

  foreach ($surname_array as $value) {
    $first_char = substr($value, 0, 1);
    if (!$no_prefix && (ord($first_char) >= 97) && (ord($first_char) <= 122)) {
      $prefix[] = $value;
    }
    else {
      $surname[] = $value;
      $no_prefix = TRUE;
    }
  }
  if (!empty($surname)) {
    $surname = implode(" ", $surname);
  }
  if (!empty ($prefix)) {
    $prefix = implode(" ", $prefix);
  }
  return array($surname, $prefix);
}
/**
 * @return unknown_type
 */
function _loadMD5() {
  static $md5   = array();
  static $count = 0;
  $db_count = db_query("SELECT COUNT(*) FROM {biblio_contributor_data}")->fetchField();;
  if ($db_count != $count) { //refresh the cached data as some new authors may have been added or removed
    $count = $db_count;
    $md5 = array();
    $result = db_query('SELECT md5,cid  FROM {biblio_contributor_data} ');
    foreach ($result as $row ) {
      $md5[$row->cid] = $row->md5;
    }
  }
  return (count($md5)) ? $md5 : NULL;
}