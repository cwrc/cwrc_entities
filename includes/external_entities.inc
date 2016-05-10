<?php

/*
* External entity handling
* To allow searching within Islandora to pickup external entities
* stub CWRC entities are created
* This file contains the functions used to create these stub entities
*
* */


/**
* traverse the contents of a given XML string and handle the entity references
*
* @param $xmlStr
*     the XML doc containing the entity references
*
*/
function cwrc_entities_traverse_xml($xmlStr) {

  $domObj = new DOMDocument();
  $domObj->loadXML($xmlStr);

  $xpathObj = new Domxpath($domObj);

  //ToDo: enhance xpath to include all versions
  //$queryResult = $xpathObj->query('//(@ref|@REF)');
  
  //ToDo: only return distinct values to limit the number of 
  // stub already exists queries
  $queryResult = $xpathObj->query('(//@ref | //@REF)');
  foreach($queryResult as $result){
      if (!cwrc_entities_external_exists($result->textContent))
        {
          $tmp = cwrc_entities_external_lookup($result->textContent);
          if ( $tmp ) 
          {
              cwrc_entities_external_create_stub($tmp);
          }
        }
  }
}


/**
* check if a stub has already been added and that it not a "CWRC" entity
*
* @param $refURI
*     URI of the external entity
*/
function cwrc_entities_external_exists($refURI)
{
    // using a chain of responsibility pattern to determine
    // which external datasource handler can work with the
    // given URI
    // ToDo: create this dynamically to make easier to add other 
    // external linked data sources
    $viafHandler = new cwrc_entities_viaf();
    $geoNamesHandler = new cwrc_entities_geonames();
    $googleHandler = new cwrc_entities_google();

    $externalEntityHandlers 
        = array($viafHandler, $geoNamesHandler, $googleHandler);

    $ret = false;;
    foreach ($externalEntityHandlers as $handler)
    {
        $tmp = $handler->is_URI($refURI);
        if ($tmp)
        {
            $ret = cwrc_entities_stub_exists($refURI, $handler->LODSourceReadible);
            break;
        }
    }
    return $ret;
}


/**
* given an external URI lookup the details of the specified entity
*
* @param $refURI
*     URI of the external entity
*/
function cwrc_entities_external_lookup($refURI)
{
    // using a chain of responsibility pattern to determine
    // which external datasource handler can work with the
    // given URI
    // ToDo: create this dynamically to make easier to add other 
    // external linked data sources
    $viafHandler = new cwrc_entities_viaf();
    $geoNamesHandler = new cwrc_entities_geonames();
    $googleHandler = new cwrc_entities_google();

    $externalEntityHandlers 
        = array($viafHandler, $geoNamesHandler, $googleHandler);

    $ret = null;
    foreach ($externalEntityHandlers as $handler)
    {
        $tmp = $handler->is_URI($refURI);
        if ($tmp)
        {
            $ret = $handler->get_external($refURI);
            //print_r($ret);
            break;
        }
    }
    return $ret;
}


/**
* given the result of an external entity detail lookup 
* parse the return and create a new entity stub
*
* @param $typeStruct
*     data structure containing the details of the stub to create 
*/
function cwrc_entities_external_create_stub($typeStruct)
{

  $xml = '';

  switch ($typeStruct['cModel'])
  {
    case 'cwrc:person-entityCModel':
      $xml = cwrc_entities_populateTemplatePerson($typeStruct);
      break;
    case 'cwrc:organization-entityCModel':
      $xml = cwrc_entities_populateTemplateOrganization($typeStruct);
      break;
    case 'cwrc:place-entityCModel':
      $xml = cwrc_entities_populateTemplatePlace($typeStruct);
      break;
    case 'cwrc:citationCModel':
      $xml = cwrc_entities_populateTemplateCitation($typeStruct);
      break;
    default:
      break;
  }

  if ($xml!='')
  {
    print_r($typeStruct['refURI']);
    print_r($xml);
    //cwrc_entities_create_stub($typeStruct, $xml);
  }
  else
  {
    watchdog_exception('cwrc_entities', 'cwrc_entities_external_create_stub', 'XML template empty :'.$typeStruct['cModel'].' '.$typeStruct['refURI'], array(), WATCHDOG_ERROR);
  }
}


/**
 * given a reference URI and LOD Source, determine is a stub entity
 * exists
 *
 * @param refURI
 *    reference URI to search for
 *
 * @param LODSource
 *    the name of the linked open data source
 *
 * @return 
 *    true if the above pair found, false otherwise
 *
 */
function cwrc_entities_stub_exists($refURI, $LODSource)
{
  $ret = false;

  $limit = 1;
  $offset = 0;
  $params = array(
      'fl' => 'PID, fgs_label_s, cwrc_entity_entityId_ms, cwrc_entity_project_id_ms'
      , 'defType' => 'dismax'
      , 'fq' => 'cwrc_entity_entityId_ms:"'.str_replace(':', '\\:', $refURI).'" AND cwrc_entity_project_id_ms:"'.$LODSource.'"'
    );

  $url = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));

  // Use Solr directly rather than the wrapper as it's easier to return JSON,
  // and entities are likely to be filtered out by the default config.
  $solr = new Apache_Solr_Service($url['host'], $url['port'], $url['path'] . '/');
  $solr->setCreateDocuments(0);
  try {
    $results = $solr->search($query, $offset, $limit, $params);
    $results = json_decode($results->getRawResponse(), TRUE);
    // print_r($results);
    if ( $results && $results['response']['numFound'] != 0 )
    {
      $ret = true;
    }
  }
  catch (Exception $exception) {
    watchdog_exception('cwrc_entities', $exception, 'Got an exception while searching.', array(), WATCHDOG_ERROR);
  }
  return $ret;
}




/**
 * an XML template to populated a Person entity
 *
 * @param typeStruct
 *    datastruct containing the necessary details for complete the template
 *
 * @return
 *    completed XML string
 */
function cwrc_entities_populateTemplatePerson($typeStruct)
{
  $today = date('Y-m-d');
  $xml = <<<EOF
<?xml version="1.0" encoding="utf-8"?>
<?xml-model href="http://cwrc.ca/schemas/entities.rng" type="application/xml" schematypens="http://relaxng.org/ns/structure/1.0"?>
<cwrc>
    <entity>
        <person>
            <identity>
                <preferredForm>
                    <namePart>{$typeStruct['label']}</namePart>
                </preferredForm>
                <sameAs>{$typeStruct['refURI']}</sameAs>
            </identity>
            <recordInfo>
                <originInfo>
                    <projectId>{$typeStruct['LODSourceReadible']}</projectId>
                    <recordIdentifier source="viafID">{$typeStruct['refID']}</recordIdentifier>
                    <recordCreationDate>{$today}</recordCreationDate>
                    <recordOrigin>{$typeStruct['recordOrigin']}</recordOrigin>
                </originInfo>
                <entityId>{$typeStruct['refURI']}</entityId>
                <accessCondition type="use and reproduction">Use of this public-domain resource is governed by the <a href="http://creativecommons.org/licenses/by-nc/4.0/" rel="license">Creative Commons Attribution-NonCommercial 4.0 International License</a>.</accessCondition>
            </recordInfo>
        </person>
    </entity>
</cwrc>
EOF;

  return $xml;
}


/**
 * an XML template to populated a Organization entity
 *
 * @param typeStruct
 *    datastruct containing the necessary details for complete the template
 *
 * @return
 *    completed XML string
 */
function cwrc_entities_populateTemplateOrganization($typeStruct)
{
  $today = date('Y-m-d');
  $xml = <<<EOF
<?xml version="1.0" encoding="utf-8"?>
<?xml-model href="http://cwrc.ca/schemas/entities.rng" type="application/xml" schematypens="http://relaxng.org/ns/structure/1.0"?>
<cwrc>
    <entity>
        <organization>
            <identity>
                <preferredForm>
                    <namePart>{$typeStruct['label']}</namePart>
                </preferredForm>
                <sameAs>{$typeStruct['refURI']}</sameAs>
            </identity>
            <recordInfo>
                <originInfo>
                    <projectId>{$typeStruct['LODSourceReadible']}</projectId>
                    <recordIdentifier source="viafID">{$typeStruct['refID']}</recordIdentifier>
                    <recordCreationDate>{$today}</recordCreationDate>
                    <recordOrigin>{$typeStruct['recordOrigin']}</recordOrigin>
                </originInfo>
                <entityId>{$typeStruct['refURI']}</entityId>
                <accessCondition type="use and reproduction">Use of this public-domain resource is governed by the <a href="http://creativecommons.org/licenses/by-nc/4.0/" rel="license">Creative Commons Attribution-NonCommercial 4.0 International License</a>.</accessCondition>
            </recordInfo>
        </organization>
    </entity>
</cwrc>
EOF;
  return $xml;
}


/**
 * an XML template to populated a Place entity
 *
 * @param typeStruct
 *    datastruct containing the necessary details for complete the template
 *
 * @return
 *    completed XML string
 */
function cwrc_entities_populateTemplatePlace($typeStruct)
{
  $today = date('Y-m-d');
  $xml = <<<EOF
<?xml version="1.0" encoding="utf-8"?>
<?xml-model href="http://cwrc.ca/schemas/entities.rng" type="application/xml" schematypens="http://relaxng.org/ns/structure/1.0"?>
<cwrc>
    <entity>
        <place>
            <identity>
                <preferredForm>
                    <namePart>{$typeStruct['label']}</namePart>
                </preferredForm>
                <sameAs>{$typeStruct['refURI']}</sameAs>
            </identity>
            <description>
                <latitude>{$typeStruct['latitude']}</latitude>
                <longitude>{$typeStruct['longitude']}</longitude>
                <countryName>{$typeStruct['countryName']}</countryName>
                <firstAdministrativeDivision>{$typeStruct['state/province']}</firstAdministrativeDivision>
            </description>
            <recordInfo>
                <originInfo>
                    <projectId>{$typeStruct['LODSourceReadible']}</projectId>
                    <recordIdentifier source="{$typeStruct['LODSource']}">{$typeStruct['refID']}</recordIdentifier>
                    <recordCreationDate>{$today}</recordCreationDate>
                    <recordOrigin>{$typeStruct['recordOrigin']}</recordOrigin>
                </originInfo>
                <entityId>{$typeStruct['refURI']}</entityId>
                <accessCondition type="use and reproduction">Use of this public-domain resource is governed by the <a href="http://creativecommons.org/licenses/by-nc/4.0/" rel="license">Creative Commons Attribution-NonCommercial 4.0 International License</a>.</accessCondition>
            </recordInfo>
        </place>
    </entity>
</cwrc>
EOF;

  return $xml;
}

/**
 * an XML template to populated a Title entity
 *
 * @param typeStruct
 *    datastruct containing the necessary details for complete the template
 *
 * @return
 *    completed XML string
 */
function cwrc_entities_populateTemplateCitation($typeStruct)
{
  $today = date('Y-m-d');
  $xml = <<<EOF
<?xml version="1.0" encoding="UTF-8"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.loc.gov/mods/v3 http://www.loc.gov/standards/mods/mods.xsd">
    <titleInfo valueURI="{$typeStruct['refURI']}">
        <title>{$typeStruct['label']}</title>
    </titleInfo>
    <name type="personal">
        <namePart>{$typeStruct['author']}</namePart>
        <role>
            <roleTerm type="text" authority="marcrealtor">Author</roleTerm>
        </role>
    </name>
    <identifier type="viaf">{$typeStruct['refURI']}</identifier>
    <accessCondition type="use and reproduction">Use of this public-domain resource is governed by the <a rel="license" href="http://creativecommons.org/licenses/by-nc/4.0/" target="_blank">Creative Commons Attribution-NonCommercial 4.0 International License</a>.</accessCondition>
    <recordInfo>
        <recordContentSource>{$typeStruct['LODSourceReadible']}</recordContentSource>
        <recordIdentifier source="viafID">{$typeStruct['refID']}</recordIdentifier>
        <recordCreationDate encoding="w3cdtf">$today</recordCreationDate>
        <recordOrigin>{$typeStruct['recordOrigin']}</recordOrigin>
    </recordInfo>
</mods>

EOF;

  return $xml;
}

/**
 * save the stub entity 
 *
 * @param typeStruct
 *    datastruct containing the necessary details for complete the template
 * @param xmlStr
 *    The contents of the CWRC entity created from a template
 *
 * @return
 *    completed XML string
 */
function cwrc_entities_create_stub($typeStruct, $xmlStr)
{

  //ToDo: what should the collection PID be?
  $collection_pid = 'cwrc:asfd';
  //ToDo: add as a configurable parameter?
  //ToDo: is this the proper workflow? 
  $activity_param = array(
     WorkflowConst::STAMP => 'cwrc:cre'
     , WorkflowConst::CATEGORY => 'created'
     , WorkflowConst::STATUS => 'c'
  );
  //ToDo: add as a configurable parameter?
  $entityToDCXSLT = '/xslt/entity_to_dc.xsl';

  $tuque = islandora_get_tuque_connection();
  $fedora_object = $tuque->repository->constructObject('cwrc', TRUE);
  $fedora_object->models = array($typeStruct['cModel']);
  $fedora_object->label = array($typeStruct['label']);

  $fedora_object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $collection_pid);

  // main content datastream
  $datastream = $object->constructDatastream($typeStruct['ds_id'], 'M');
  $datastream->mimetype = 'text/xml';
  $datastream->setContentFromString($xml);
  $fedora_object->ingestDatastream($datastream);

  // workflow record
  if (isset($activity_params)) {
    islandora_markup_entity_add_workflow($fedora_object, $activity_params);
  }

  // DC record
  $dc = islandora_markup_entity_build_DC($fedora_object, $entityToDCXSLT, $xml);
  islandora_markup_entity_update_DC($fedora_object, $dc);

  // ingest
  $object_object = $tuque->repository->ingestObject($fedora_object);

  print_r($object->id);
}



/**
 * template class to represent what a handler for the different
 * external open linked data handlers
 */
class cwrc_entities_external_handler
{
    function is_URI($uriStr) {}
    function get_external($refURI) {}

    /**
     * get a single XPath Textual Item in the event there are multiple
     * returned items for the specified XPath
     */
    function getSingleXPathItem($xpathStr,$xpathObj)
    {
        $queryResult = $xpathObj->query($xpathStr);
        foreach ($queryResult as $node)
        {
            $tmp = $node->textContent;
            if ($tmp)
            {
                break;
            }
        }
        return $tmp;
    }
}


/**
 * viaf external entity handler
 */
class cwrc_entities_viaf extends cwrc_entities_external_handler
{
    var $uriPrefix = 'http://viaf.org/viaf/';
    var $LODSourceReadible = 'VIAF';

    function is_URI($uriStr)
    {
        if (1===preg_match("#$this->uriPrefix#",$uriStr))
            return TRUE;
        else
            return FALSE;
    }

    function get_external($refURI)
    {
        $ret = null;
        $marcCodeSelector = null;

        // test if last character in the URI is a '/' and if not append 
        if (substr($refURI, -1)!='/')
        {
          $refURI = $refURI . '/';
        }
        $qry = $refURI.'viaf.xml';


        // use curl to lookup Linked Open Data source to retrieve and parse
        // xml response
        $ch = curl_init($qry);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        if( ($response = curl_exec($ch) ) === false)
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . ' ' . $qry;
          echo 'Curl error: ' . $errStr;
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else
        {
          $domObj = new DOMDocument();
          $domObj->loadXML($response);

          $xpathObj = new Domxpath($domObj);
          $xpathObj->registerNamespace('ns2',"http://viaf.org/viaf/terms#");

          // grab type from VIAF response
          $queryResult = $xpathObj->query('/ns2:VIAFCluster/ns2:nameType/text()');
          foreach ($queryResult as $node)
          {
              $tmp = $node->textContent;
              if ($tmp)
              {
                  switch($tmp)
                  {
                      case "Personal":
                          $tmp = 'cwrc:person-entityCModel';
                          $ds_id = 'PERSON';
                          $marcCodeSelector = "a";
                          break;
                      case "Corporate":
                          $tmp = 'cwrc:organization-entityCModel';
                          $ds_id = 'ORGANIZATION';
                          $marcCodeSelector = "a";
                          break;
                      case "UniformTitleWork":
                          $tmp = 'cwrc:citationCModel';
                          $ds_id = 'MODS';
                          $marcCodeSelector = "t";
                          break;
                      default:
                          $tmp = 'unknown';
                          break;
                  }
                  break;
              }
          }
          $ret['cModel'] = $tmp;
          $ret['query'] = $qry;

          // grab label from VIAF response
          $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield[@dtype="MARC21"]/ns2:subfield[@code="'.$marcCodeSelector.'"]/text()';
          $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);

          if ( ! $ret['label'])
          {
              //check UNIMarc
              // grab label from VIAF response
              $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield/ns2:subfield[@code="'.$marcCodeSelector.'"]/text()';
              $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);
          }

          if ( $ret['cModel'] === 'cwrc:citationCModel' )
          {
              $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield/ns2:subfield[@code="a"]';
              $ret['author'] = self::getSingleXPathItem($xpathStr, $xpathObj);
          }

          $ret['recordOrigin'] = 'Entity stub derived from an entity record in the Virtual International Authority File (VIAF) database.';

          $ret['refURI'] = $refURI;

          $ret['refID'] = preg_replace("#".$this->uriPrefix."([^/]*)/?#",'${1}', $refURI);

          $ret['LODSource'] = 'viaf';
          $ret['LODSourceReadible'] = $this->LODSourceReadible;
          $ret['ds_id'] = $ds_id;

        }
        return $ret;
    }
}



/**
 * GeoNames external entity handler
 */
class cwrc_entities_geonames extends cwrc_entities_external_handler
{
    var $uriPrefix = 'http://www.geonames.org/';
    var $qryPrefix = 'http://ws.geonames.org/get?username=cwrcgeonames&geonameId=';
    var $LODSourceReadible = 'GeoNames';

    function is_URI($uriStr)
    {
        if (1===preg_match("#$this->uriPrefix#",$uriStr))
            return TRUE;
        else
            return FALSE;
    }

    function get_external($refURI)
    {
        $ret = '';
        $marcCodeSelector = null;

        // get ID from given URI and add to a lookup API call to return XML
        $refId = preg_replace("#".$this->uriPrefix."(.*)/?#",'${1}', $refURI);
        $qry = $this->qryPrefix.$refId;

        // use curl to lookup Linked Open Data source to retrieve and parse
        // xml response
        $ch = curl_init($qry);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        if( ($response = curl_exec($ch) ) === false)
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . ' ' . $qry;
          echo 'Curl error: ' . $errStr;
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else
        {
            $domObj = new DOMDocument();
            $domObj->loadXML($response);

            $xpathObj = new Domxpath($domObj);

            $ret['cModel'] = 'cwrc:place-entityCModel';
            $ret['query'] = $qry;

            // grab label from GeoNames response
            $xpathStr = '/geoname/toponymName[1]/text()';
            $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);

            // grab latitude from GeoNames response
            $xpathStr = '/geoname/lat[1]/text()';
            $ret['latitude'] = self::getSingleXPathItem($xpathStr, $xpathObj);

            // grab longitude from GeoNames response
            $xpathStr = '/geoname/lng[1]/text()';
            $ret['longitude'] = self::getSingleXPathItem($xpathStr, $xpathObj);

            // grab longitude from GeoNames response
            $xpathStr = '/geoname/countryName[1]/text()';
            $ret['countryName'] = self::getSingleXPathItem($xpathStr, $xpathObj);

            // grab longitude from GeoNames response
            $xpathStr = '/geoname/adminName1[1]/text()';
            $ret['state/province'] = self::getSingleXPathItem($xpathStr, $xpathObj);


            $ret['recordOrigin'] = 'Entity stub derived from an entity record in the Geonames database.';

            $ret['refURI'] = $refURI;

            $ret['refID'] = $refID;

            $ret['LODSource'] = 'geonameid';
            $ret['LODSourceReadible'] = $this->LODSourceReadible;
            $ret['ds_id'] = 'PLACE';
        }

        return $ret;
    }
}




/**
 * Google Geocode external entity handler
 */
class cwrc_entities_google extends cwrc_entities_external_handler
{
    var $uriPrefix = 'https://www.google.ca/maps/place/';
    var $qryPrefix = 'http://maps.googleapis.com/maps/api/geocode/xml?sensor=false&address=';
    var $LODSourceReadible = 'Google Maps';

    function is_URI($uriStr)
    {
        if (1===preg_match("#$this->uriPrefix#",$uriStr))
            return TRUE;
        else
            return FALSE;
    }

    function get_external($refURI)
    {
        $ret = null;
        $marcCodeSelector = null;

        $addressStr = preg_replace("#".$this->uriPrefix."(.*)/?#",'${1}', $refURI);
        $qry = $this->qryPrefix.$addressStr;

        // use curl to lookup Linked Open Data source to retrieve and parse
        // xml response
        $ch = curl_init($qry);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        if( ($response = curl_exec($ch) ) === false)
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . ' ' . $qry;
          echo 'Curl error: ' . $errStr;
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else
        {
          $domObj = new DOMDocument();
          $domObj->loadXML($response);

          $xpathObj = new Domxpath($domObj);

          $ret['cModel'] = 'cwrc:place-entityCModel';
          $ret['query'] = $qry; 

          // grab label from response
          $xpathStr = '/GeocodeResponse/result[1]/address_component/long_name[following-sibling::type/text()="locality"]';
          $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);

          // grab latitude from response
          $xpathStr = '/GeocodeResponse/result[1]/geometry[1]/location[1]/lat[1]';
          $ret['latitude'] = self::getSingleXPathItem($xpathStr, $xpathObj);

          // grab longitude from response
          $xpathStr = '/GeocodeResponse/result[1]/geometry[1]/location[1]/lng[1]';
          $ret['longitude'] = self::getSingleXPathItem($xpathStr, $xpathObj);

          // grab longitude from response
          $xpathStr = '/GeocodeResponse/result[1]/address_component/long_name[following-sibling::type/text()="country"]';
          $ret['countryName'] = self::getSingleXPathItem($xpathStr, $xpathObj);

          // grab longitude from response
          $xpathStr = '/GeocodeResponse/result[1]/address_component/long_name[following-sibling::type/text()="administrative_area_level_1"]';
          $ret['state/province'] = self::getSingleXPathItem($xpathStr, $xpathObj);

          $ret['recordOrigin'] = 'Entity stub derived from an entity record in the Google Maps database.';

          $ret['refURI'] = $refURI;

          $ret['refID'] = $addressStr;

          $ret['LODSource'] = 'googlemaps';
          $ret['LODSourceReadible'] = $this->LODSourceReadible;
          $ret['ds_id'] = 'PLACE';
        }

        return $ret;
    }
}





