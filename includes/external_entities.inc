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
* @param $typeStruct
*     data structure containing the details of the stub to create 
*/
function cwrc_entities_external_create_stub($typeStruct)
{

  $xml = '';

  switch ($typeStruct['cModel'])
  {
    case 'cwrc:person-entityCModel':
      $xml = populateTemplatePerson($typeStruct);
      break;
    case 'cwrc:organization-entityCModel':
      $xml = populateTemplateOrganization($typeStruct);
      break;
    case 'cwrc:place-entityCModel':
      $xml = populateTemplatePlace($typeStruct);
      break;
    case 'cwrc:citationCModel':
      $xml = populateTemplateCitation($typeStruct);
      break;
    default:
      break;
  }

  print_r($xml);


}


function populateTemplatePerson($typeStruct)
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
                    <projectId>{$typeStruct['LODSource']}</projectId>
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

function populateTemplateOrganization($typeStruct)
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
                    <projectId>{$typeStruct['LODSource']}</projectId>
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


function populateTemplatePlace($typeStruct)
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

function populateTemplateCitation($typeStruct)
{
  $today = date('Y-m-d');
  $xml = <<<EOF
<?xml version="1.0" encoding="UTF-8"?>
<mods xmlns="http://www.loc.gov/mods/v3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.loc.gov/mods/v3 http://www.loc.gov/standards/mods/mods.xsd">
    <titleInfo valueURI="{$typeStruct['refURI']}">
        <title>{$typeStruct['label']}</title>
    </titleInfo>
    <identifier type="viaf">{$typeStruct['refURI']}</identifier>
    <accessCondition type="use and reproduction">Use of this public-domain resource is governed by the <a rel="license" href="http://creativecommons.org/licenses/by-nc/4.0/" target="_blank">Creative Commons Attribution-NonCommercial 4.0 International License</a>.</accessCondition>
    <recordInfo>
        <recordContentSource>VIAF</recordContentSource>
        <recordIdentifier source="viafID">{$typeStruct['refID']}</recordIdentifier>
        <recordCreationDate encoding="w3cdtf">$today</recordCreationDate>
        <recordOrigin>{$typeStruct['recordOrigin']}</recordOrigin>
    </recordInfo>
</mods>

EOF;

  return $xml;
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
        $ret['cModel'] = $tmp;
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

        if ( $ret['cModel'] === 'cwrc:citationCModel' )
        {
            $xpathStr = '/ns2:VIAFCluster/ns2:mainHeadings/ns2:mainHeadingEl/ns2:datafield/ns2:subfield[@code="a"]';
            $ret['author'] = self::getSingleXPathItem($xpathStr, $xpathObj);
        }

        $ret['recordOrigin'] = 'Entity stub derived from an entity record in the Virtual International Authority File (VIAF) database.';

        $ret['refURI'] = $refURI;

        $ret['refID'] = preg_replace("#".$this->uriPrefix."([^/]*)/?#",'${1}', $refURI);

        $ret['LODSource'] = 'viaf';
        $ret['LODSourceReadible'] = 'VIAF';

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
        $refId = substr($refURI, $tmpPrefixLen, strlen($refURI)-$tmpPrefixLen);
        $qry = $this->qryPrefix.$refId;
        //print_r("\nxx\n" .$tmp ."\nxx\n");

        $response = file_get_contents($qry);
        
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
        $ret['LODSourceReadible'] = 'GeoNames';

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

        //ToDo: try/catch if server down or mistaken uri
        $response = file_get_contents($qry);
        
        $domObj = new DOMDocument();
        $domObj->loadXML($response);

        $xpathObj = new Domxpath($domObj);
        //$xpathObj->registerNamespace('ns2',"http://viaf.org/viaf/terms#");

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
        $ret['LODSourceReadible'] = 'Google Maps';

        return $ret;
    }
}





