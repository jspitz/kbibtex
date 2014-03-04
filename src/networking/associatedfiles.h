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

#ifndef KBIBTEX_NETWORKING_ASSOCIATEDFILES_H
#define KBIBTEX_NETWORKING_ASSOCIATEDFILES_H

#include <QUrl>

#include <KUrl>

#include "entry.h"

class QWidget;

class File;

#include "kbibtexnetworking_export.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT AssociatedFiles
{
public:
    enum PathType {ptAbsolute = 0, ptRelative = 1};
    enum RenameOperation {roKeepName = 0, roEntryId = 1, roUserDefined = 2};
    enum MoveCopyOperation {mcoNoCopyMove = 0, mcoCopy = 1, mcoMove = 2};

    static bool urlIsLocal(const QUrl &url);

    /**
     * Translate a given URL of a document (e.g. a PDF file) to a string
     * representation pointing to the relative location of this document.
     * A "base URL", i.e. the bibliography's file location has to provided to
     * calculate the relative location of the document.
     * "Upwards relativity" (i.e. paths containing "..") is not supported for this
     * functions output; in this case, an absolute path will be generated as fallback.
     * @param document The document's URL
     * @param baseUrl The base URL
     * @return The document URL's string representation relative to the base URL
     */
    static QString relativeFilename(const KUrl &document, const KUrl &baseUrl);
    /**
     * Translate a given URL of a document (e.g. a PDF file) to a string
     * representation pointing to the absolute location of this document.
     * A "base URL", i.e. the bibliography's file location may be provided to
     * resolve relative document URLs
     * @param document The document's URL
     * @param baseUrl The base URL
     * @return The document URL's string representation in absolute form
     */
    static QString absoluteFilename(const KUrl &document, const KUrl &baseUrl);

    static QString associateDocumentURL(const QUrl &document, QSharedPointer<Entry> &entry, const File *bibTeXFile, PathType pathType, const bool dryRun = false);
    static QString associateDocumentURL(const QUrl &document, const File *bibTeXFile, PathType pathType);
    static QUrl copyDocument(const QUrl &document, const QString &entryId, const File *bibTeXFile, RenameOperation renameOperation, MoveCopyOperation moveCopyOperation, QWidget *widget, const QString &userDefinedFilename = QString(), const bool dryRun = false);
};

#endif // KBIBTEX_NETWORKING_ASSOCIATEDFILES_H
