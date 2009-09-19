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
#ifndef KBIBTEX_GUI_BIBTEXFILEVIEW_H
#define KBIBTEX_GUI_BIBTEXFILEVIEW_H

#include <QTreeView>

#include <kbibtexgui_export.h>

#include <element.h>

class QSignalMapper;

namespace KBibTeX
{
namespace GUI {
namespace Widgets {

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT BibTeXFileView : public QTreeView
{
    Q_OBJECT
public:
    BibTeXFileView(QWidget * parent = 0);
    virtual ~BibTeXFileView();

    const QList<KBibTeX::IO::Element*>& selectedElements() const;
    const KBibTeX::IO::Element* currentElement() const;

signals:
    void selectedElementsChanged();
    void currentElementChanged(const KBibTeX::IO::Element*);

protected:
    void resizeEvent(QResizeEvent *event);

protected slots:
    void currentChanged(const QModelIndex & current, const QModelIndex & previous);
    void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

private:
    QSignalMapper *m_signalMapperBibTeXFields;
    KBibTeX::IO::Element* m_current;
    QList<KBibTeX::IO::Element*> m_selection;

private slots:
    void headerActionToggled(QObject *action);
    void headerResetToDefaults();
};

}
}
}

#endif // KBIBTEX_GUI_BIBTEXFILEVIEW_H
