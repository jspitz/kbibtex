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

#include <typeinfo>

#include <KComboBox>

#include <QTreeView>
#include <QGridLayout>
#include <QStringListModel>
#include <QScrollBar>
#include <QTimer>

#include <bibtexfields.h>
#include <entry.h>
#include <bibtexeditor.h>
#include <valuelistmodel.h>
#include "valuelist.h"

class ValueList::ValueListPrivate
{
private:
    ValueList *p;

public:
    BibTeXEditor *editor;
    QTreeView *treeviewFieldValues;
    KComboBox *comboboxFieldNames;
    const int countWidth;

    ValueListPrivate(ValueList *parent)
            : p(parent), countWidth(parent->fontMetrics().width(QLatin1String("Count888"))) {
        setupGUI();
        initialize();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        comboboxFieldNames = new KComboBox(true, p);
        layout->addWidget(comboboxFieldNames, 0, 0, 1, 1);

        treeviewFieldValues = new QTreeView(p);
        layout->addWidget(treeviewFieldValues, 1, 0, 1, 1);
        treeviewFieldValues->setEditTriggers(QAbstractItemView::NoEditTriggers);

        p->setEnabled(false);

        connect(comboboxFieldNames, SIGNAL(activated(int)), p, SLOT(comboboxChanged()));
        connect(treeviewFieldValues, SIGNAL(activated(const QModelIndex &)), p, SLOT(listItemActivated(const QModelIndex &)));
    }

    void initialize() {
        const BibTeXFields *bibtexFields = BibTeXFields::self();

        comboboxFieldNames->clear();
        for (BibTeXFields::ConstIterator it = bibtexFields->constBegin(); it != bibtexFields->constEnd(); ++it) {
            FieldDescription fd = *it;
            if (!fd.upperCamelCaseAlt.isEmpty()) continue; /// keep only "single" fields and not combined ones like "Author or Editor"
            comboboxFieldNames->addItem(fd.label, fd.upperCamelCase);
        }
    }

    void comboboxChanged() {
        update();
    }

    void update() {
        QVariant var = comboboxFieldNames->itemData(comboboxFieldNames->currentIndex());
        QString text = var.toString();
        if (text.isEmpty()) text = comboboxFieldNames->currentText();

        treeviewFieldValues->setModel(editor == NULL ? NULL : editor->valueListModel(text));
    }
};

ValueList::ValueList(QWidget *parent)
        : QWidget(parent), d(new ValueListPrivate(this))
{
}

void ValueList::setEditor(BibTeXEditor *editor)
{
    d->editor = editor;
    d->update();
    setEnabled(d->editor != NULL);
    QTimer::singleShot(100, this, SLOT(resizeEvent()));
}

void ValueList::resizeEvent(QResizeEvent */*event*/)
{
    int widgetWidth = d->treeviewFieldValues->size().width() - d->treeviewFieldValues->verticalScrollBar()->size().width();
    d->treeviewFieldValues->setColumnWidth(0, widgetWidth - d->countWidth * 4 / 3);
    d->treeviewFieldValues->setColumnWidth(1, d->countWidth);
}

void ValueList::comboboxChanged()
{
    d->comboboxChanged();
}

void ValueList::listItemActivated(const QModelIndex &index)
{
    QString itemText = index.data(Qt::DisplayRole).toString();
    QVariant fieldVar = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex());
    QString fieldText = fieldVar.toString();
    if (fieldText.isEmpty()) fieldText = d->comboboxFieldNames->currentText();

    SortFilterBibTeXFileModel::FilterQuery fq;
    fq.terms << itemText;
    fq.combination = SortFilterBibTeXFileModel::EveryTerm;
    fq.field = fieldText;

    emit filterChanged(fq);
}
