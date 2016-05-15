<?php

/**
* @file 
* class to interact with the php_listeners to build stub entities 
* of external entities for local lookups 
*/

class cwrc_entities_external_listener
{

  function __construct($config)
  {
      $this->config = $config;
  }

  function __destruct() {
  }
    

  /**
  * given a Fedora object , read the object, extract entity URIs,
  * and if URI is external and doesn't already have an
  * local entity stub then create one 
  *
  * @param string $fedora_object
  *  the Fedora object as a Tuque object
  *
  **/
  function cwrc_entities_external_handler($fedora_object, $msgDateTime=null)
  {

    // ugly kludge - otherwise have to remove Drupal functions from the 
    // cwrc_entities include
    exec('/usr/bin/drush --root='.$this->config->drupalPath.' -u 1 cwrc_entities_add_missing_stubs_from_pid '.$fedora_object->id.';');
    
    //exec('/usr/bin/drush --root=/var/www/html/drupal7/ -u 1 cwrc_entities_add_missing_stubs_from_pid '.$fedora_object->id.';');

  }


}
