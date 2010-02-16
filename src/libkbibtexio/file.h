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

#ifndef KBIBTEX_IO_FILE_H
#define KBIBTEX_IO_FILE_H

#include <QList>
#include <QStringList>

#include <element.h>

#include "kbibtexio_export.h"

namespace KBibTeX
{
namespace IO {

class Element;

/**
 * This class represents a bibliographic file such as a BibTeX
 * (or any other format after proper conversion). The files content
 * can be accessed using the inherited QList interface (for example
 * list iterators).
 * @see Element
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT File : public QList<KBibTeX::IO::Element*>
{
public:

    File();
    virtual ~File();

    /**
     * Check if a given key,for example a key for a macro or an id for an entry
     * is contained in the file object.
     * @see #allKeys() const
     * @return @c the object addressed by the key @c NULL if no such file has been found
     */
    const Element *containsKey(const QString &key) const;

    /**
     * Retrieves a list of all keys for example from macros or entries.
     * @see #const containsKey(const QString &) const
     * @return list of keys
     */
    QStringList allKeys() const;
};

}
}

#endif // KBIBTEX_IO_FILE_H
