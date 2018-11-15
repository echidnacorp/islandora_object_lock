/**
 * @file
 * A JavaScript file for the islandora_object_lock module.
 */

(function ($) {

  Drupal.behaviors.islandoraMenuItemLock = {
    attach: function (context, settings) {
      $('.tabs a.lock, .tabs a.unlock').once(function () {

        // Add click handler.
        $(this).click(function (e) {
          if (!$(this).hasClass('disabled')) {
            $(this).addClass('disabled');
          }
        })
      });
    }
  };

})(jQuery);
