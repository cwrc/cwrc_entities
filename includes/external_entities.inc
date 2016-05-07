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
      echo $result->textContent;
      if (!cwrc_entities_external_exists($result->textContent))
        {
          $tmp = cwrc_entities_external_lookup($result->textContent);
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
  return false;
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
            print_r($ret);
            break;
        }
    }
    return $ret;
}


/**
* given the result of an external entity detail lookup 
* parse the return and create a new entity stub
*
* @param $refURI
*     URI of the external entity
*/
function cwrc_entities_external_create_stub($refURI)
{

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
    function is_URI($uriStr)
    {
        if (1===preg_match('#http://viaf.org/viaf#',$uriStr))
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
        $response = file_get_contents($qry);
        
        //echo $response;
        //$xmlObj = new SimpleXMLElement($response);

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
                        $marcCodeSelector = "a";
                        break;
                    case "Corporate":
                        $tmp = 'cwrc:organization-entityCModel';
                        $marcCodeSelector = "a";
                        break;
                    case "UniformTitleWork":
                        $tmp = 'cwrc:citationCModel';
                        $marcCodeSelector = "t";
                        break;
                    default:
                        $tmp = 'unknown';
                        break;
                }
                break;
            }
        }
        $ret['type'] = $tmp;
        $ret['query'] = $qry;

        // grab label from VIAF response
        $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield[@dtype="MARC21"]/ns2:subfield[@code="'.$marcCodeSelector.'"]/text()';
        echo $xpathStr;
        $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);

        if ( ! $ret['label'])
        {
            //check UNIMarc
            // grab label from VIAF response
            $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield/ns2:subfield[@code="'.$marcCodeSelector.'"]/text()';
            echo $xpathStr;
            $ret['label'] = self::getSingleXPathItem($xpathStr, $xpathObj);
        }

        if ( $ret['type'] === 'cwrc:citationCModel' )
        {
            $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield/ns2:subfield[@code="a"]';
            $ret['author'] = self::getSingleXPathItem($xpathStr, $xpathObj);
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

        //$tmpStr = preg_replace("#".$uriPrefix."#",'', $refURI);
        
        // get ID from given URI and add to a lookup API call to return XML
        $tmpPrefixLen = strlen($this->uriPrefix);
        $tmpId = substr($refURI, $tmpPrefixLen, strlen($refURI)-$tmpPrefixLen);
        $qry = $this->qryPrefix.$tmpId;
        //print_r("\nxx\n" .$tmp ."\nxx\n");

        $response = file_get_contents($qry);
        
        $domObj = new DOMDocument();
        $domObj->loadXML($response);

        $xpathObj = new Domxpath($domObj);

        $ret['type'] = 'cwrc:place-entityCModel';
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

    function is_URI($uriStr)
    {
        //if (1===preg_match('#http://www.google.ca/maps/place#',$uriStr))
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
        //print_r("\n\n" .$tmp ."\n");

        $response = file_get_contents($qry);
        
        $domObj = new DOMDocument();
        $domObj->loadXML($response);

        $xpathObj = new Domxpath($domObj);
        //$xpathObj->registerNamespace('ns2',"http://viaf.org/viaf/terms#");

        $ret['type'] = 'cwrc:place-entityCModel';
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

        return $ret;
    }
}





