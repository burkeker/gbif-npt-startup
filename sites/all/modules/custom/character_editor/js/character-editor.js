/**
 * Charcter project JS
 */
 
 (function ($) {
   
   Drupal.CharacterEditor = {

   slickgridInit: function () {
       Drupal.CharacterEditor.initBT();       
    },
    
    slickgridCallback: function (e, data) {      
      
      if(typeof data.response.columns !== 'undefined'){
        Drupal.CharacterEditor.initBT();
        $('.slickgrid-wrapper').addClass('slickgrid-has-tabs');
      }
      
    },
    
    initBT: function(){
      
      var ops = {
        cssClass: "character-editor-header",
        fill: 'rgba(0, 0, 0, .7)',
        cssStyles: {color: 'white', 'font-size': '10px'},
        spikeLength: 8,
        shrinkToFit: true,
        offsetParent: '#slickgrid',
        positions: ['bottom']
      }
    
      $.each(grid.getColumns(), function(i, col) { 
    
        if(col.name != "Classification"){
          $('.' + col.id).bt(col.data.char, ops);
        }
    
      });
      
    },
     
   }   

 Drupal.behaviors.characterEditor = {
   
   attach: function(context, settings) {     
     // Overlay width fix
     if(typeof settings.overlay === 'undefined'){
      $('#slickgrid').parent().width($('#overlay-content').find('#content').width()); 
     }
     
     $('#slickgrid', context).bind('onSlickgridInit', Drupal.CharacterEditor.slickgridInit);
     $('#slickgrid', context).bind('onSlickgridCallback', Drupal.CharacterEditor.slickgridCallback);
     $('#slickgrid', context).bind('onSlickgridTabChanged', Drupal.CharacterEditor.slickgridInit);

   }
   

 }
 
 
 



 })(jQuery);