<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output encoding="UTF-8"/>


<!-- ==============================================================================
     Maintain original HTML tags
-->
<xsl:template match="a|abbr|acronym|address|applet|b|big|blockquote|br|cite|code|del|dfn|div|em|hr|i|kbd|p|param|pre|q|quote|samp|script|span|small|strike|strong|sub|sup|tt|var|button|fieldset|form|input|label|legend|object|option|optgroup|select|caption|col|colgroup|table|tbody|td|tfoot|th|thead|tr|dl|dd|dt|ol|ul|li|img|quote|quotation" xmlns:html="http://www.w3.org/1999/XSL/some">
<xsl:copy>
<xsl:copy-of select="@*" />
<xsl:apply-templates />
</xsl:copy>
</xsl:template>



<xsl:template match="text">
<xsl:apply-templates />
</xsl:template>

<xsl:template match="person">
<xsl:value-of select="lastname"/><xsl:if test="string-length(firstname)>0"><xsl:text>, </xsl:text>
<xsl:value-of select="substring(firstname,1,1)"/><xsl:text>.</xsl:text></xsl:if>
</xsl:template>

<xsl:template match="authors">
<xsl:if test="string-length(.)>0">
<xsl:for-each select="person">
<span style="font-weight:bold;">
<xsl:apply-templates select="."/>
</span>
<xsl:if test="position()!=last()"><xsl:text>, </xsl:text>
</xsl:if>
<xsl:if test="position()=last()-1 and @etal!='true'">
<xsl:text> and </xsl:text>
</xsl:if>
</xsl:for-each>
<xsl:if test="@etal='true'"><xsl:text> </xsl:text><i>et&#160;al.</i></xsl:if>
</xsl:if>
</xsl:template>

<xsl:template match="editors">
<xsl:for-each select="person">
<xsl:apply-templates /><xsl:if test="position()!=last()"><xsl:text>, </xsl:text>
</xsl:if>
<xsl:if test="position()=last()-1 and @etal!='true'"><xsl:text> and </xsl:text></xsl:if>
</xsl:for-each>
<xsl:if test="@etal='true'"><xsl:text> </xsl:text><i>et&#160;al.</i></xsl:if>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="title"><xsl:apply-templates /></xsl:template>

<xsl:template match="booktitle">
<i><xsl:apply-templates /></i>
</xsl:template>

<xsl:template match="school">
<xsl:apply-templates />
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="journal">
<span style="font-style:italic;"><xsl:apply-templates /></span>
<xsl:if test="string-length(../volume)>0">
<xsl:value-of select="../volume"/>
<xsl:if test="string-length(../number)>0">
<xsl:text>(</xsl:text>
<xsl:value-of select="../number"/>
<xsl:text>)</xsl:text>
</xsl:if>
</xsl:if>
</xsl:template>

<xsl:template match="institution">
<i><xsl:apply-templates /></i>
<xsl:if test="string-length(../number)>0">
<xsl:text> No. </xsl:text>
<xsl:value-of select="../number"/>
</xsl:if>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="publisher">
<xsl:apply-templates />
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="volume">
<xsl:text>volume </xsl:text>
<xsl:apply-templates />
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="edition">
<xsl:apply-templates />
<xsl:text> edition, </xsl:text>
</xsl:template>

<xsl:template match="pages">
<xsl:apply-templates /><xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="year">
<xsl:apply-templates />
</xsl:template>

<xsl:template match="note">
<xsl:text>, </xsl:text><xsl:apply-templates />
</xsl:template>

<xsl:template match="month">
<xsl:apply-templates />
</xsl:template>

<xsl:template match="abstract">
<br/><i>Abstract</i><xsl:text>: </xsl:text><xsl:apply-templates />
</xsl:template>

<xsl:template match="entry">
<div style="margin:0px; padding: 0.5ex; border-top: solid 0.25ex #c00;"><xsl:apply-templates select="authors" /></div>
<div style="margin:0px; padding: 0.5ex; background: #ec6; font-size: 125%;">
<xsl:apply-templates select="title" />
</div>
<xsl:if test="string-length(journal)>0 or string-length(booktitle)>0 or string-length(year)>0">
<div style="margin:0px; padding: 0.5ex; background: #900; color: white;">
<xsl:if test="string-length(journal)>0"><xsl:apply-templates select="journal" /></xsl:if>
<xsl:if test="string-length(booktitle)>0"><xsl:apply-templates select="booktitle" /></xsl:if>
<xsl:if test="( string-length(journal)>0 or string-length(booktitle)>0 ) and string-length(year)>0"><xsl:text>, </xsl:text></xsl:if>
<xsl:if test="string-length(year)>0">
<xsl:if test="string-length(month)>0"><xsl:apply-templates select="month" /><xsl:text> </xsl:text></xsl:if>
<xsl:apply-templates select="year" />
</xsl:if>
</div>
</xsl:if>
<xsl:if test="string-length(abstract)>0">
<div style="margin:0px; padding: 0.5ex; padding-top: 0ex; text-align: justify; font-size: 80%; color: #333;">
<xsl:apply-templates select="abstract" />
</div>
</xsl:if>
<!--
<p>
<xsl:apply-templates select="authors" />
<xsl:apply-templates select="title" />
<xsl:apply-templates select="booktitle" />
<xsl:apply-templates select="journal" />
<xsl:apply-templates select="school" />
<xsl:apply-templates select="volume" />
<xsl:apply-templates select="edition" />
<xsl:apply-templates select="publisher" />
<xsl:apply-templates select="institution" />
<xsl:apply-templates select="pages" />
<xsl:apply-templates select="editors" />
<xsl:apply-templates select="month" />
<xsl:apply-templates select="year" />
<xsl:apply-templates select="note" />
<xsl:apply-templates select="abstract" />
</p>
-->
</xsl:template>

<xsl:template match="bibliography">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>Bibliography</title>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
</head>
<body style="margin:0px; padding: 0px;">
<xsl:apply-templates select="entry" />
</body>
</html>
</xsl:template>

</xsl:stylesheet>
