cwrc_entities
=============

CWRC Entity Handler

## Introduction

## Requirements

https://github.com/cwrc/basic-solr-config/blob/cwrc-entities/islandora_transforms/CWRC_Entities_to_solr.xslt

islandora_workflow

## Installation

## Troubleshooting/Issues

## Maintainers/Sponsors

## Development

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)


# Documentation: Open Annotation API


## set of annotations with a given 'hasTarget'

#### URL syntax
islandora/cwrc_entities/oa/HasTarget/{uri encoded}

For paging of results - unsupported as of 2014-10-06

islandora/cwrc_entities/oa/HasTarget/{uri encoded}?page.start={}&page.rows={}

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Request parameters
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| page.start    | (optional) For paging of results - the results page
| page.rows     | (optional) For paging of results - the number of rows per page

#### Response: 200 OK
##### Content-Type: application/json
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| hasTarget     | The query 'hasTarget' uri
| numFound      | The number of results in the 'oa_items' array
| page.start    | (optional) For paging of results - the results page of the current 'oa_items'
| page.rows     | (optional) For paging of results - the number of rows per page
| oa_items      | The number of results identified by the 'hasTarget' uri


#### Example Response
```JSON
{
    "hasTarget": "islandora:b305fc90-a59e-415e-8c58-f7ee5d06ad2f",
    "numFound": "0",
    "page": {
        "start": "0",
        "rows": "5"
    },
    "oa_items": [{},{},...]
}
```





## Mint URI

#### URL syntax
islandora/cwrc_entities/oa/mintURI

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Request parameters
None

#### Response: 200 OK
##### Content-Type: application/json
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| uri           | OA minted annotation

#### Example Response
```JSON
{"uri","http:\/\/id.cwrc.ca\/cwrc:2755fb77-b11a-43b8-947c-43654b1cb6d6"}
```




## Create A New Annotation

#### URL syntax
islandora/cwrc_entities/oa/annotation

#### HTTP Method
POST

#### Headers
Accept: application/json

#### POST (form-data)
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| URI           | The identifier for the annotation
| hasTarget     | hasTarget URI for the annotation
| annotation    | RDF XML serialiazation of the annotation

#### Response: 201 Created
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.


## Update An Existing Annotation

#### URL syntax
islandora/cwrc_entities/oa/annotation/{URI}

#### HTTP Method
PUT

#### Headers
Accept: application/json

#### POST (form-data)
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| annotation    | RDF XML serialiazation of the annotation

#### Response: 200
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.




## Get An Existing Annotation

#### URL syntax
islandora/cwrc_entities/oa/annotation/{URI}

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Request parameters
none

#### Response: 200
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.




## Delete An Existing Annotation

#### URL syntax
islandora/cwrc_entities/oa/annotation/{URI}

#### HTTP Method
DELETE

#### Headers
Accept: application/json

#### Request parameters
none

#### Response: 200
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.







# Entities 

{type} = person|organization|place|title


## Search

#### URL syntax
islandora/cwrc_entities/v1/search/{type}?

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Request parameters
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| query         | full-text query terms 
| limit         | results per page 
| page          | page number of results to retrieve

#### Response: 200
##### Content-Type: application/json
Returns a JSON formated Solr result 



## Create A New Entity 

#### URL syntax
islandora/cwrc_entities/v1/{type}/{PID}

#### HTTP Method
POST

#### Headers
Accept: application/json

#### POST (form-data)
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| data          | XML entity object as per the CWRC schema 

#### Response: 201 Created
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.


## Update An Existing Entity 

#### URL syntax
islandora/cwrc_entities/v1/{type}/{PID}


#### HTTP Method
PUT

#### Headers
Accept: application/json

#### POST (form-data)
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| annotation    | RDF XML serialiazation of the annotation

#### Response: 200
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.


## Get An Existing Annotation

#### URL syntax
islandora/cwrc_entities/v1/{type}/{URI}

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Request parameters
none

#### Response: 200
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.




## Delete An Existing Entity 

#### URL syntax
islandora/cwrc_entities/v1/{type}/{URI}

#### HTTP Method
DELETE

#### Headers
Accept: application/json

#### Request parameters
none

#### Response: 200
##### Content-Type: application/json
Returns the same response as a [GET Annotation](#response-200-ok) request.











### Common Responses
Unless otherwise specified each end-point can return the following responses.

#### Response: 401 Unauthorized
##### No response body.
In general 401 means an anonymous user attempted some action and was denied.
Either the action is not granted to anonymous uses in the Drupal permissions,
or XACML denied the action for the anonymous user.

#### Response: 403 Forbidden
##### No response body.
In general 403 means an authenticated user attempted some action and was denied.
Either the authenticated user does not have a Drupal role that grants him/her
permission to perform that action, or XACML denied the action for that user.

#### Response: 404 Not Found
##### No response body.
In general a 404 will occur, when a user tries to perform an action on a
object or datastream that doesn't not exist. Or XACML is hiding that
object or datastream from the user.

404 Responses can be returned even if the user was **not** determined to have
permission to perform the requested action, as the resource must be first
fetched from fedora before the users permission can be determined.

#### Response: 500 Internal Server Error

##### Content-Type: application/json
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| message       | A detail description of the error.

Any problem can trigger a 500 error, but in general you'll find that this is
typically an error returned by Fedora.


