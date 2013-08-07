<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as generated by KBibTeX into nice HTML code.
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<xsl:template match="entry">
<xsl:text>{{cite </xsl:text>

<xsl:choose>
<xsl:when test="doi/text">
<xsl:text>doi|</xsl:text>
<xsl:value-of select="doi/text" />
</xsl:when>
<xsl:when test="jstor_id/text">
<xsl:text>jstor|</xsl:text>
<xsl:value-of select="jstor_id/text" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://www.jstor.org/stable/')">
<xsl:text>jstor|</xsl:text>
<xsl:value-of select="substring(url/text, 29, 9)" />
</xsl:when>
<xsl:when test="starts-with(@id, 'jstor')">
<xsl:text>jstor|</xsl:text>
<xsl:value-of select="substring(@id, 6, 9)" />
</xsl:when>
<xsl:when test="pubmed/text">
<xsl:text>pmid|</xsl:text>
<xsl:value-of select="pubmed/text" />
</xsl:when>
<xsl:when test="pmid/text">
<xsl:text>pmid|</xsl:text>
<xsl:value-of select="pmid/text" />
</xsl:when>
<xsl:when test="starts-with(@id, 'pmid')">
<xsl:text>pmid|</xsl:text>
<xsl:value-of select="substring(@id, 5)" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://hdl.handle.net/')">
<xsl:text>hdl|</xsl:text>
<xsl:value-of select="substring(url/text, 23)" />
</xsl:when>
<xsl:when test="isbn/text">
<xsl:text>isbn|</xsl:text>
<xsl:value-of select="isbn/text" />
</xsl:when>
<xsl:when test="starts-with(eprint/text, 'arXiv:')">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="substring(eprint/text, 7)" />
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="starts-with(arxivId/text, 'arXiv:')">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="substring(arxivId/text, 7)" />
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="arxivid/text">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="arxivid/text" />
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://arxiv.org/abs/') or starts-with(url/text, 'http://arxiv.org/pdf/')">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="substring(url/text, 21)" />
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="@type='article'">
<xsl:text>journal</xsl:text>
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="@type='book'">
<xsl:text>book</xsl:text>
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="@type='inproceedings' or @type='incollection'">
<xsl:text>conference</xsl:text>
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="@type='techreport'">
<xsl:text>conference</xsl:text>
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="@type='phdthesis' or @type='mastersthesis'">
<xsl:text>thesis</xsl:text>
<xsl:choose>
<xsl:when test="@type='phdthesis'"><xsl:text>|type=Ph.D.</xsl:text></xsl:when>
<xsl:when test="@type='mastersthesis'"><xsl:text>|type=M.Sc.</xsl:text></xsl:when>
</xsl:choose>
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:when test="url/text">
<xsl:text>web</xsl:text>
<xsl:text>|url=</xsl:text><xsl:value-of select="url/text" />
<xsl:text>|TODO</xsl:text>
</xsl:when>
<xsl:otherwise><!-- BEGIN unknown type of citation -->
<xsl:text>FIXME</xsl:text>
</xsl:otherwise><!-- END unknown type of citation -->
</xsl:choose>

<xsl:text>}}
</xsl:text>
</xsl:template>

<xsl:template match="bibliography">
<xsl:apply-templates select="entry" />
</xsl:template>


</xsl:stylesheet>
