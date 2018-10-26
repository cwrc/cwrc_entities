<?php

/**
 * @file
 * Handles the Entity callback requirements for cwrc_entities_module.
 */


define("CWRC_ENTITIES_DATASTREAM",   "");
define("CWRC_ENTITIES_DATASTREAM_LABEL", "");
define("CWRC_ENTITIES_CMODEL",   "cwrc:openAnnotationCModel");
define("CWRC_ENTITIES_HASTARGET", "cwrc:hasTarget");
define("CWRC_ENTITIES_NS", 'http://cwrc.ca/ns/relsext#');

/**
 * Menu callback to handle the REST methods to handle an Entity .
 *
 * model after: islandora_image_annotation - islandora_rest_datastream and 
 *    islandora_rest_get_response functions to model the REST functionality
 *  
 * @param array $uri
 *   The GET parameters that were embedded within the path of the URL. They
 *   typically contain the following fields:
 *   - pid: Optional, the PID of the requested resource
 *
 * @param string cModel
 *   optional - Fedora cModel describing the entity type
 *
 *
 * @param string collection = NULL
 *    optional - Fedora PID of the collection to add the entity as a child
 *
 * @param string workstatus = NULL
 *    optional - CWRC workflow status information 
 *
 *
 * @param string workstamp = NULL
 *    optional - CWRC workflow status information 
 *
 *
 * @param string entityToDCXSL = NULL
 *    optional - path to an XSLT to generate a DC datastream 
 *
 */
function cwrc_entities_rest_api(
  $uri = NULL
  , $cModel = NULL
  , $collectionPID = NULL
  , array $activity = NULL
  , $entityToDCXSLT = NULL
  , $dataStreamID = NULL
  ) 
  {

  // include REST utilities for the API callbacks 
  module_load_include('inc', 'islandora_rest', "includes/utilities"); 
  $endpoint = 'object';

  //$url_parameters = array_filter($url_parameters);
  $request_method = islandora_rest_get_request_method();

  //$pid = !empty($url_parameters['pid']) ? $url_parameters['pid'] : FALSE;
  //if ( $pid===FALSE ) {
  //  $pid = islandora_markup_editor_oa_convert_uri_to_pid($uri);
  //}

  $pid = cwrc_entities_convert_uri_to_pid($uri);
  $dsid = FALSE;

  try {
    // Fetch the requested resource. Throws 404, if the resource is not found.
    $resource = islandora_rest_get_requested_resource($pid, $dsid);

    // Check Drupal Permissions. Throws 401, 403, if access is denied.
    cwrc_entities_require_access($endpoint, $request_method, $resource);

    // Generate the Response.
    $response_function = "cwrc_entities_{$request_method}_response";
    if (function_exists($response_function)) {
      $parameters = array(
        'uri' => $uri
        , 'pid' => $pid
        , 'request_method' => $request_method
        , 'request' => islandora_rest_get_request_parameters()
        , 'resource' => $resource
        , 'cModel' => $cModel
        , 'collectionPID' => $collectionPID
        , 'activity' => $activity
        , 'entityToDCXSLT' => $entityToDCXSLT
        , 'dataStreamID' => $dataStreamID
      );

      // Can throw 400, 401, 500, etc.
      $response = $response_function($parameters);
    }
  }
  catch (Exception $e) {
    //drupal_add_http_header('statusText', $e->getMessage());
    drupal_add_http_header('responseText', $e->getMessage());
    drupal_add_http_header('Content-Type', 'application/json; utf-8');
    //$response =  drupal_json_encode(array('error' => $e->getMessage()));
    $response =  drupal_json_encode(array('message' => $e->getMessage()));
  }

  $encode = is_array($response) || is_object($response);
  if ($encode) {
    // Reponses that don't generate objects or arrays, must set their own
    // content types. Not calling drupal_json_output because we are setting
    // encoding.
    drupal_add_http_header('Content-Type', 'application/json; utf-8');
    return drupal_json_encode($response);
  }
  print $response;
  exit();
  //return $response;
}



/**
 * Responds to the GET request for this resource.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - resource: The object to generate the response for.
 *
 * @return array
 *
 *
*/

function cwrc_entities_get_response(array $parameters) {

  $fedora_object = $parameters['resource'];
  $ds_id = $parameters['dataStreamID'];

  if (!isset($fedora_object) or !isset($fedora_object[$ds_id])) {
    $tmp = $parameters['pid'];
    drupal_add_http_header('Status', 404);
  }
  else {
    $tmp = $fedora_object[$ds_id]->content;
  }

  return drupal_json_output($tmp);
}


/**
 * Responds to the PUT request for this resource.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - resource: The object to generate the response for.
 *   - request: The request parameters
 *     - annotation: The object's OA datastream contents.
 *
 * @return 
 */
function cwrc_entities_put_response(array $parameters) {

  try {
    $fedora_object = $parameters['resource'];
    
    $ds_id = $parameters['dataStreamID'];
      $cModel = $parameters['cModel'];
    $activity_params = $parameters['activity'];
    $entityToDCXSLT = $parameters['entityToDCXSLT'];

    $request = $parameters['request'];
    // content is the body of the request (no parameter e.g., $request[CWRC_ENTITES_REQUEST_CONTENT_PARAM];
    $content_blob = $request;

    if (!isset($fedora_object) or !isset($fedora_object[$ds_id])) {
      drupal_add_http_header('Status', 404);
    }
    else if (!isset($content_blob) || trim($content_blob) === '') {
      throw new Exception("Unable to modify object {$fedora_object->id}", 510); 
    }
    else {

    // Initialize the content_blob within DOMDocument and Xpath libs so we can modify the tags.
	  $contentDOM = init_dom_mod($content_blob);
    
    // Get the existing Datastreams responsibility tags 
    $contentDOM = tei_get_previous_responsibilities($contentDOM, $fedora_object, $cModel);
    
	  $contentDOM = tei_add_responsibilities($contentDOM, $cModel, 'modified');
      $contentDOM = tei_add_xmlid($contentDOM, $pid, $cModel);
	  $content_blob = close_dom_mod($contentDOM);
 
      // update content datastream
      $fedora_object[$ds_id]->setContentFromString($content_blob);

      // add a workflow record 
      if (isset($activity_params)) {
        islandora_markup_entity_add_workflow($fedora_object, $activity_params);
      }

      // update the DC datastream and the associated Fedora Object label
      if (isset($entityToDCXSLT)) {
        $dc = islandora_markup_entity_build_DC($fedora_object, $entityToDCXSLT, $content_blob);
        islandora_markup_entity_update_DC($fedora_object, $dc);
        $title  = islandora_markup_entity_get_title_from_DC($dc);
        $fedora_object->label = $title;
      }
    }
  }
  catch (Exception $e)
  {
    throw new Exception($e->getMessage(), $e->getCode()); //drupal_add_http_header('Status', 500);
    //return drupal_json_output( array('error' => $e->getMessage() ) );
    //return  array('error' => $e->getMessage()) ;
  }

  return drupal_json_output( array('pid' => $fedora_object->id) );
  //return islandora_markup_editor_entities_get_response(array('resource' => $fedora_object));
  //return drupal_json_output($content_blob);
      //return drupal_json_output(print_r($fedora_object));
  //return drupal_json_output(print_r($activity_params)); 
  //return drupal_json_output("");
}


/**
 * Responds to the POST request for this resource.
 *
 * Creates a new object, with the given $params.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - request: The request parameters
 *     - data: content blob to store in Fedora 
 *
 *   - dataStreamID: DS_ID to store the content of the Fedora Object
 *
 * @return array
 *   Returns the same response as a GET request to the existing object.
 */
function cwrc_entities_post_response(array $parameters) {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $ds_id = $parameters['dataStreamID'];
  $cModel = $parameters['cModel'];
  $collectionPID = $parameters['collectionPID'];
  $activity_params = $parameters['activity'];
  $entityToDCXSLT = $parameters['entityToDCXSLT'];

// The TEI xml is the second element in the request, unless keys are added to the request array.

 $request = $parameters['request']['tei'];
 
// The Collection PID is the first parameter.  $collectionPID = $parameters['request']['collectionPID'];

  $collectionPID = $parameters['request']['collectionPID'];
  

  $content_blob = $request;

  if (!isset($content_blob) || trim($content_blob) === '') {
    throw new Exception("Unable to add request content empty", 510); 
  }

  $hasTarget = ((isset($request['hasTarget'])) ? $request['hasTarget'] : NULL);

  if (isset($request['uri'])) {
    $uri = $request['uri'];
    $pid = cwrc_entities_convert_uri_to_pid($uri);
  }

  if (!isset($pid) or $pid == FALSE)
  {
    // Get the namespace of the parent Collection
    $namespace_prefix = explode(':', $collectionPID)[0];
    // generate an id for the object
    $connection = islandora_get_tuque_connection();
    $pid = $connection->repository->getnextIdentifier($namespace_prefix, TRUE);
  }

  // prepare a new object.
  $fedora_object
    = islandora_prepare_new_object(
      $pid
      , NULL 
      , array()
      , array($cModel)
      , array()
      , array(
          'relationship' => 'isMemberOfCollection'
          , 'pid' => "$collectionPID"
        )
    );

  // add the new object as a child of a specific collection
  if (isset($collectionPID))
  {
    $fedora_object->relationships->add(
      FEDORA_RELS_EXT_URI
      , 'isMemberOfCollection'
      , $collectionPID 
    );
  }

  // add a hasTarget relationship
  if (isset($hasTarget) or isset($hasTarget[$ds_id])) {
    # add RELS-EXT relationships
    # ToDo: temporary 'targets', consider removing
    $fedora_object->relationships->add(
      ISLANDORA_RELS_EXT_URI
      , 'targets'
      , substr( $hasTarget, strrpos( $hasTarget, '/' )+1) 
    );
    $fedora_object->relationships->add(
      CWRC_ENTITIES_NS
      , CWRC_ENTITES_HASTARGET
      , $hasTarget
      , RELS_TYPE_FULL_URI
    );
  }

  // add a workflow record 
  if (isset($activity_params)) {
    islandora_markup_entity_add_workflow($fedora_object, $activity_params);
  }

  // update the DC datastream and the associated Fedora Object label
  if (isset($entityToDCXSLT)) {
    $dc = islandora_markup_entity_build_DC($pid,$entityToDCXSLT,$content_blob);
    $title  = islandora_markup_entity_get_title_from_DC($dc);
    islandora_markup_entity_update_DC($fedora_object, $dc);
    $fedora_object->label = $title;
  }


  $contentDOM = init_dom_mod($content_blob);
  $contentDOM = tei_add_responsibilities($contentDOM, $cModel, 'created') ;
  $contentDOM = tei_add_xmlid($contentDOM, $pid, $cModel);
  $content_blob = close_dom_mod($contentDOM);
  

  // create content datastream
  $content_ds 
    = $fedora_object->constructDatastream($ds_id, 'M');
  $content_ds->label = $ds_id;
  $content_ds->mimeType = 'text/xml';
  $content_ds->setContentFromString($content_blob);
  $fedora_object->ingestDatastream($content_ds);


  // save the object 
  islandora_add_object($fedora_object);

  // set the response status
  drupal_add_http_header('Status', 201);
  //return drupal_json_output($parameters);

  return drupal_json_output( array('pid' => $fedora_object->id) );

  // Mock the Get Response.
  //return cwrc_entities_get_response(array('resource' => $fedora_object));
}


/**
 * Responds to the DELETE request for this resource.
 *
 * Delete's the given object.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - resource: The object to generate the response for.
 */
function cwrc_entities_delete_response(array $parameters) {
  $object = $parameters['resource'];
  $object->repository->purgeObject($object->id);
}



function cwrc_entities_convert_uri_to_pid($uri)
{
  $match_array = array();
  //preg_match("/^.*\/(cwrc:[\w-]*)/i",$uri, $match_array);
  //preg_match("/(cwrc:[\w-]*)/i",$uri, $match_array);
  preg_match("/([\w]+:[\w-]*)/i",$uri, $match_array);
  $pid = !empty($match_array[1]) ? $match_array[1] : FALSE;
  return $pid;
}


/**
 * build a DC xml file 
 *
 * @param string $pid
 *   PID of the fedora object 
 *
 */
function islandora_markup_entity_build_DC($pid, $entityToDCXSLT, $content_blob)
{
    // setup XSL
    // Get the path to the content_to_dc.xsl file.
    $path_to_xsl_file
          = drupal_get_path('module', 'cwrc_entities') . $entityToDCXSLT;
    $xsl_doc = new DOMDocument();
    $xsl_doc->load($path_to_xsl_file);

    $xslt_proc = new XSLTProcessor();
    $xslt_proc->importStylesheet($xsl_doc);
    
    $input_doc = new DOMDocument();
    @$input_doc->loadXML($content_blob);

    // build the DC content
    // Convert MODS to DC.
    // Transform to DC xml.
    // Apply the stylesheet.
    $xslt_proc->setParameter( "", "PID_PARAM", $pid );
    $dc_xml = $xslt_proc->transformToXML($input_doc);

    return $dc_xml;
}


/**
 * Update the DC datastream of the specific object 
 *
 * @param object $fedora_object
 *   The requested Fedora object/datastream .
 * @param string $dc_xml
 *   DC record for the Fedora Object 
 *
 */
function islandora_markup_entity_update_DC($fedora_object, $dc_xml)
{
    if (!isset($fedora_object['DC'])) {
      // create content datastream
      $ds 
        = $fedora_object->constructDatastream('DC', 'M');
      $ds->label = 'DC';
      $ds->mimeType = 'text/xml';
      $ds->setContentFromString($dc_xml);
      $fedora_object->ingestDatastream($ds);
    }
    else
    {
      $fedora_object['DC']->setContentFromString($dc_xml);
    }
}

/**
 * Get the Fedora Object Title from the DC record 
 *
 * @param string $dc_xml
 *   DC record for the Fedora Object 
 *
 */
function islandora_markup_entity_get_title_from_DC($dc_xml)
{
    $dc_doc = new DOMDocument();
    $dc_doc->loadXML($dc_xml); 
    $xpath = new DOMXPath($dc_doc);
    $xpath->registerNamespace('dc',"http://purl.org/dc/elements/1.1/");
    $xpath->registerNamespace('oai_dc',"http://www.openarchives.org/OAI/2.0/oai_dc/");
    $label_node = $xpath->evaluate('/oai_dc:dc/dc:title/text()');

    return $label_node->item(0)->nodeValue;
}


/**
 * add a simple workflow activity to the Entity's WORKFLOW datastream 
 *
 * @param object $fedora_object
 *   The requested Fedora object/datastream .
 * @param object $activity_params
 *   an associative array of activity params such as stamp, status
 *
 */
function islandora_markup_entity_add_workflow($fedora_object, array $activity_params) {
    //$use_realname = module_exists('realname');
    module_load_include('inc', 'islandora_workflow_rest', 'includes/cwrc_workflow');
    module_load_include('inc', 'islandora_workflow_rest', 'includes/basic_enum');
    module_load_include('inc', 'islandora_workflow_rest', 'includes/workflow_item');
    module_load_include('inc', 'islandora_workflow_rest', 'includes/utilities');

    try
    {
      if (!isset($fedora_object['WORKFLOW'])) {
          $workflow = CWRCWorkflowAPI::fromTemplate();
          CWRCWorkflowAPI::updateDatastream($workflow, $fedora_object);
      }
      else {
          $workflow = CWRCWorkflowAPI::fromDatastream($fedora_object['WORKFLOW']);
      }

      //createDefaultParamsForWorkflow
      $entry = new WorkflowItem(WorkflowConst::WORKFLOW, array());
      $activity = new WorkflowItem(WorkflowConst::ACTIVITY, $activity_params);
      //$assigned = new WorkflowItem(WorkflowConst::ASSIGNED, $assigned_params);
      $assigned = NULL;

      $workflow->addWorkflowStep($fedora_object->id, $entry, $activity, $assigned);
      CWRCWorkflowAPI::updateDatastream($workflow, $fedora_object);
    }
    catch (Exception $e)
    {
      throw new Exception("Unable to create workflow. reason: " . $e->getMessage());
    }
}



/**
 * Checks to see if the current user has access for the requested resource.
 *
 * @throw Exception If not access is not granted to the user. If the user is not
 * logged in throw a 'Unauthorized' exception, if they are logged in throw a
 * 'Forbidden' exception.
 *
 * @param string $endpoint
 *   The rest end-point to generate the response for.
 * @param array $request_method
 *   The request method (GET/POST/PUT/DELETE).
 * @param object $resource
 *   The requested Fedora object/datastream (optional).
 *
 * @return bool
 *   TRUE if all parameters are present FALSE otherwise.
 */
function cwrc_entities_require_access($endpoint, $request_method, $resource = NULL) {
  if (!cwrc_entities_accessible($endpoint, $request_method, $resource)) {
    if (user_is_anonymous()) {
      throw new Exception('Unauthorized', 401);
    }
    else {
      throw new Exception('Forbidden', 403);
    }
  }
}

/**
 * Check module permissions etc, to see if the current action is allowed.
 *
 * @param array $request_method
 *   The request method (GET/POST/PUT/DELETE)
 * @param object $resource
 *   The requested Fedora object/datastream (optional).
 *
 * @return bool
 *   TRUE if the user is allowed to access the rest end-point, FALSE otherwise.
 */
function cwrc_entities_accessible($endpoint, $request_method, $resource = FALSE) {
  $perm = cwrc_entities_resource_access_permission($endpoint, $request_method);
  if (is_subclass_of($resource, 'AbstractObject')) {
    return islandora_object_access($perm, $resource);
  }
  elseif (is_subclass_of($resource, 'AbstractDatastream')) {
    return islandora_datastream_access($perm, $resource);
  }
  return user_access($perm);
}


/**
 * Gets the defined access permission for the given resource and request method.
 *
 * @param string $endpoint
 *   The rest end-point to generate the response for.
 * @param array $request_method
 *   The request method (GET/POST/PUT/DELETE).
 */
function cwrc_entities_resource_access_permission($endpoint, $request_method) {
  $perm_map = array(
    'object' => array(
      'GET' => CWRC_ENTITY_VIEW,
      'PUT' => CWRC_ENTITY_CREATE,
      'POST' => CWRC_ENTITY_CREATE,
      'DELETE' => CWRC_ENTITY_CREATE,
    ),
  );
  return $perm_map[$endpoint][$request_method];
}



/**
 * call the Solr search API 
 * modeled off of CWRC-ccm-rest-api/controllers/entity.php
 *
 * @param string $cModel
 *    Fedora cModel describing the type of entity
 *
 * @param array solr_field_array
 *   Solr field array names to search 
 **/
function cwrc_entities_search($cModel, $solr_field_array)
{
  try
  {
    module_load_include('inc', 'islandora_rest', "includes/utilities");

    $tmp = islandora_rest_get_request_parameters();
    $limit  = ((isset($tmp['limit'])) ? $tmp['limit'] : NULL);
    $page   = ((isset($tmp['page'])) ? $tmp['page'] : NULL);
    $queryTerms  = $tmp['query'];

    // not sure if needed - 2014-11-5 - copied directly from ccm-rest-api entity.php
    $queryTerms = strtolower($queryTerms);
    $queryTerms = preg_replace("/\+/", " ", $queryTerms);
    $queryTerms = urldecode($queryTerms);
    $queryTerms = preg_replace("/\s/", "+", $queryTerms);

    // build query string
    $iter = 0;
    $queryStr = "(";
    foreach ($solr_field_array as $item)
    {
      if ($iter !=0)
      {
        $queryStr = $queryStr . "%20OR%20";
      }
      $queryStr .= $item . ":" . "(" . $queryTerms .")";
      $iter++;
    }
    $queryStr .= ")";

    $request = array(
        'wt' => 'json'
        , 'limit' => $limit
        , 'page' => $page
        //, 'f' => array("RELS_EXT_hasModel_uri_ms:".str_replace(':','\:',$cModel))
        , 'f' => array("RELS_EXT_hasModel_uri_ms:".islandora_solr_lesser_escape($cModel))
      );

    $solr = new IslandoraSolrQueryProcessor();
    $solr->buildAndExecuteQuery($queryStr, $request);
    $response = $solr->islandoraSolrResult;
  }
  catch (Exception $e) {
    drupal_add_http_header('Status', $e->getCode());
    $response = array('message' => $e->getMessage());
  }
  $encode = is_array($response) || is_object($response);
  if ($encode) {
    // Reponses that don't generate objects or arrays, must set their own
    // content types. Not calling drupal_json_output because we are setting
    // encoding.
    drupal_add_http_header('Content-Type', 'application/json; utf-8');
    return drupal_json_output($response);
  }
  return drupal_json_encode($response);
}



/**
 *
 */
function cmodel_to_TEI_type_constant($cmodel) {
	switch ($cmodel) {
		case 'cwrc:person-entityCModel':
			return 'Person';
		case 'cwrc:place-entityCModel':
			return 'Place';
		case 'cwrc:organization-entityCModel':
			return 'Org';
		default:
			return NULL;
	}
}

/**
 * Initializes DOMDocument and XPath objects,
 * and packs them into an array for various modifications.
 */ 
function init_dom_mod($content_blob) {
  $dom = new DOMDocument();
  @$dom->loadXML($content_blob);

  $xpath = new DOMXPath($dom);
  $ns = $dom->documentElement->namespaceURI;

  $xpath->registerNamespace("ns", $ns);
	
  $contentDOM = [
  'xpath' => $xpath,
  'dom' => $dom,
  ];
  return $contentDOM;
}

function close_dom_mod($contentDOM) {
	
	return $contentDOM['dom']->saveXML();
}





/*
 * Grabs previous responsibilities tags 
 */
function tei_get_previous_responsibilities($contentDOM, $object, $cModel){
  
  // This converts the cmodel machine name to person, place, or org.
  $teiType = cmodel_to_TEI_type_constant($cModel);
  
  $dsid = strtoupper($teiType);
  $tei = islandora_datastream_load($dsid, $object);
  $content = $tei->getContent();
  
  // Old Responsibility Tags
  $oldRes = new DOMDocument();
  @$oldRes->loadXML($content);

  // We created a DOMDocument, now lets make an XPath.
  $oldResXpath = new DOMXPath($oldRes);
  $ns = $oldRes->documentElement->namespaceURI;

  $oldResXpath->registerNamespace("ns", $ns);
  
     /* For Organizations and Places, the respons tag is inside a Note tag.  */
  if( $cModel == 'cwrc:place-entityCModel' || $cModel == 'cwrc:organization-entityCModel')
  {
    // So we can pull out the old Respons and <note> tags.
    $tei_entity_xpath = '/ns:TEI/ns:text/ns:body/ns:list' . $teiType . '/ns:' . strtolower($teiType) . '/ns:note';
    $oldResList = $oldResXpath->evaluate($tei_entity_xpath);
  } else {
    // So we can pull out the old Respons and <note> tags.
    $tei_entity_xpath = '/ns:TEI/ns:text/ns:body/ns:list' . $teiType . '/ns:' . strtolower($teiType) . '/ns:respons';
    $oldResList = $oldResXpath->evaluate($tei_entity_xpath);
  }
  
  $tei_entity_xpath = '/ns:TEI/ns:text/ns:body/ns:list' . cmodel_to_TEI_type_constant($cModel) . '/ns:' . strtolower(cmodel_to_TEI_type_constant($cModel));
 
  $targetNode = $contentDOM['xpath']->evaluate($tei_entity_xpath);   
  foreach( $oldResList as $key => $resTag ) {
    $targetNode->item(0)->appendChild($contentDOM['dom']->importNode($resTag, TRUE));
  }
  
   

  //$contentDOM['dom']->importNode($oldResList->item(0));
  return $contentDOM;
}
  
  
  
  
/*
 * Adds <respons> tags to TEI datastreams.
 */
function tei_add_responsibilities($contentDOM, $cModel, $respType) {
  global $user;
  
  global $base_url;   // Will point to http://www.example.com
  global $base_path;  // Will point to at least "/" or the subdirectory where the drupal in installed.
  $user_link = $base_url . $base_path . 'users/' . $user->uid;


  $responsString = '<respons locus="value">
<desc>
<date type="' . $respType . '" source="https://www.w3.org/TR/NOTE-datetime" when="' . date('Y-m-d') . '"/>
<persName ref="' . $user_link .  '">' . realname_load($user) . '</persName>
</desc>
</respons>';

  /* For Organizations and Places, the respons tag is inside a Note tag.  */
  if( $cModel == 'cwrc:place-entityCModel' || $cModel == 'cwrc:organization-entityCModel')
  {
	  $responsString = '<note type="resp_statement">
	  ' . $responsString . '
	  </note>';
	  watchdog('braorg and placedmodule', '<pre>Dumping @variable </pre>', array('@variable' => print_r($responsString, true)));
  }

  $responsNode = new DOMDocument();
  @$responsNode->loadXML($responsString);
  
    /* For Organizations and Places, the respons tag is inside a Note tag.  */
  if( $cModel == 'cwrc:place-entityCModel' || $cModel == 'cwrc:organization-entityCModel')
  {
    $injectThis = $responsNode->getElementsByTagName('note')->item(0);
  } else {
    $injectThis = $responsNode->getElementsByTagName('respons')->item(0);
  }
 
  $tei_entity_xpath = '/ns:TEI/ns:text/ns:body/ns:list' . cmodel_to_TEI_type_constant($cModel) . '/ns:' . strtolower(cmodel_to_TEI_type_constant($cModel));
  $personNode = $contentDOM['xpath']->evaluate($tei_entity_xpath);
  $personNode = $personNode->item(0);
  $personNode->appendChild($contentDOM['dom']->importNode($injectThis, TRUE));

  return $contentDOM;
}


/*
 * Adds <respons> tags to TEI datastreams.
 */
function tei_add_xmlid($contentDOM, $pid, $cModel) {
  
  $tei_entity_xpath = '/ns:TEI/ns:text/ns:body/ns:list' . cmodel_to_TEI_type_constant($cModel) . '/ns:' . strtolower(cmodel_to_TEI_type_constant($cModel));
  
  
  $personNode = $contentDOM['xpath']->evaluate($tei_entity_xpath);
  $personNode = $personNode->item(0);

  $personNode->setAttribute('xml:id', 'commons.cwrc.ca/' . $pid);

  return $contentDOM;
}


