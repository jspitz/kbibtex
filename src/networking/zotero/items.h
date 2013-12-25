/***************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_NETWORKING_ZOTERO_ITEMS_H
#define KBIBTEX_NETWORKING_ZOTERO_ITEMS_H

#include <QObject>
#include <QSharedPointer>

#include <KUrl>

#include "kbibtexnetworking_export.h"

class Element;

namespace Zotero
{

class API;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT Items : public QObject
{
    Q_OBJECT
public:
    Items(API *api, QObject *parent = NULL);

    void retrieveItems(const QString &collectionId);

signals:
    void foundElement(QSharedPointer<Element>);
    void stoppedSearch(int);

private:
    class Private;
    Private *const d;

private slots:
    void finishedFetchingItems();
};

} // end of namespace Zotero

#endif // KBIBTEX_NETWORKING_ZOTERO_ITEMS_H

