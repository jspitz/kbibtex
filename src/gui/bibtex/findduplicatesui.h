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

#ifndef KBIBTEX_GUI_FINDDUPLICATES_H
#define KBIBTEX_GUI_FINDDUPLICATES_H

#include "kbibtexgui_export.h"

#include <QObject>
#include <QTreeView>

namespace KParts
{
class Part;
}
class KXMLGUIClient;
class KPushButton;

class BibTeXEditor;
class EntryClique;
class File;

class RadioButtonTreeView;
class AlternativesItemModel;
class CheckableBibTeXFileModel;
class FilterIdBibTeXFileModel;

class KBIBTEXGUI_EXPORT MergeWidget : public QWidget
{
    Q_OBJECT

public:
    MergeWidget(File *file, QList<EntryClique*> &cliques, QWidget *parent);
    ~MergeWidget();

    void showCurrentClique();

private slots:
    void previousClique();
    void nextClique();

private:
    class MergeWidgetPrivate;
    MergeWidgetPrivate *d;
};


class KBIBTEXGUI_EXPORT FindDuplicatesUI : public QObject
{
    Q_OBJECT

public:
    FindDuplicatesUI(KParts::Part *part, BibTeXEditor *bibTeXEditor);
    ~FindDuplicatesUI();

private slots:
    void slotFindDuplicates();

private:
    class FindDuplicatesUIPrivate;
    FindDuplicatesUIPrivate *d;
};

#endif // KBIBTEX_GUI_FINDDUPLICATES_H
