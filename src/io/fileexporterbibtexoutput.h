/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/
#ifndef BIBTEXFILEEXPORTERBIBTEXOUTPUT_H
#define BIBTEXFILEEXPORTERBIBTEXOUTPUT_H

#include <QStringList>

#include "fileexportertoolchain.h"

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileExporterBibTeXOutput : public FileExporterToolchain
{
    Q_OBJECT

public:
    enum OutputType {BibTeXLogFile, BibTeXBlockList};
    explicit FileExporterBibTeXOutput(OutputType outputType, QObject *parent);
    ~FileExporterBibTeXOutput() override;

    void reloadConfig() override;

    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = nullptr) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = nullptr) override;

    void setLaTeXLanguage(const QString &language);
    void setLaTeXBibliographyStyle(const QString &bibStyle);

private:
    OutputType m_outputType;
    QString m_fileBasename;
    QString m_fileStem;
    QString m_latexLanguage;
    QString m_latexBibStyle;

    bool generateOutput(QStringList *errorLog);
    bool writeLatexFile(const QString &filename);
};

#endif
