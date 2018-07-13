<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xmlns:mods="http://www.loc.gov/mods/v3" xmlns:tei="http://www.tei-c.org/ns/1.0"
    exclude-result-prefixes="mods tei" xmlns:dc="http://purl.org/dc/elements/1.1/"
    xmlns:oai_dc="http://www.openarchives.org/OAI/2.0/oai_dc/"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

    <!--
  * MVI
  * June 18, 2018
  * given a TEI entity - person, organization or place - output a DC format datastream
  * designed to work with 1 entity per file
  -->

    <xsl:output encoding="UTF-8" method="xml" indent="yes" omit-xml-declaration="no"/>


    <!--
  * PID value passed into the transform 
  -->
    <xsl:param name="PID_PARAM" select="'zzzz'"/>


    <!--
  * build DC
  -->
    <xsl:template match="/tei:TEI | /mods:modsCollection/mods:mods | /mods:mods">
        <oai_dc:dc
            xsi:schemaLocation="http://www.openarchives.org/OAI/2.0/oai_dc/ http://www.openarchives.org/OAI/2.0/oai_dc.xsd">
            <dc:title>
                <xsl:call-template name="GET_DC_TITLE"/>
            </dc:title>
            <dc:identifier>
                <xsl:value-of select="$PID_PARAM"/>
            </dc:identifier>
        </oai_dc:dc>
    </xsl:template>

    <!--
  * select the appropriate DC title template
  -->
    <xsl:template name="GET_DC_TITLE">
        <xsl:apply-templates select="tei:teiHeader/tei:fileDesc/tei:titleStmt/tei:title | mods:titleInfo" mode="entity_dc_title"/>
    </xsl:template>

    <xsl:template match="tei:teiHeader/tei:fileDesc/tei:titleStmt/tei:title" mode="entity_dc_title">
        <xsl:choose>
            <xsl:when test="text() != ''">
                <xsl:value-of select="text()"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>Unknown entity name</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
        
    </xsl:template>
    
    <xsl:template match="mods:titleInfo" mode="entity_dc_title">
        <xsl:choose>
            <xsl:when test="not(@type) and mods:title">
                <xsl:value-of select="mods:title"/>
            </xsl:when>
            <xsl:when test="not(@type) and @usage='primary' and mods:title ">
                <xsl:value-of select="mods:title"/>
            </xsl:when>
            <xsl:when test="@type='alternative' or @type='abbreviated' or @type='translated' or @type='uniform'">
                <!-- multiple titles, don't use type='alternative' -->
            </xsl:when>
            <xsl:otherwise> 
                <xsl:text>Unknown entity name</xsl:text>
            </xsl:otherwise >
        </xsl:choose>
    </xsl:template>
    
    
</xsl:stylesheet>
