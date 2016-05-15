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

    exec('/urs/bin/drush --root=/var/www/html/drupal7/sites/default/ -u 1 --uri=http://cwrc-dev-05.srv.ualberta.ca cwrc_entities_add_missing_stubs_from_pid');

    /*
    print("zzzz update object: ".$fedora_object->id."\n");

    cwrc_entities_traverse_xml("<x ref='http://viaf.org/viaf/308558564'></x>");
    print("zzzz2 update object: ".$fedora_object->id."\n");
    
    // if the Feodra object has a start of 'deleted' or 'inactive'
    // the delete the object from the XML database
    if ( isset($fedora_object) && $fedora_object->state === 'A' )
    {
      // add content from datastreams
      $tmpDom = null;
      foreach ($fedora_object as $datastream) {
         if  ($datastream->mimetype==='text/xml' or $datastream->mimetype==='application/xml' or $datastream->id==='RELS-EXT' )
         {
            //cwrc_entities_traverse_xml($datastream->content);
         }
      }
    }
     */


  }


}
