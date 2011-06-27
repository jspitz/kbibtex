/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#include <QFile>
#include <QStringList>
#include <QUrl>
#include <QTextStream>

#include <KDebug>
#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>

#include <element.h>
#include <entry.h>
#include <fileexporterbibtex.h>
#include "fileexporterpdf.h"

FileExporterPDF::FileExporterPDF(bool embedFiles)
        : FileExporterToolchain(), m_embedFiles(embedFiles)
{
    m_laTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-pdf.tex");
    m_bibTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-pdf.bib");
    m_outputFilename = tempDir.name() + QLatin1String("/bibtex-to-pdf.pdf");

    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("kbibtexrc"));
    KConfigGroup configGroup(config, QLatin1String("FileExporterPDFPS"));
    m_babelLanguage = configGroup.readEntry(keyBabelLanguage, defaultBabelLanguage);
    m_bibliographyStyle = configGroup.readEntry(keyBibliographyStyle, defaultBibliographyStyle);

    KConfigGroup configGroupGeneral(config, QLatin1String("General"));
    m_paperSize = configGroupGeneral.readEntry(keyPaperSize, defaultPaperSize);
}

FileExporterPDF::~FileExporterPDF()
{
    // nothing
}

bool FileExporterPDF::save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog)
{
    bool result = false;
    m_embeddedFileList.clear();
    if (m_embedFiles) {
        m_embeddedFileList.append(QString("%1|%2").arg("BibTeX source").arg(m_bibTeXFilename));
        fillEmbeddedFileList(bibtexfile);
    }

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX* bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, bibtexfile, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generatePDF(iodevice, errorLog);

    return result;
}

bool FileExporterPDF::save(QIODevice* iodevice, const Element* element, QStringList *errorLog)
{
    bool result = false;
    m_embeddedFileList.clear();
    if (m_embedFiles)
        fillEmbeddedFileList(element);

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX * bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, element, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generatePDF(iodevice, errorLog);

    return result;
}

void FileExporterPDF::setDocumentSearchPaths(const QStringList& searchPaths)
{
    m_searchPaths = searchPaths;
}

bool FileExporterPDF::generatePDF(QIODevice* iodevice, QStringList *errorLog)
{
    QStringList cmdLines = QStringList() << QLatin1String("pdflatex -halt-on-error bibtex-to-pdf.tex") << QLatin1String("bibtex bibtex-to-pdf") << QLatin1String("pdflatex -halt-on-error bibtex-to-pdf.tex") << QLatin1String("pdflatex -halt-on-error bibtex-to-pdf.tex");

    return writeLatexFile(m_laTeXFilename) && runProcesses(cmdLines, errorLog) && writeFileToIODevice(m_outputFilename, iodevice, errorLog);
}

bool FileExporterPDF::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        m_embedFiles &= kpsewhich("embedfile.sty");
        QTextStream ts(&latexFile);
        ts.setCodec("UTF-8");
        ts << "\\documentclass{article}\n";
        ts << "\\usepackage[T1]{fontenc}\n";
        ts << "\\usepackage[utf8]{inputenc}\n";
        if (kpsewhich("babel.sty"))
            ts << "\\usepackage[" << m_babelLanguage << "]{babel}\n";
        if (kpsewhich("hyperref.sty"))
            ts << "\\usepackage[pdfproducer={KBibTeX: http://home.gna.org/kbibtex/},pdftex]{hyperref}\n";
        else if (kpsewhich("url.sty"))
            ts << "\\usepackage{url}\n";
        if (m_bibliographyStyle.startsWith("apacite") && kpsewhich("apacite.sty"))
            ts << "\\usepackage[bibnewpage]{apacite}\n";
        if (kpsewhich("embedfile.sty"))
            ts << "\\usepackage{embedfile}\n";
        if (kpsewhich("geometry.sty"))
            ts << "\\usepackage[paper=" << m_paperSize << (m_paperSize.length() <= 2 ? "paper" : "") << "]{geometry}\n";
        ts << "\\bibliographystyle{" << m_bibliographyStyle << "}\n";
        ts << "\\begin{document}\n";

        if (m_embedFiles) {
            ts << "\\embedfile[desc={" << i18n("BibTeX file") << "}]{bibtex-to-pdf.bib}\n";

            for (QStringList::ConstIterator it = m_embeddedFileList.begin(); it != m_embeddedFileList.end(); ++it) {
                QStringList param = (*it).split("|");
                QFile file(param[1]);
                if (file.exists())
                    ts << "\\embedfile[desc={" << param[0] << "}]{" << param[1] << "}\n";
            }
        }

        ts << "\\nocite{*}\n";
        ts << "\\bibliography{bibtex-to-pdf}\n";
        ts << "\\end{document}\n";
        latexFile.close();
        return true;
    } else
        return false;
}

void FileExporterPDF::fillEmbeddedFileList(const File* bibtexfile)
{
    for (File::ConstIterator it = bibtexfile->begin(); it != bibtexfile->end(); ++it)
        fillEmbeddedFileList(*it);
}

void FileExporterPDF::fillEmbeddedFileList(const Element* /*element*/)
{
    /* FIXME
    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL) {
        QString id = entry->id();
        QStringList urls = entry->urls();
        for (QStringList::Iterator it = urls.begin(); it != urls.end(); ++it) {
            QUrl url = QUrl(*it);
            if (url.isValid() && url.scheme() == "file" && !(*it).endsWith("/") && QFile(url.path()).exists())
                m_embeddedFileList.append(QString("%1|%2").arg(id).arg(url.path()));
            else
                for (QStringList::Iterator path_it = m_searchPaths.begin(); path_it != m_searchPaths.end(); ++path_it) {
                    url = QUrl(QString(*path_it).append("/").append(*it));
                    if (url.isValid() && url.scheme() == "file" && !(*it).endsWith("/") && QFile(url.path()).exists()) {
                        m_embeddedFileList.append(QString("%1|%2").arg(id).arg(url.path()));
                        break;
                    }
                }
        }
    }
    */
}
