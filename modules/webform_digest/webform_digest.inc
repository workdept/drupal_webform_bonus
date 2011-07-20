<?php

/**
 * Return periods.
 */
function webform_digest_periods() {
  return array('day' => t('Daily'), 'week' => t('Weekly'), 'month' => t('Monthly'));
}

/**
 * Settings form.
 */
function webform_digest_form(&$form_state, $node) {
  drupal_add_js(drupal_get_path('module', 'webform_digest') . '/webform_digest.js');
  drupal_set_message(t('Cron should be configured to run every hour for correct work of this module.'));
  drupal_set_message(t('Digest is sent to email addresses specified in E-mails configuration tab.'));

  $form = array();

  // Get record from database.
  $record = db_fetch_array(db_query('SELECT * FROM {webform_digest} WHERE nid=%d', array($node->nid)));

  // Define form elements.
  $form['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable digest delivery.'),
    '#default_value' => isset($record['enabled']) ? $record['enabled'] : 0,
    '#description' => t('If checkbox is checked, digest will be sent to email addresses specified in the E-mails settings page of Wefborm.'),
  );
  $form['new_data'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send only new data.'),
    '#default_value' => isset($record['new_data']) ? $record['new_data'] : 0,
    '#description' => t('If checkbox is checked, digest will include only new data since last email.'),
  );
  $periods = webform_digest_periods();
  $default_period = array_shift(array_keys($periods));
  $form['period'] = array(
    '#type' => 'select',
    '#title' => t('Frequency'),
    '#default_value' => isset($record['period']) ? $record['period'] : 0,
    '#options' => $periods,
    '#description' => t('Choose how often digest should be sent.'),
  );
  $form['daily_granularity_month'] = array(
    '#type' => 'select',
    '#title' => t('Day of month'),
    '#default_value' => isset($record['daily_granularity']) ? $record['daily_granularity'] : 1,
    '#options' => array_combine(range(1, 31), range(1, 31)),
    '#description' => t('Choose day of month when digest should be sent.'),
  );
  $form['daily_granularity_week'] = array(
    '#type' => 'select',
    '#title' => t('Day of week'),
    '#default_value' => isset($record['daily_granularity']) ? $record['daily_granularity'] : 0,
    '#options' => array(
      '0' => t('Sunday'),
      '1' => t('Monday'),
      '2' => t('Tuesday'),
      '3' => t('Wednesday'),
      '4' => t('Thursday'),
      '5' => t('Friday'),
      '6' => t('Saturday'),
    ),
    '#description' => t('Choose day of week when digest should be sent.'),
  );
  $form['hourly_granularity'] = array(
    '#type' => 'select',
    '#title' => t('Hour'),
    '#default_value' => isset($record['hourly_granularity']) ? $record['hourly_granularity'] : 0,
    '#options' => range(0, 23),
    '#description' => t('Choose time when digest should be sent.'),
  );
  $form['sent'] = array(
    '#type' => 'value',
    '#value' => $record['sent'],
  );

  // Prepare download form state
  $download_form_state = unserialize($record['settings']);

  // Get download form
  module_load_include('inc', 'webform', 'includes/webform.report');
  $download_form = call_user_func_array('drupal_retrieve_form', array('webform_results_download_form', &$download_form_state, $node));

  // Set default values of fields in download form
  array_walk($download_form, '_webform_digest_set_defualt_value', $download_form_state);

  // Add $node to $form_state.
  $download_form_state['values']['node'] = $node;

  // Embed dowload form here.
  $form = array_merge($form, $download_form);

  $form['submit']['#value'] = t('Save configuration');

  $form['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
  );

  $form['test'] = array(
    '#type' => 'submit',
    '#value' => t('Test delivery'),
  );

  return $form;
}

/**
 * Settings form submit.
 */
function webform_digest_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Save configuration')) {

    // Create record object to save.
    $record = new stdClass();

    // Add nid to record.
    $record->nid = $form_state['values']['node']->nid;

    // Check period and add granularity field to record.
    if (isset($form_state['values']['daily_granularity_' . $form_state['values']['period']])) {
      $record->daily_granularity = $form_state['values']['daily_granularity_' . $form_state['values']['period']];
    }

    // Add other fields to record.
    $fields = array('enabled', 'new_data', 'period', 'sent', 'hourly_granularity');
    foreach ($fields as $field) {
      $record->$field = $form_state['values'][$field];
    }

    // Unset fields from $form_state. We need to have only download form results here.
    $fields = array('enabled', 'new_data', 'period', 'sent', 'hourly_granularity', 'daily_granularity_week', 'daily_granularity_month', 'node', 'reset', 'test');
    foreach ($fields as $field) {
      unset($form_state['values'][$field]);
      unset($form_state['clicked_button']['#post'][$field]);
    }

    // Unset empty components.
    foreach ($form_state['values']['components'] as $key => $value) {
      if (empty($value)) {
        unset($form_state['values']['components'][$key]);
      }
    }

    // Add serialized $form_state of download form to record.
    $record->settings = serialize($form_state);

    // Update record in database.
    db_query('DELETE FROM {webform_digest} WHERE nid=%d', array($record->nid));
    drupal_write_record('webform_digest', $record);
  } elseif ($form_state['values']['op'] == t('Reset')) {
    db_query('DELETE FROM {webform_digest} WHERE nid=%d', array($form_state['values']['node']->nid));
  } elseif ($form_state['values']['op'] == t('Test delivery')) {
    _webform_digest_send_digest($form_state['values']['node']->nid);
  }
}

/**
 * Helper function to set default values of fields in download form.
 */
function _webform_digest_set_defualt_value(&$item, $key, &$form_state) {
  if (is_array($item) && isset($item['#type']) && isset($form_state['values'][$key])) {
    $default_value = $form_state['values'][$key];
    $item['#default_value'] = $default_value;
  } elseif (is_array($item)) {
    array_walk($item, '_webform_digest_set_defualt_value', $form_state);
  }
}


/**
 * Send digest.
 */
function _webform_digest_send_digest($nid) {
  module_load_include('inc', 'webform', 'includes/webform.report');

  // Indicates that webform_digest is currently sending emails.
  // Used in webform_digest_nodeapi and in  webform_results_download to override standard behaviour.
  $GLOBALS['webform_digest_send'] = true;

  // Reset node cache, it should contain some overriden information
  $node = node_load($nid, NULL, TRUE);

  // Get record from database.
  $record = db_fetch_array(db_query('SELECT * FROM {webform_digest} WHERE nid=%d', array($node->nid)));

  // Prepare download form state
  $download_form_state = unserialize($record['settings']);

  // Add $node to $form_state.
  $download_form_state['values']['node'] = $node;

  $exporters = webform_export_fetch_definition();
  $format = $download_form_state['values']['format'];
  $exporter = $exporters[$format]['handler'];

  // Override exporter. Implement ability to filter by mapping component.
  // Mapping component could be selected as email to.
  $extender = "
    class {$exporter}_digest extends $exporter {
      function __construct(\$options) {
        parent::__construct(\$options);
        // Save global variables, which acts as arguments.
        \$this->options = \$options;
        \$this->nid = \$GLOBALS['webform_digest_nid'];
        \$this->map = \$GLOBALS['webform_digest_map'];
        \$this->sent = \$GLOBALS['webform_digest_sent'];
        // Get all the submissions for the node.
        \$this->submissions = webform_get_submissions(\$this->nid);
        // Save index of submissions id
        if (\$sid = array_search('sid', \$this->options['components'])) {
          \$this->sid_index = \$sid-1;
        }
      }
      function add_row(&\$file_handle, \$data) {
        // Check if sid is assigned.
        if (isset(\$this->sid_index)) {
          // Check if row contains data.
          // In case if it is header, following condition returns false.
          if (isset(\$this->submissions[\$data[\$this->sid_index]])) {
            // Include only new data
            if (\$this->sent && \$this->submissions[\$data[\$this->sid_index]]->submitted < \$this->sent) {
              return;
            }
            // Filter with mapped fields
            if (!empty(\$this->map)) {
              // Set return flag to true
              \$return = true;
              // Find matching between mapping and mapped components values.
              // In case if it matches set return flag to false.
              foreach (\$this->map as \$cid => \$values) {
                \$match = array_intersect(
                  \$this->submissions[\$data[\$this->sid_index]]->data[\$cid]['value'],
                  \$values
                );
                if (!empty(\$match)) {
                 \$return = false;
                  break;
                }
              }
              if (\$return) return;
            }
            // Update row counter.
            \$GLOBALS['webform_digest_count']++;
          }
        } else {
          // Update row counter.
          \$GLOBALS['webform_digest_count']++;
        }
        // Call parent function.
        parent::add_row(&\$file_handle, \$data);
      }
    }
  ";
  // Eval overrides.
  if (!class_exists($exporter . '_digest')) {
    eval($extender);
  }
  // Update name of exporter to overriden one.
  $download_form_state['values']['format'] = $format . '_digest';

  // Prepare array of emails to send.
  $custom_emails = array();
  $maps = array();
  foreach ($node->webform['emails'] as $key => $email) {
    if (valid_email_address($email['email'])) {
      // Save custom emails.
      $emails[] = $email['email'];
    } elseif (is_numeric($email['email']) && $node->webform['components'][$email['email']]['type'] == 'mapping') {
      // Prepare map for mapping component, which is selected as email to.
      $items = explode("\n", $node->webform['components'][$email['email']]['extra']['items']);
      foreach ($items as $item) {
        $item_arr = explode('|', $item);
        $address = trim($item_arr[1]);
        $cid =  $node->webform['components'][$email['email']]['extra']['mapped_component'];
        $mapped_value = trim($item_arr[0]);
        $maps[$address][$cid][] = $mapped_value;
      }
    }
  }

  // Set global variables, which acts as arguments to overriden exporter.
  $GLOBALS['webform_digest_nid'] = $node->nid;
  if ($record['new_data'] && $record['sent']) {
    $GLOBALS['webform_digest_sent'] = $record['sent'];
  }
  // Send digest to custom emails. Set global variable that prevents exiting. See webform.patch.
  _webform_digest_send_digest_email($emails, $download_form_state, $node);
  // Send digest to mapped emails.
  foreach ($maps as $email => $map) {
    // Pass Map.
    $GLOBALS['webform_digest_map'] = $map;
    // Reset exported row counter.
    $GLOBALS['webform_digest_count'] = 0;
    // Submit and email digest.
    _webform_digest_send_digest_email(array($email), $download_form_state, $node);
  }

  // Unser global variables.
  unset($GLOBALS['webform_digest_count']);
  unset($GLOBALS['webform_digest_map']);
  unset($GLOBALS['webform_digest_sent']);
  unset($GLOBALS['webform_digest_nid']);
  unset($GLOBALS['webform_digest_send']);

  // Reset node cache which contains some overriden information
  node_load($nid, NULL, TRUE);
}


/**
 * Send digest to specific email. Called by _webform_digest_send_digest().
 */
function _webform_digest_send_digest_email($emails, $download_form_state, $node) {
  // Intercept download form submission, get file.
  $file_content = '';
  if (ob_start ()) {
    // Submit download form.
    drupal_execute('webform_results_download_form', $download_form_state, $node);
    $file_content = ob_get_contents();
    ob_end_clean();
  }

  // Do not send digest if nothing to send
  if (empty($GLOBALS['webform_digest_count'])) {
    return;
  }

  // Parse headers.
  $headers = array();
  $headers_arr_str = split("[\n;]", drupal_get_headers());
  foreach ($headers_arr_str as $header_str) {
    $header = split("[=:]", $header_str);
    $headers[trim($header[0])] = trim($header[1]);
  }

  // Prepare email data.
  $mail_from = variable_get('site_mail', '');
  $subject = 'Webform digest: ' . $node->title;
  $message = 'Webform digest is in attachment.';
  $file = array(
      'filename' => $headers['filename'],
      'filemime' => $headers['content-type'],
      'filecontent' => $file_content
  );
  
  // Send emails
  mimemail($mail_from, $emails, $subject, $message, true, array(), NULL, array($file), 'webform_digest');
}