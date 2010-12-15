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

#include <QDropEvent>

#include <KDialog>
#include <KLocale>
#include <KDebug>

#include <elementeditor.h>
#include <entry.h>
#include <macro.h>
#include <bibtexfilemodel.h>
#include <fileexporterbibtex.h>
#include "valuelistmodel.h"
#include "bibtexeditor.h"

BibTeXEditor::BibTeXEditor(QWidget *parent)
        : BibTeXFileView(parent), m_isReadOnly(false), m_current(NULL)
{
    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
}

void BibTeXEditor::setModel(QAbstractItemModel * model)
{
    BibTeXFileView::setModel(model);
}

void BibTeXEditor::viewCurrentElement()
{
    viewElement(currentElement());
}

void BibTeXEditor::viewElement(const Element *element)
{
    Q_ASSERT_X(element->uniqueId % 1000 == 42, "void BibTeXEditor::editElement(Element *element)", "Invalid Element passed as argument");

    KDialog dialog(this);
    ElementEditor elementEditor(element, bibTeXModel()->bibTeXFile(), &dialog);
    elementEditor.setReadOnly(true);
    dialog.setCaption(i18n("View Element"));
    dialog.setMainWidget(&elementEditor);
    dialog.setButtons(KDialog::Close);
    elementEditor.reset();
    dialog.exec();
}

void BibTeXEditor::editCurrentElement()
{
    editElement(currentElement());
}

void BibTeXEditor::editElement(Element *element)
{
    if (isReadOnly()) {
        /// read-only forbids editing elements, calling viewElement instead
        viewElement(element);
        return;
    }

    KDialog dialog(this);
    ElementEditor elementEditor(element, bibTeXModel()->bibTeXFile(), &dialog);
    dialog.setCaption(i18n("Edit Element"));
    dialog.setMainWidget(&elementEditor);
    dialog.setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel | KDialog::Reset);
    dialog.enableButton(KDialog::Apply, false);

    connect(&elementEditor, SIGNAL(modified(bool)), &dialog, SLOT(enableButtonApply(bool)));
    connect(&dialog, SIGNAL(applyClicked()), &elementEditor, SLOT(apply()));
    connect(&dialog, SIGNAL(okClicked()), &elementEditor, SLOT(apply()));
    connect(&dialog, SIGNAL(resetClicked()), &elementEditor, SLOT(reset()));

    dialog.exec();

    if (elementEditor.isModified()) {
        emit currentElementChanged(currentElement(), bibTeXModel()->bibTeXFile());
        emit modified();
    }
}

const QList<Element*>& BibTeXEditor::selectedElements() const
{
    return m_selection;
}

void BibTeXEditor::setSelectedElements(QList<Element*> &list)
{
    m_selection = list;

    QItemSelectionModel *selModel = selectionModel();
    selModel->clear();
    for (QList<Element*>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        int row = bibTeXModel()->row(*it);
        QModelIndex idx = model()->index(row, 0);
        selModel->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
    }
}

void BibTeXEditor::setSelectedElement(Element* element)
{
    m_selection.clear();
    m_selection << element;

    QItemSelectionModel *selModel = selectionModel();
    selModel->clear();
    int row = bibTeXModel()->row(element);
    QModelIndex idx = bibTeXModel()->index(row, 0);
    selModel->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
}

const Element* BibTeXEditor::currentElement() const
{
    return m_current;
}

Element* BibTeXEditor::currentElement()
{
    return m_current;
}

void BibTeXEditor::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    QTreeView::currentChanged(current, previous); // FIXME necessary?

    m_current = bibTeXModel()->element(sortFilterProxyModel()->mapToSource(current).row());
    emit currentElementChanged(m_current, bibTeXModel()->bibTeXFile());
}

void BibTeXEditor::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    QModelIndexList set = selected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.append(bibTeXModel()->element((*it).row()));
    }
    if (m_current == NULL && !set.isEmpty())
        m_current = bibTeXModel()->element(set.first().row());

    set = deselected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.removeOne(bibTeXModel()->element((*it).row()));
    }


    emit selectedElementsChanged();
}

void BibTeXEditor::selectionDelete()
{
    QModelIndexList mil = selectionModel()->selectedRows();
    while (mil.begin() != mil.end()) {
        bibTeXModel()->removeRow(mil.begin()->row());
        mil.removeFirst();
    }
    emit modified();
}

void BibTeXEditor::externalModification()
{
    emit modified();
}

void BibTeXEditor::setReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
}

bool BibTeXEditor::isReadOnly() const
{
    return m_isReadOnly;
}

ValueListModel *BibTeXEditor::valueListModel(const QString &field)
{
    BibTeXFileModel *bibteXModel = bibTeXModel();
    if (bibteXModel != NULL)
        return new ValueListModel(bibteXModel->bibTeXFile(), field, this);

    return NULL;
}

void BibTeXEditor::mouseMoveEvent(QMouseEvent *event)
{
    emit editorMouseEvent(event);
}

void BibTeXEditor::dragEnterEvent(QDragEnterEvent *event)
{
    emit editorDragEnterEvent(event);
}

void BibTeXEditor::dropEvent(QDropEvent *event)
{
    if (event->source() != this)
        emit editorDropEvent(event);
}

void BibTeXEditor::dragMoveEvent(QDragMoveEvent *event)
{
    emit editorDragMoveEvent(event);
}

void BibTeXEditor::itemActivated(const QModelIndex & index)
{
    emit elementExecuted(bibTeXModel()->element(sortFilterProxyModel()->mapToSource(index).row()));
}
