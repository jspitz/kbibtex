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

#ifndef KBIBTEX_GUI_ELEMENTEDITOR_H
#define KBIBTEX_GUI_ELEMENTEDITOR_H

#include "kbibtexgui_export.h"

#include <QWidget>

class Element;
class File;

/**
@author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT ElementEditor : public QWidget
{
    Q_OBJECT
public:
    class ApplyElementInterface
    {
    public:
        virtual ~ApplyElementInterface() {
            /** nothing */
        }
        virtual void apply(QSharedPointer<Element>) {
            /** nothing */
        }
    };

    ElementEditor(bool scrollable, QWidget *parent);
    ~ElementEditor();

    void setElement(QSharedPointer<Element> element, File *file);
    void setElement(QSharedPointer<const Element> element, File *file);
    void setReadOnly(bool isReadOnly = true);
    bool elementChanged();
    bool elementUnapplied();

    QWidget *currentPage() const;
    void setCurrentPage(QWidget *tab);

signals:
    void modified(bool);

public slots:
    void apply();
    void reset();

private slots:
    void tabChanged();
    void checkBibTeX();
    void childModified(bool);
    void updateReqOptWidgets();
    void limitKeyboardTabStops();

private:
    class ElementEditorPrivate;
    ElementEditorPrivate *d;
};

#endif // KBIBTEX_GUI_ELEMENTEDITOR_H
