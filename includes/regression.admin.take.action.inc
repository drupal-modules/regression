<?php

/**
 * @file
 * Settings file.
 */

/**
 * Generates form to configure module.
 */
function regression_take_action_form() {

  drupal_add_css(drupal_get_path('module', 'regression') . '/styles/regression.css');

  if (arg(0) == 'system' && arg(1) == 'ajax') {
    $ajax                   = TRUE;
    list($ajax_form, $ajax_form_state, $ajax_form_id, $ajax_form_build_id) = ajax_get_form();
    $revision_pid            = arg(5, $ajax_form['#action']);
  }
  else {
    $ajax                   = FALSE;
    $revision_pid            = arg(4);
  }

  $revision                  = regression_revision_load($revision_pid);
  $entry                     = regression_entry_load($revision->entry_id);
  $last_working_revision_id  = $entry->current_working_revision_id;
  $last_working_revision_pid = $entry->current_working_revision_pid;
  $settings                  = regression_get_path_configuration($revision->path, TRUE);
  $is_candidate              = $revision->revision_id == $entry->current_revision_id && $entry->state == REGRESSION_ENTRY_STATE_CANDIDATE;

  $form['regression_take_action']['go_back'] = array(
    '#markup' => '<div class="url">' . l(t('‹‹ Go back to the list'), 'admin/config/regression/status', array('html' => TRUE)) . '</div>',
  );

  $info_header = array(
    'path'                  => t('Menu path'),
    'uri'                   => t('Page path'),
    'last_working_revision' => t('Last working revision'),
    'current_revision'      => t('Revision candidate'),
  );

  if ($last_working_revision_id === NULL) {
    $last_working_revision      = NULL;
    $last_working_revision_name = 'N/A';
  }
  else {
    $last_working_revision      = regression_revision_load($last_working_revision_pid);
    $last_working_revision_name = t('#!num on %date', array(
      '!num'  => $last_working_revision_id,
      '%date' => date('d/m/Y H:i:s', $last_working_revision->updated),
    ));
  }

  $revision_candidate_text = t('#!num on %date', array(
    '!num' => $revision->revision_id,
    '%date' => date('d/m/Y H:i:s', $revision->updated),
  ));

  $info_rows = array(array(
    'path'                  => $revision->path,
    'uri'                   => l($revision->uri, $revision->uri),
    'last_working_revision' => $last_working_revision_name,
    'current_revision'      => $revision_candidate_text,
    ));

  $form['regression_take_action']['info']['table'] = array(
    '#markup' => theme('table', array('header' => $info_header, 'rows' => $info_rows)),
    '#weight' => 0,
  );

  if ($is_candidate) {

    $form['regression_take_action']['#weight'] = 1;
    $form['regression_take_action']['actions'] = array(
      '#title'         => 'Available actions',
      '#type'          => 'radios',
      '#default_value' => 'approve',
      '#description'   => '',
      '#options'       => array(
        'approve'    => t('Approve the change and set the page as default'),
        'ignore'     => t('Mark that change as dynamic (ignore it in the future)'),
        'regression' => t('Mark as regression'),
        'broken'     => t('Mark as broken'),
      ),
    );
  }

  // Description.
  $form['regression_take_action_description'] = array(
    '#weight'  => 4,
    '#title'   => t('Description'),
    '#type'    => 'fieldset',
  );

  // Description Text.
  $form['regression_take_action_description']['decription_text'] = array(
    '#title'         => t('Description'),
    '#type'          => 'textarea',
    '#rows'          => 3,
    '#default_value' => check_plain($entry->description),
  );

  $form['regression_take_action_description']['description_priority'] = array(
    '#title'         => t('Priority'),
    '#type'          => 'radios',
    '#options'       => array(
      '0' => t('Minor'),
      '1' => t('Major'),
      '2' => t('Blocker'),
    ),
    '#default_value' => check_plain($entry->priority),
  );

  $form['regression_take_action_description']['description_module_name'] = array(
    '#title'             => t('Module name'),
    '#type'              => 'textfield',
    '#autocomplete_path' => 'regression/module-autocomplete',
    '#size'              => 40,
    '#default_value'     => check_plain($entry->related_module_name),
  );

  $form['regression_take_action_description']['description_bug_tracker_url'] = array(
    '#title'         => t('Bug tracker URL'),
    '#type'          => 'textfield',
    '#size'          => 40,
    '#default_value' => check_plain($entry->related_bug_tracker_url),
  );

  $form['regression_take_action_description']['assignee'] = array(
    '#title'             => t('Assignee'),
    '#type'              => 'textfield',
    '#autocomplete_path' => 'user/autocomplete',
    '#size'              => 40,
    '#default_value'     => check_plain($entry->assignee),
  );

  if ($is_candidate) {
    if (!isset($settings['require_resolve_description']) || !$settings['require_resolve_description']) {
      $form['regression_take_action_description']['#collapsible'] = TRUE;
      $form['regression_take_action_description']['#collapsed']   = TRUE;
      $form['regression_take_action_description']['#attributes']  = array(
        'class' => array('collapsible'),
      );

    }
    else {
      $form['regression_take_action_description']['decription_text']['#required'] = TRUE;
    }
  }

  $form['regression_ignores'] = array(
    '#weight' => 7,
    '#title'  => t('Dynamic difference viewer'),
    '#type'   => 'fieldset',
    '#prefix' => '<div class="admin">',
    '#suffix' => '</div>',
  );

  $form['regression_ignores']['left'] = array(
    '#prefix' => '<div class="left">',
    '#suffix' => '</div>',
  );

  // Difference between last working version of the page.
  $form['regression_ignores']['left']['regression_take_action_changes'] = array(
    '#title'         => t('Difference between last working version of the page'),
    '#type'          => 'item',
    '#prefix'        => '<div id="changes_diff">',
    '#suffix'        => '</div>',
    '#weight'        => 6,
  );

  if ($ajax) {
    $ignores = array(
      'regex' => array_merge(
        explode("\n", $ajax_form_state['input']['ignores_regexes_path_configuration']),
        explode("\n", $ajax_form_state['input']['ignores_regexes_page_entry'])
      ),
    );
  }

  $num_differences = 0;

  if ($last_working_revision !== $revision) {
    $diff_text = regression_diff_revisions($last_working_revision, $revision, TRUE, $ajax ? $ignores : NULL, 0, $num_differences);
  }

  if ($last_working_revision === $revision || $num_differences == 0) {
    $diff_text = t('No difference detected.');
  }

  $form['regression_ignores']['left']['regression_take_action_changes']['diff'] = array(
    '#type'          => 'item',
    '#markup'        => '<div class="diff">' . nl2br($diff_text) . '</div>',
  );

  $form['regression_ignores']['right'] = array(
    '#prefix' => '<div class="right">',
    '#suffix' => '</div>',
  );

  $form['regression_ignores']['right']['original'] = array(
    '#title'         => 'Working version of the page',
    '#type'          => 'textarea',
    '#rows'          => 10,
    '#default_value' => $last_working_revision ? $last_working_revision->parser_value_plain_text : '',
  );

  $form['regression_ignores']['right']['ignores_regexes_path_configuration'] = array(
    '#title'  => 'List of ignored expressions for pages grouped by menu path',
    '#type'   => 'textarea',
    '#rows'   => 4,
    '#default_value' => $entry->get_path_configuration()->ignores_regex,
    '#ajax' => array(
      'callback' => 'regression_ajax_changes_diff',
      'wrapper'  => 'changes_diff',
      'method'   => 'html',
    ),
  );

  $form['regression_ignores']['right']['ignores_regexes_page_entry'] = array(
    '#title'  => 'List of ignored expressions for page revisions',
    '#type'   => 'textarea',
    '#rows'   => 4,
    '#default_value' => $entry->ignores_regex,
    '#ajax' => array(
      'callback' => 'regression_ajax_changes_diff',
      'wrapper'  => 'changes_diff',
      'method'   => 'html',
    ),
  );

  $form = system_settings_form($form);

  if ($is_candidate) {
    $form['actions']['submit']['#value'] = t('Resolve');
  }
  else {
    $form['actions']['submit']['#value'] = t('Save');
  }

  $form['actions']['#weight'] = 5;
  $form['#submit'] = array('regression_take_action_form_submit');

  if (!$is_candidate) {

    if ($revision->revision_id == $entry->current_revision_id) {

      switch ($entry->state) {
        case REGRESSION_ENTRY_STATE_APPROVED:
          $state_text = t('approved');
          break;

        case REGRESSION_ENTRY_STATE_BROKEN:
          $state_text = t('broken');
          break;

        case REGRESSION_ENTRY_STATE_IGNORED:
          $state_text = t('ignored');
          break;

        case REGRESSION_ENTRY_STATE_REGRESSION:
          $state_text = t('a regression');
          break;
      }

      $form['regression_take_action']['state'] = array(
        '#markup' => '<div class="state">' . t('Page is already marked as <b>!state</b>.', array('!state' => $state_text)) . '</div>',
      );
    }
    else {
      $form['regression_take_action']['state'] = array(
        '#markup' => '<div class="state">'
        . t('There is already a !url.', array(
          '!url' => '<b>' . l(t('new revision candidate #!num for this page', array('!num' => $entry->current_revision_id)), 'admin/config/regression/review/' . $entry->current_revision_pid) . '</b>',
          )) . '</div>',
      );
    }

  }

  return $form;
}

/**
 * Form ajax action. Returns changes widget.
 */
function regression_ajax_changes_diff(&$form, &$form_state) {
  return $form['regression_ignores']['left']['regression_take_action_changes'];
}

/**
 * Validate form submission.
 */
function regression_take_action_form_validate($form, &$form_state) {
}

/**
 * Implements hook_form_submit().
 */
function regression_take_action_form_submit($form, &$form_state) {

  $revision_pid = arg(4);

  $revision = regression_revision_load($revision_pid);
  $entry    = regression_entry_load($revision->entry_id);

  $entry->description             = check_plain($form_state['input']['decription_text']);
  $entry->priority                = check_plain($form_state['input']['description_priority']);
  $entry->related_module_name     = check_plain($form_state['input']['description_module_name']);
  $entry->related_bug_tracker_url = check_plain($form_state['input']['description_bug_tracker_url']);
  $entry->assignee                = check_plain($form_state['input']['assignee']);
  $entry->ignores_regex           = check_plain($form_state['input']['ignores_regexes_page_entry']);

  $path_configuration_entity      = $entry->get_path_configuration();
  $path_configuration_entity->ignores_regex = check_plain($form_state['input']['ignores_regexes_path_configuration']);

  db_update('regression_path_configuration')
    ->condition('pid', $path_configuration_entity->pid)
    ->fields(array(
      'ignores_regex' => $path_configuration_entity->ignores_regex,
    ))
    ->execute();

  switch ($form_state['input']['actions']) {
    case 'approve':
      $entry->state                        = REGRESSION_ENTRY_STATE_APPROVED;
      $entry->current_working_revision_id  = $revision->revision_id;
      $entry->current_working_revision_pid = $revision->pid;
      break;

    case 'ignore':
      $entry->state                        = REGRESSION_ENTRY_STATE_IGNORED;
      $entry->current_working_revision_id  = $revision->revision_id;
      $entry->current_working_revision_pid = $revision->pid;
      break;

    case 'regression':
      $entry->state                       = REGRESSION_ENTRY_STATE_REGRESSION;
      break;

    case 'broken':
      $entry->state                       = REGRESSION_ENTRY_STATE_BROKEN;
      break;
  }

  $entry->save();

}
