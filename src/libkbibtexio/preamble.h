/***************************************************************************
 *   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#ifndef BIBTEXPreamble_H
#define BIBTEXPreamble_H

#include <element.h>

namespace KBibTeX
{
namespace IO {

/**
 @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
*/
class Preamble : public Element
{
public:
public:
    Preamble();
    Preamble(const Preamble *other);
    virtual ~Preamble();

    Value *value() const;
    void setValue(Value *value);

    bool containsPattern(const QString& pattern, EntryField::FieldType fieldType = EntryField::ftUnknown, FilterType filterType = Element::ftExact, Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive) const;

    Element* clone() const;
    void copyFrom(const Preamble *other);
    QString text() const;

private:
    Value *m_value;
};

}
}

#endif
