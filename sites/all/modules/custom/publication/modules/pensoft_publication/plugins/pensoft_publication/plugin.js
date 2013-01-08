(function($) {
  var settings = Drupal.settings['pensoft'];
  var $tables = null;
  
  /**
   * Init function
   */
  function init() {
    CKEDITOR.plugins.add("pensoft_publication_insert_table", {
      init : function(editor) {
        editor.ui.addButton("pensoft_publication_insert_table",{
          label : "pensoft_publication_insert_table",
          icon : Drupal.settings.basePath + settings['editor_table_icon'],
          command : "pensoft_publication_insert_table",
        });

        editor.addCommand( 'pensoft_publication_insert_table', {
          exec : function() {
            $tables = $('<div id="pensoft_select_table"><table><tr><th>select</th><th>Name</th><th>Table</th></tr></table></div>').appendTo('body');
            
            // Populate with available tables
            $('#edit-field-publication-tables .form-type-textarea').each(function(i,e) {
              var label = $(this).find('label').html();
              var id = i + 1;
              
              // Ensure all text areas are up to date
              if (typeof CKEDITOR != 'undefined') {
                for (var instanceName in CKEDITOR.instances) {
                  CKEDITOR.instances[instanceName].updateElement();
                }
              }              
              
              var content = $.trim($(this).find('textarea').val());
              if (content != '') {   
                // Create the row
                var $row = $('<tr class="pensoft_select_table_row">' + 
                             '<td><input type="checkbox" name="' + id + '"/></td><td>' + 
                             label + '</td>' + '<td>' + content + '</td></tr>'
                 ).appendTo($tables.children('table'));
                
                // Handle clicks: Bypass clicks on links within the row
                $('a', $row).click(function(e) {
                  e.stopPropagation();
                  $row.click();
                  return false;
                });
                
                // Don't propagate clicks on the checkbox
                $('input', $row).click(function(e) {
                  e.stopPropagation();
                });

                // Handle single inserts
                $row.click(function() {
                  editor.insertHtml('<span>Table. ' + id + '</span>');
                  editor.updateElement();
                  $.colorbox.close();
                });
              }
            });
            
            // Handle multile inserts
            $('<input type="button" class="form-submit" value="' + Drupal.t('Insert') + '"/>').appendTo($tables).mousedown(function() {
              var elements = [];
              $tables.find('tr.pensoft_select_table_row input:checked').each(function() {
                elements.push('<span>Table. ' + $(this).attr('name') + '</span>');
              });
              
              editor.insertHtml(elements.join(''));
              editor.updateElement();
              $.colorbox.close();
            });
            
            // Launch the colorbox
            $.colorbox({
              inline: true,
              iframe: false,
              href: $tables,
              width: settings['editor_table_width'],
              height: settings['editor_table_height'],
              onClosed: function() {
                $tables.remove();
                $tables = null;
              }
            });
          }
        });
      }
    });    
  }
  
  init();
  
})(jQuery);