/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef IO_FILEEXPORTERBIBUTILS_H
#define IO_FILEEXPORTERBIBUTILS_H

#include "fileexporter.h"
#include "bibutils.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileExporterBibUtils : public FileExporter, public BibUtils
{
public:
    explicit FileExporterBibUtils();
    ~FileExporterBibUtils();

    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = NULL);

private:
    class Private;
    Private *const d;
};

#endif // IO_FILEEXPORTERBIBUTILS_H