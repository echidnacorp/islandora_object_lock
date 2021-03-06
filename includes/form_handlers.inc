<?php
/**
 * @file
 * Form handlers dealing with our locks.
 */

/**
 * Form validation handler.
 *
 * Check that we have the lock; otherwise, we will be redirected with an error
 * message output.
 */
function islandora_object_lock_check_lock_validation_handler(&$form, &$form_state) {
  form_load_include($form_state, 'inc', 'islandora_object_lock', 'includes/utilities');

  $info = $form_state['islandora_object_lock_info'];
  $object = islandora_object_load($info['pid']);

  $path_suffix = $info['suffix'];
  $allow_acquisition = $info['allow_acquisition'];

  $locked = islandora_object_lock_is_locked($object);
  $owned = islandora_object_lock_user_has_lock($object);

  $manage_page = "islandora/object/$object->id/manage/datastreams";

  if (!$owned) {
    if ($locked) {
      $message = islandora_object_lock_get_locked_message($object);
    }
    elseif ($path_suffix !== NULL && $allow_acquisition) {
      $message = t('Your lock has expired due to inactivity. To re-lock this object, please <a href="@edit_url">try again</a>. Any changes since your last save will be lost.', array(
        '@edit_url' => url("islandora/object/{$object->id}/{$path_suffix}"),
      ));
    }
    else {
      $message = t('You must have a lock on the object in order to perform this operation. To re-lock this object, please <a href="@edit_url">try again</a>.', array(
        '@edit_url' => url("islandora/object/{$object->id}/manage/datastreams/locking/lock", array(
          'query' => array(
            'destination' => $path_suffix !== NULL ? "islandora/object/{$object->id}/$path_suffix" : NULL,
          ),
        )),
      ));
    }
    drupal_set_message(filter_xss($message), 'error');
    drupal_goto($manage_page);
  }
}

/**
 * Form submission handler.
 *
 * Release the lock (assuming we have it).
 */
function islandora_object_lock_release_lock_submit_handler(&$form, &$form_state) {
  module_load_include('inc', 'islandora_object_lock', 'includes/utilities');
  $object = islandora_object_load($form_state['islandora_object_lock_info']['pid']);
  $locked = islandora_object_lock_is_locked($object);
  // It's possible the datastream modified hook may have already removed the
  // lock (if enabled). Don't try to double remove it if that's the case.
  if ($locked) {
    islandora_object_lock_remove_object_lock($form_state['islandora_object_lock_info']['pid']);
  }
}

/**
 * Helper to inject our lock validation and release logic.
 *
 * @param array $form
 *   The form structure being output.
 * @param array $form_state
 *   The form state for the given form.
 * @param AbstractObject $object
 *   The object being modified by the form process.
 * @param string|NULL $dsid
 *   A string indicating which datastream is being modified, or NULL to
 *   indicate that it is the object being modified.
 * @param string $path_suffix
 *   A path suffix relative to 'islandora/object/%islandora_object'
 *   to use when redirecting.
 * @param bool $allow_acquisition
 *   Try to acquire the lock if the current user does not have it; the lock
 *   will have to be manually acquired.
 * @param NULL|string $redirect
 *   The relative URL to redirect to, the manage page otherwise.
 * @param array $parents
 *   A hierarchy of keys into the form, where the #validate and #submit
 *   handlers need to be set.
 */
function islandora_object_lock_acquire_during_alter(&$form, &$form_state, AbstractObject $object, $dsid = NULL, $path_suffix = NULL, $allow_acquisition = FALSE, $redirect = NULL, $parents = array()) {
  form_load_include($form_state, 'inc', 'islandora_object_lock', 'includes/utilities');
  form_load_include($form_state, 'inc', 'islandora_object_lock', 'includes/form_handlers');

  $submitting_element =& drupal_array_get_nested_value($form, $parents);

  $locked = islandora_object_lock_is_locked($object);
  if ($locked) {
    // Object is locked...
    if (islandora_object_lock_user_has_lock($object)) {
      // ... and you have it! Jolly good!
      drupal_set_message(islandora_object_lock_get_locked_message($object), 'status', FALSE);
    }
    else {
      // ... but somebody else has it ...
      $redirect_url = is_null($redirect) ? "islandora/object/$object->id/manage" : $redirect;

      $message = islandora_object_lock_get_locked_message($object);

      drupal_set_message(filter_xss($message), 'error');
      drupal_goto($redirect_url);
    }
  }
  elseif ($allow_acquisition) {
    // Object is not locked, and have been instructed to automatically
    // acquire/release it.
    islandora_object_lock_set_object_lock($object->id);
    $submitting_element['#submit'][] = 'islandora_object_lock_release_lock_submit_handler';
  }
  else {
    if (islandora_object_lock_access_lock($object, 'lock')) {
      drupal_set_message(t('You must <a href="@lock_url">acquire the lock</a> before any modifications can be made with this form.', array(
        '@lock_url' => url("islandora/object/{$object->id}/manage/datastreams/locking/lock", array(
          'query' => array(
            'destination' => isset($path_suffix) ? "islandora/object/{$object->id}/$path_suffix" : NULL,
          ),
        )),
      )), 'warning');
    }
    else {
      drupal_set_message(t('This form requires the object to be locked; however, you do not have permission to lock the object.'), 'error');
    }
  }

  // Dump some info into the $form_state we will later use in submission and
  // validation handlers.
  $form_state['islandora_object_lock_info'] = array(
    'pid' => $object->id,
    'dsid' => $dsid,
    'suffix' => $path_suffix,
    'allow_acquisition' => $allow_acquisition,
  );

  $submitting_element['#validate'][] = 'islandora_object_lock_check_lock_validation_handler';
}
