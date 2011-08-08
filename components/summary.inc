<?php

/**
 * @file
 *   Webform module summary component.
 */

/**
 * Create a default summary component.
 */
function _webform_defaults_summary() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'extra' => array(
      //'format' => FILTER_FORMAT_DEFAULT,
    ),
  );
}

/**
 * Implementation of _webform_edit_component().
 */
function _webform_edit_summary($currfield) {
  $edit_fields = array();
  return $edit_fields;
}

/**
 * Implementation of _webform_render_component().
 */
function _webform_render_summary($component) {
 $form_item['results'] = array(
    '#type' => 'markup',
    '#weight' => $component['weight'],
    '#title' => '',//$component['name'],
    '#markup' => '',
    '#after_build' => array('_set_summary'),  //Set the value property by calling _set_summary 
  );      
  return $form_item;
}

/**
 * Form after_build callback function. Set the value property of the form item according to the submitted values.
 */
function _set_summary($form_element, &$form_state) {
  global $user;

  module_load_include('inc', 'webform', 'includes/webform.submissions');

  $submitted = $form_state['storage']['submitted'];
  
  $node = $form_state['build_info']['args'][0];
  foreach ($node->webform['components'] as $key => $component) {
    if ($component['type'] == 'pagebreak' || $component['type'] == 'summary') {
      unset($node->webform['components'][$key]);
    }
  }

  $submission = (object) array(
    'nid' => $node->nid,
    'uid' => $user->uid,
    'submitted' => REQUEST_TIME,
    'remote_addr' => ip_address(),
    'is_draft' => TRUE,
    'data' => webform_submission_data($node, $submitted),
  ); 
  $form_element['#markup'] = webform_submission_render($node, $submission, NULL, 'html');
  return $form_element;
}

/**
 * Module specific instance of hook_help().
 */
function _webform_help_summary($section) {
  switch ($section) {
    case 'admin/settings/webform#summary_description':
      return t('Displays previously submitted values in the form; does not render a field. Only values of fields contained in a fieldgroup on the first level are displayed, the name of the fieldgroup is used as caption.');
  }
}