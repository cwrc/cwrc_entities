<?php

/**
 * @file
 * Custom CWRC drush scripts.
 */

/**
 * Implements hook_drush_command().
 */
function cwrc_entities_drush_command()
{

  $items = array();

  $items['cwrc_entities_test'] = array(
    'description' => 'Test',
    'examples' => array(
      'drush -u 1 cwrc_entities_test'
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX
  );

  return $items;
}


/**
* Test the entities stub building code
*/

function drush_cwrc_entities_test()
{
    drush_print("Extract, lookup, and create stubs for external entities");

    // Include modules.
    module_load_include('inc', 'cwrc_entities', 'includes/external_entities');

    cwrc_entities_traverse_xml('
        <x ref="http://viaf.org/viaf/147166987">
        <x ref="http://www.geonames.org/6106437"></x>
        <x ref="http://viaf.org/viaf/219468490/"></x>
        <x ref="https://www.google.ca/maps/place/Carmel-By-The-Sea,%20CA%2093923,%20USA"></x>
        </x>');


}
