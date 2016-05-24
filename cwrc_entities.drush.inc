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

  $items['cwrc_entities_add_missing_stubs'] = array(
    'description' => 'Add missing stub entities for external URIs',
    'examples' => array(
      'drush -u 1 --uri=http://example.com cwrc_entities_add_missing_stubs'
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX
  );


  $items['cwrc_entities_add_missing_stubs_from_pid'] = array(
    'description' => 'Add missing stub entities for external URIs by PID',
    'examples' => array(
      'drush -u 1 cwrc_entities_add_missing_stubs_from_pid PID'
    ),
    'arguments' => array(
        'pid' => "FedoraCommons Object PID",
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX
  );

 
  $items['cwrc_entities_eap_test'] = array(
    'description' => 'Test the Entity Aggregation Page using a give URI',
    'examples' => array(
      'drush -u 1 --uri=http://example.com cwrc_entities_eap_test URI'
    ),
    'arguments' => array(
        'uri' => "Entity URI",
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

    /*
    cwrc_entities_traverse_xml('
        <x ref="http://viaf.org/viaf/147166987">
        <x ref="http://www.geonames.org/6106437"></x>
        <x ref="http://viaf.org/viaf/29547910"></x>
        <x ref="http://viaf.org/viaf/219468490/"></x>
        <x ref="https://www.google.ca/maps/place/Carmel-By-The-Sea,%20CA%2093923,%20USA"></x>
        </x>');
     */

    cwrc_entities_traverse_xml(
      //'<x ref="http://viaf.org/viaf/174618349"></x>'
      '<x ref="http://www.geonames.org/6148428"></x>'
    );
}


/**
* entities stub building code
*/
function drush_cwrc_entities_add_missing_stubs()
{
    drush_print("Add missing stub entities for external URIs");

    // Include modules.
    module_load_include('inc', 'cwrc_entities', 'includes/external_entities');
    module_load_include('inc', 'islandora_cwrc_basexdb', 'includes/xmldb_endpoints');
    
    $keyValue = array();
    $tmp = islandora_cwrc_exec_xquery(ISLANDORA_EAP_EXTERNAL_MISSING_STUBS, $keyValue);
    $tmp = json_decode($tmp, TRUE);

    foreach ( array_unique($tmp['missingStubs']) as $uri )
    {
      print($uri."\n");
      cwrc_entities_handle_external_by_uri($uri);
    }
}

/**
* entities stub building code
*/
function drush_cwrc_entities_add_missing_stubs_from_pid($pid='')
{
    drush_print("Add missing stub entities within a given PID");

    //$pid = drush_get_option('pid');

    // Include modules.
    module_load_include('inc', 'cwrc_entities', 'includes/external_entities');
    
    print($pid."\n");
    cwrc_entities_handle_external_by_pid($pid);
    print($pid."\n");

}

/**
* Test the Entity Aggregation Page using a give URI 
*/
function drush_cwrc_entities_eap_test($uri='')
{
    drush_print("Build Entity Aggregation Page JSON for a given URI");

    // Include modules.
    module_load_include('inc', 'islandora_cwrc_basexdb', 'includes/xmldb_endpoints');
    
    print($uri."\n");

    $json_results = "";
    if ($uri) {
      $pidKeyValue = array("ENTITY_URI" => "$uri");
      $json_results = islandora_cwrc_exec_xquery(ISLANDORA_EAP_XQUERY_ID, $pidKeyValue);
    }
    print($json_results."\n");

}
