/***************************************************************************
*   Copyright (C) 2004-2006 by Thomas Fischer                             *
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
#ifndef BIBTEXFILEEXPORTERRTF_H
#define BIBTEXFILEEXPORTERRTF_H

#include <fileexportertoolchain.h>

class QTextStream;

namespace KBibTeX
{
namespace IO {

/**
@author Thomas Fischer
*/
class FileExporterRTF : public FileExporterToolchain
{
public:
    FileExporterRTF(const QString& latexBibStyle = "plain", const QString& latexLanguage = "english");
    ~FileExporterRTF();

    bool save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice* iodevice, const Element* element, QStringList *errorLog = NULL);

private:
    QString laTeXFilename;
    QString bibTeXFilename;
    QString outputFilename;
    QString m_latexLanguage;
    QString m_latexBibStyle;

    bool generateRTF(QIODevice* iodevice, QStringList *errorLog);
    bool writeLatexFile(const QString &filename);
};

}
}

#endif
