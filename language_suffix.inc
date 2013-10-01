<?php

/*
 * @file mvc_lang.inc
 * 
 * Implementation functions for language selector
 */

// Divider used between path and language: node/44-eng could be node/44.en
define('MVC_LANG_DIVIDER', '-');
define('MVC_LANG_SUFFIX', ''); //'.php' or '.html'


/*
 * Add prefix to path if it's not there already. use / in admin path
 */

function mvc_lang_path_to_mvc($path, $lang) {
  $languages = language_list('enabled');
  $prefix = $lang->prefix;
  $incoming_path = $path;

  $lookup_path = drupal_lookup_path('alias', $path, $lang->language);
  if ($lookup_path !== FALSE) {
    // Get alias if there is one
    $path = $lookup_path;
  }

  /* TODO: set this character in a variable */
  $divider = MVC_LANG_DIVIDER;
  $divider_len = strlen($divider);

  // Clear the terminal suffix
  $extra_suffix = MVC_LANG_SUFFIX;
  if (substr($path, -1 * strlen($extra_suffix)) == $extra_suffix) {
    $path = substr($path, 0, strlen($path) - strlen($extra_suffix));
  }

  // Clear an existing language suffix.
  foreach ($languages[1] as $language) {
    $prefix_len = -1 * strlen($language->prefix);
    if (substr($path, $prefix_len - $divider_len) == $divider . $language->prefix
        || substr($path, $prefix_len - 1) == '/' . $language->prefix) {
      $path = substr($path, 0, strlen($path) + $prefix_len - $divider_len);
    }
  }

  if (!is_string($path) || empty($path)) {
    return FALSE;
  }
  if (substr($path, -1) == '/') {
    $path = substr($path, 0, -1);
  }

  if (/*substr($path, 0, 10) == 'admin_menu'
      || */ substr($path, 0, 5) == 'media'
      || $path == '<front>'
  ) {
    // Can't have anything on the admin_menu links like for clearing the cache
    return $path;
  }

  // Add the correct language suffix back in
  $prefix_len = -1 * strlen($prefix);
  if (substr($path, $prefix_len - 1) != '/' . $prefix
      && substr($path, $prefix_len - $divider_len) != $divider . $prefix) {
    $path .= $divider . $prefix . $extra_suffix;
  }
  return $path;
}

/*
 * Convert from /path/to/the/current/file-fra format to /path/to/the/current/file/fra
 */

function mvc_lang_path_from_mvc($path, $languages) {
  if (!is_string($path) || empty($path)) {
    return FALSE;
  }
  $temp_path = $path;

  /* TODO: set this character in a variable */
  $divider = MVC_LANG_DIVIDER;
  $divider_len = strlen($divider);

  // Clear the terminal suffix
  $extra_suffix = MVC_LANG_SUFFIX;
  if (substr($path, -1 * strlen($extra_suffix)) == $extra_suffix) {
    $path = substr($path, 0, strlen($path) - strlen($extra_suffix));
  }

  // Convert language to path format
  foreach ($languages as &$lang) {
    // Make autocomplete strings work
    if (preg_match('/^(.*autocomplete.*)-'.$lang->prefix.'(.*)/', $path, $matches) == 1) {
      unset($matches[0]);
      $path = implode('', $matches).'-'.$lang->prefix;
      //return $path;
    }

    $prefix_len = -1 * strlen($lang->prefix);
    if (substr($path, $prefix_len - $divider_len, $divider_len) == $divider
        && substr($path, $prefix_len) == $lang->prefix) {

      $path = substr($path, 0, $prefix_len - $divider_len) . '/' . substr($path, $prefix_len);
      break;
    }
  }
  return $path;
}

/**
 * Figure out the language from the path
 */
function mvc_lang_language_getter($languages) {
  //$qcache = $_GET['q'];
  $parts = explode('/', mvc_lang_path_from_mvc($_GET['q'], $languages));
  $suffix = end($parts);
  foreach ($languages as &$lang) {
    if ($lang->prefix == $suffix) {
      if ($parts[0] == substr($lang->prefix, 0, 2)) {
        $parts[0] = $lang->prefix;
      }
      // remove the suffix
      unset($parts[count($parts) - 1]);

      // Rebuild the q without the language
      $_GET['q'] = join('/', $parts);

      // Backup the current language in the session
      $_SESSION['language'] = $lang->language;

      return $lang->language;
    }
  }
  //$default = language_default();
  //return $default->language;
  return FALSE;
}

/**
 * Rewrite URLs - add prefix as the suffix when generating path aliases
 */
function mvc_lang_url_rewriter(&$path, &$options) {
  global $language;

  // Be sure to grab the url language if present and use it instead of the global language
  if (isset($options['language'])) {
    $lang = $options['language'];
  }
  else {
    $lang = $language;
  }
  // Act only if language has a prefix
  if (!empty($lang->prefix)) {
    // This may not be needed but doesn't seem to be hurting anything
    if (strncmp($path, '<front>', 7) == 0) {
      $path = 'index';
    }
    $path = mvc_lang_path_to_mvc($path, $lang);
  }

  // The secret to this whole function.  Set the alias flag so that url() doesn't
  // update the path with the alias and remove our suffix
  if (isset($options['alias'])) {
    $options['alias'] = TRUE;
  }
}

/**
 * Language switcher ties into Drupal standard switcher
 */
function mvc_lang_language_switcher_url($type, $path) {
  $languages = language_list('enabled');
  $links = array();
  $node_path = FALSE;
  $view_node_path = FALSE;

  $lookup_path = drupal_lookup_path($path);

  if ($lookup_path !== FALSE) {
    $path = $lookup_path;
  }

  unset($matches);
  if (preg_match('/^node\/([0-9]{1,4})/', $path, $matches) == 1) {
    $node_path = TRUE;
    $nid = $matches[1];
  }

  // This is fairly specific to the mbam site.
  // Allows the the language switcher to identify the translation set:
  // /oeuvres-artworks/page/87-fra <-> /oeuvres-artworks/page/540-eng
  unset($matches);
  if (preg_match('/^(.*)\/page\/([0-9]{1,4})/', $path, $matches) == 1) {
    // In this case the path is to a view with a node id as the last element.. 
    // translate that nid if needed
    if (!empty($matches[2])) {
      $view_node_path = TRUE;
      $path_head = $matches[1] . '/page/';
      $nid = $matches[2];
    }
  }

  foreach ($languages[1] as $language) {
    $temp_path = $path;
    if ($node_path) {
      $temp_path = $nid;
      mvc_lang_get_translated_node_id($temp_path, $language->language);
      $temp_path = 'node/' . $temp_path;
    }
    else if ($view_node_path) {
      $temp_path = $nid;
      mvc_lang_get_translated_node_id($temp_path, $language->language);
      $temp_path = $path_head . $temp_path;
    }
    $temp_path = mvc_lang_path_to_mvc($temp_path, $language);

    $query_values = drupal_get_query_parameters();
    
    /*$title = $language->native;
    if (function_exists('mvc_text_lang_filter_quick_process') && !empty($node->title)) {
      $title = mvc_text_lang_filter_quick_process($language->native, TRUE);
    }*/

    $links[$language->language] = array(
      'href' => $temp_path,
      'title' => $language->native,
      'language' => $language,
      'query' => $query_values,
      'attributes' => array(
        'class' => array('language-link'),
        // Add support for WCAG 2.0's Language of Parts to add language identifiers. 'lang' variable above
        // http://www.w3.org/TR/UNDERSTANDING-WCAG20/meaning-other-lang-id.html
        'lang' => $language->language,
        'xml:lang' => $language->language,
      ),
    );
    
  }
  //kpr($links);
  return $links;
}

/*
 * IF $lang is NULL then set $nid to alternate language's nid
 * If $lang is a specific language set $nid to that language' nid
 */
function mvc_lang_get_translated_node_id(&$nid, $lang = NULL) {
  if (empty($nid)) return;
  $temp_nid = $nid;
  $node = node_load($nid);
  if (empty($node)) return $nid;
  $translations = translation_node_get_translations($node->tnid);

  if (is_null($lang)) {
    if ($node->language == 'en') $lang = 'fr';
    else $lang = 'en';
  }
  if (!empty($translations) && isset($translations[$lang])) {
    $nid = $translations[$lang]->nid;
    return $nid;
  }
}