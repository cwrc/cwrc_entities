<?php

/*
* External entity handling
* To allow searching within Islandora to pickup external entities
* stub CWRC entities are created
* This file contains the functions used to create these stub entities
*
* */

/**
* Given a PID, open the FEdora object, get the contents of the XML datastreams and handle the entity references
*
* @param $pid
*     The PID of the FedoraCommons object 
*
*/
function cwrc_entities_handle_external_by_pid($pid) 
{

    $fedora_object = islandora_object_load($pid);

    // if the Feodra object has a start of 'deleted' or 'inactive'
    // the delete the object from the XML database
    if ( isset($fedora_object) && $fedora_object->state === 'A' )
    { 
        // add content from datastreams
        foreach ($fedora_object as $datastream) {
            if  ($datastream->mimetype==='text/xml' or $datastream->mimetype==='application/xml')
            {
                //print($datastream->content);
                cwrc_entities_traverse_xml($datastream->content);
            }
        }
    } 
    $fedora_object = null;
}



/**
* traverse the contents of a given XML string and handle the entity references
*
* @param $xmlStr
*     the XML doc containing the entity references
*
*/
function cwrc_entities_traverse_xml($xmlStr) 
{
  $domObj = new DOMDocument();
  $domObj->loadXML($xmlStr);

  $xpathObj = new Domxpath($domObj);

  //ToDo: enhance xpath to include all versions
  //$queryResult = $xpathObj->query('//(@ref|@REF)');
  
  //ToDo: only return distinct values to limit the number of 
  // stub already exists queries
  $queryResult = $xpathObj->query('(//@ref | //@REF)');
  $uriArray = array();
  foreach($queryResult as $result){
    $uriArray[] = $result->textContent;
  }
  $uriArray = array_unique($uriArray);
  foreach($uriArray as $uri){
    cwrc_entities_handle_external_by_uri($uri);
  }
}


/**
 * handle an external entity URI by building a stub entity if it doesn't
 * already exist 
*
* @param $refURI
*     external open linked data entity URI 
*
*/
function cwrc_entities_handle_external_by_uri($refURI) 
{
  if (!cwrc_entities_external_exists($refURI)) 
  {
    $tmp = cwrc_entities_external_lookup($refURI);
    if ( $tmp  && $tmp['label'] ) 
    {
      // create only if a label
      cwrc_entities_external_create_stub($tmp);
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
    //print_r($typeStruct['refURI']);
    //print_r($typeStruct);
    //print_r($xml);
    cwrc_entities_create_stub($typeStruct, $xml);
  }
  else
  {
    $errStr = 'cwrc_entities_external_create_stub : XML template empty :'.$typeStruct['cModel'].' '.$typeStruct['refURI'];
    echo 'cwrc_entities_external_create_stub error: ' . $errStr . "\n";
    watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
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
    //print_r($results);
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
  * given a text string, escape XML special characters
  * only to be used for text nodes not attributes
  */
function cwrc_entities_escapeText($str)
{
  return str_replace(
    array('&', '<', '>')
    , array('&amp;', '&lt;', '&gt;')
    , $str
  );
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
  $label = cwrc_entities_escapeText($typeStruct['label']);
  $xml = <<<EOF
<?xml version="1.0" encoding="utf-8"?>
<?xml-model href="http://cwrc.ca/schemas/entities.rng" type="application/xml" schematypens="http://relaxng.org/ns/structure/1.0"?>
<entity>
    <person>
        <identity>
            <preferredForm>
                <namePart>{$label}</namePart>
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
  $label = cwrc_entities_escapeText($typeStruct['label']);
  $xml = <<<EOF
<?xml version="1.0" encoding="utf-8"?>
<?xml-model href="http://cwrc.ca/schemas/entities.rng" type="application/xml" schematypens="http://relaxng.org/ns/structure/1.0"?>
<entity>
    <organization>
        <identity>
            <preferredForm>
                <namePart>{$label}</namePart>
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
  $label = cwrc_entities_escapeText($typeStruct['label']);
  $xml = <<<EOF
<?xml version="1.0" encoding="utf-8"?>
<?xml-model href="http://cwrc.ca/schemas/entities.rng" type="application/xml" schematypens="http://relaxng.org/ns/structure/1.0"?>
<entity>
    <place>
        <identity>
            <preferredForm>
                <namePart>{$label}</namePart>
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
  $label = cwrc_entities_escapeText($typeStruct['label']);
  $author = cwrc_entities_escapeText($typeStruct['author']);
  $xml = <<<EOF
<?xml version="1.0" encoding="UTF-8"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.loc.gov/mods/v3 http://www.loc.gov/standards/mods/mods.xsd">
    <titleInfo valueURI="{$typeStruct['refURI']}">
        <title>{$label}</title>
    </titleInfo>
    <name type="personal">
        <namePart>{$author}</namePart>
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
  module_load_include('inc', 'islandora_workflow_rest', 'includes/cwrc_workflow');
  module_load_include('inc', 'islandora_workflow_rest', 'includes/basic_enum');
  module_load_include('inc', 'islandora_workflow_rest', 'includes/workflow_item');
  module_load_include('inc', 'islandora_workflow_rest', 'includes/utilities');
  module_load_include('inc', 'cwrc_entities', 'includes/callbacks_entities');

  //ToDo: what should the collection PID be?
  $collection_pid = $typeStruct['collectionPID']; 
  //ToDo: add as a configurable parameter?
  //ToDo: is this the proper workflow? 
  $activity_params = array(
     WorkflowConst::STAMP => 'cwrc:cre'
     , WorkflowConst::CATEGORY => 'created'
     , WorkflowConst::STATUS => 'c'
  );
  //ToDo: add as a configurable parameter?
  $entityToDCXSLT = '/xslt/entity_to_dc.xsl';

  $tuque = islandora_get_tuque_connection();
  $fedora_object = $tuque->repository->constructObject('cwrc', TRUE);
  $fedora_object->models = array($typeStruct['cModel']);
  $fedora_object->label = $typeStruct['label'];

  $fedora_object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $collection_pid);


  // main content datastream
  $datastream = $fedora_object->constructDatastream($typeStruct['ds_id'], 'M');
  $datastream->mimetype = 'text/xml';
  $datastream->label= $typeStruct['ds_id'];
  $datastream->setContentFromString($xmlStr);
  $fedora_object->ingestDatastream($datastream);

  // workflow record
  if (isset($activity_params)) {
    islandora_markup_entity_add_workflow($fedora_object, $activity_params);
  }

  // DC record
  $dc = islandora_markup_entity_build_DC($fedora_object->id, $entityToDCXSLT, $xmlStr);
  //print_r($dc);
  //print_r($xmlStr);
  if (isset($dc))
  {
    islandora_markup_entity_update_DC($fedora_object, $dc);
  }

  // ingest
  $fedora_object = $tuque->repository->ingestObject($fedora_object);

  print("adding: " . $fedora_object->id . " : " . $fedora_object->label . " : [\"" . $typeStruct['refURI'] . "\"]\n");
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
      $tmp = '';
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

        // ToDo: how to properly handle cases where the user
        // (a) enters a URI ending with a '/' 
        // or (b) not ending with a '/'
        // 2016-05-11 : this will create 2 separate stubs, one for each case
        // test if last character in the URI is a '/' and if not append 
        if (substr($refURI, -1)!='/')
        {
          $qry = $refURI.'/'.'viaf.xml';
        }
        else
        {
          $qry = $refURI.'viaf.xml';
        }

        // use curl to lookup Linked Open Data source to retrieve and parse
        // xml response
        $ch = curl_init($qry);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true); // viaf redirects
        $response = curl_exec($ch);
        $http_status = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        if ( $response === false)
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . ' ["' . $qry . '"]';
          echo 'Curl error: ' . $errStr . "\n";
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else if ( $http_status != "200" )
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . " [$http_status] " . ' ["' . $qry . '"]';
          echo 'Curl error: ' . $errStr . "\n";
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else
        {
          $domObj = new DOMDocument();
          $domObj->loadXML($response);

          $xpathObj = new Domxpath($domObj);
          $xpathObj->registerNamespace('ns2',"http://viaf.org/viaf/terms#");

          // grab type from VIAF response
          $cModel = '';
          $viafType = '';
          $queryResult = $xpathObj->query('/ns2:VIAFCluster/ns2:nameType/text()');
          foreach ($queryResult as $node)
          {
              $viafType = $node->textContent;
              if ($viafType)
              {
                  switch($viafType)
                  {
                      case "Personal":
                          $cModel = 'cwrc:person-entityCModel';
                          $ds_id = 'PERSON';
                          $collection_pid = 'cwrc:externalEntityStub-Person';
                          $marcCodeSelector = "a";
                          break;
                      case "Corporate":
                          $cModel = 'cwrc:organization-entityCModel';
                          $collection_pid = 'cwrc:externalEntityStub-Organization';
                          $ds_id = 'ORGANIZATION';
                          $marcCodeSelector = "a";
                          break;
                      case "UniformTitleWork":
                          $cModel = 'cwrc:citationCModel';
                          $collection_pid = 'cwrc:externalEntityStub-Title';
                          $ds_id = 'MODS';
                          $marcCodeSelector = "t";
                          break;
                      default:
                          $cModel = '';
                          break;
                  }
                  break;
              }
          }

          $ret['query'] = $qry;
          $ret['LODSource'] = 'viaf';
          $ret['LODSourceReadible'] = $this->LODSourceReadible;
          $ret['refURI'] = $refURI;
          $ret['refID'] = preg_replace("#".$this->uriPrefix."([^/]*)/?#",'${1}', $refURI);
          $ret['cModel'] = $cModel;
          $ret['ds_id'] = $ds_id;
          $ret['collectionPID'] = $collection_pid;

          // grab label from VIAF response
          $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield[@dtype="MARC21"]/ns2:subfield[@code="'.$marcCodeSelector.'"]/text()';
          $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);

          if ( !$ret['label'])
          {
              //check UNIMarc
              // grab label from VIAF response
              $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield/ns2:subfield[@code="'.$marcCodeSelector.'"]/text()';
              $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);
          }

          if ( $ret['cModel'] === 'cwrc:citationCModel' )
          {
              $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield[@dtype="MARC21"]/ns2:subfield[@code="a"]';
              $ret['author'] = self::getSingleXPathItem($xpathStr, $xpathObj);
              if ( !$ret['author'])
              {
                $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield/ns2:subfield[@code="a"]';
              }
          }

          $ret['recordOrigin'] = 'Entity stub derived from an entity record in the Virtual International Authority File (VIAF) database.';

          // check to see if a label was found, if not report
          if ( !$ret['label'] or !$ret['cModel'] )
          {
            $errStr
              = 'linked open data lookup - no error but no label: '
//              . "{\"" . $response . "\"} " 
              . ' ["' . $qry . '"]'
              . ' ["' . $refURI . '"]'
//              . ' ["' . $http_status . '"]'
              . ' ["' . $viafType . '"]'
              ;
            echo 'Curl error: ' . $errStr . "\n";
            watchdog('cwrc_entities', $errStr, array(), WATCHDOG_WARNING);
          }

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
        $refID = preg_replace("#".$this->uriPrefix."([^/]*)/?#",'${1}', $refURI);
        $qry = $this->qryPrefix.$refID;

        // use curl to lookup Linked Open Data source to retrieve and parse
        // xml response
        $ch = curl_init($qry);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true); // viaf redirects
        $response = curl_exec($ch);
        $http_status = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        if ( $response === false)
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . ' ' . $qry;
          echo 'Curl error: ' . $errStr;
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else if ( $http_status != "200" )
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . " [$http_status] " . ' ["' . $qry . '"]';
          echo 'Curl error: ' . $errStr . "\n";
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else
        {
          $domObj = new DOMDocument();
          $domObj->loadXML($response);

          $xpathObj = new Domxpath($domObj);

          $ret['query'] = $qry;
          $ret['LODSource'] = 'geonameid';
          $ret['LODSourceReadible'] = $this->LODSourceReadible;
          $ret['refURI'] = $refURI;
          $ret['refID'] = $refID;
          
          $ret['cModel'] = 'cwrc:place-entityCModel';
          $ret['ds_id'] = 'PLACE';
          $ret['collectionPID'] = 'cwrc:externalEntityStub-Place';
      
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

          // check to see if a label was found, if not report
          if ( !$ret['label'] )
          {
            $errStr
              = 'linked open data lookup - no error but no label: '
              //. "{\"" . $response . "\"} " 
              . ' ["' . $qry . '"]'
              . ' ["' . $refURI . '"]'
              . ' ["' . $refID . '"]'
              . ' ["' . $http_status . '"]'
              ;
            echo 'Curl error: ' . $errStr . "\n";
            watchdog('cwrc_entities', $errStr, array(), WATCHDOG_WARNING);
          }
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

        $addressStr = preg_replace("#".$this->uriPrefix."([^/]*)/?#",'${1}', $refURI);
        $qry = $this->qryPrefix.$addressStr;

        // use curl to lookup Linked Open Data source to retrieve and parse
        // xml response
        $ch = curl_init($qry);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true); // viaf redirects
        $response = curl_exec($ch);
        $http_status = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        if ( $response === false)
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . ' ' . $qry;
          echo 'Curl error: ' . $errStr;
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else if ( $http_status != "200" )
        {
          $errStr = 'Error during curl external linked open data lookup: '.curl_error($ch) . " [$http_status] " . ' ["' . $qry . '"]';
          echo 'Curl error: ' . $errStr . "\n";
          watchdog('cwrc_entities', $errStr, array(), WATCHDOG_ERROR);
        }
        else
        {
          $domObj = new DOMDocument();
          $domObj->loadXML($response);

          $xpathObj = new Domxpath($domObj);

          $ret['query'] = $qry; 
          $ret['LODSource'] = 'googlemaps';
          $ret['LODSourceReadible'] = $this->LODSourceReadible;
          $ret['refURI'] = $refURI;
          $ret['refID'] = $addressStr;
          
          $ret['cModel'] = 'cwrc:place-entityCModel';
          $ret['ds_id'] = 'PLACE';
          $ret['collectionPID'] = 'cwrc:externalEntityStub-Place';

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

          // check to see if a label was found, if not report
          if ( !$ret['label'] )
          {
            $errStr
              = 'linked open data lookup - no error but no label: '
              //. "{\"" . $response . "\"} " 
              . ' ["' . $qry . '"]'
              . ' ["' . $refURI . '"]'
              . ' ["' . $http_status . '"]'
              ;
            echo 'Curl error: ' . $errStr . "\n";
            watchdog('cwrc_entities', $errStr, array(), WATCHDOG_WARNING);
          }

        }
        return $ret;
    }
}





