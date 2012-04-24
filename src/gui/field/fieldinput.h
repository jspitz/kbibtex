/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include "kbibtexgui_export.h"

#include <QWidget>

#include "value.h"
#include "kbibtexnamespace.h"

class Element;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT FieldInput : public QWidget
{
    Q_OBJECT

public:
    FieldInput(KBibTeX::FieldInputType fieldInputType, KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent = NULL);
    ~FieldInput();

    bool reset(const Value &value);
    bool apply(Value &value) const;

    void clear();
    void setReadOnly(bool isReadOnly);

    void setFile(const File *file);
    void setElement(const Element *element);
    void setFieldKey(const QString &fieldKey);
    void setCompletionItems(const QStringList &items);

signals:
    void modified();

private slots:
    void setMonth(int month);
    void selectCrossRef();

private:
    class FieldInputPrivate;
    FieldInputPrivate *d;
};
