/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#ifndef KBIBTEX_GUI_FIELDLINEEDIT_H
#define KBIBTEX_GUI_FIELDLINEEDIT_H

#include <kbibtexgui_export.h>

#include <KIcon>

#include <value.h>
#include <menulineedit.h>
#include <kbibtexnamespace.h>

class QMenu;
class QSignalMapper;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT FieldLineEdit : public MenuLineEdit
{
    Q_OBJECT

public:
    FieldLineEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, bool isMultiLine = false, QWidget *parent = NULL);

    void reset(const Value& value);
    void apply(Value& value) const;

private:
    bool m_incompleteRepresentation;


    KBibTeX::TypeFlag typeFlag();
    KBibTeX::TypeFlag setTypeFlag(KBibTeX::TypeFlag typeFlag);

    void setupMenu();
    void updateGUI();

    class FieldLineEditPrivate;
    FieldLineEdit::FieldLineEditPrivate *d;

private slots:
    void slotTypeChanged(int);
};

#endif // KBIBTEX_GUI_FIELDLINEEDIT_H
