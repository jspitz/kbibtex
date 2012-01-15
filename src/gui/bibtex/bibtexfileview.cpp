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

#include <QHeaderView>
#include <QScrollBar>

#include <KAction>
#include <KConfigGroup>
#include <KLocale>
#include <KSharedConfig>

#include <bibtexfields.h>
#include "bibtexfilemodel.h"
#include "bibtexfileview.h"

class BibTeXFileView::BibTeXFileViewPrivate
{
private:
    BibTeXFileView *p;
    const int storedColumnCount;
    int *storedColumnWidths;
    bool *storedColumnVisible;

public:
    QString name;
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configHeaderState;

    BibTeXFileModel *bibTeXFileModel;
    QSortFilterProxyModel *sortFilterProxyModel;

    BibTeXFileViewPrivate(const QString &n, BibTeXFileView *parent)
            : p(parent), storedColumnCount(BibTeXFields::self()->count()), name(n), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("BibTeXFileView")), configHeaderState(QLatin1String("HeaderState_%1")) {
        storedColumnWidths = new int[storedColumnCount];
        storedColumnVisible = new bool[storedColumnCount];
        int col = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            storedColumnVisible[col] = fd->defaultVisible;
            storedColumnWidths[col] = fd->defaultWidth;
            ++col;
        }
    }

    ~BibTeXFileViewPrivate() {
        delete[] storedColumnWidths;
        delete[] storedColumnVisible;
    }

    void resetColumnsToDefault() {
        int widgetWidth = p->size().width() - p->verticalScrollBar()->size().width() - 8;
        if (widgetWidth < 8) return; ///< widget is too narrow or not yet initialized

        int sum = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            if (fd->defaultVisible)
                sum += fd->defaultWidth;
        }

        int col = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            storedColumnWidths[col] = (fd->defaultWidth * widgetWidth / sum);
            storedColumnVisible[col] = fd->defaultVisible;
            p->setColumnHidden(col, !storedColumnVisible[col]);
            p->setColumnWidth(col, storedColumnWidths[col]);
            ++col;
        }
    }

    void storeColumns() {
        for (int col = storedColumnCount - 1; col >= 0; --col) {
            storedColumnWidths[col] = p->columnWidth(col);
            storedColumnVisible[col] = !p->isColumnHidden(col);
        }

        QByteArray headerState = p->header()->saveState();
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configHeaderState.arg(name), headerState);
        config->sync();
    }

    void restoreColumns() {
        int widgetWidth = p->size().width() - p->verticalScrollBar()->size().width() - 8;
        if (widgetWidth < 8) return; ///< widget is too narrow or not yet initialized

        int sum = 0;
        int col = 0;
        foreach(const FieldDescription *fd, *BibTeXFields::self()) {
            storedColumnWidths[col] = qMax(storedColumnWidths[col], fd->defaultWidth);
            if (storedColumnVisible[col])
                sum += storedColumnWidths[col];
            ++col;
        }

        for (int col = storedColumnCount - 1; col >= 0; --col) {
            storedColumnWidths[col] = storedColumnWidths[col] * widgetWidth / sum;
            p->setColumnHidden(col, !storedColumnVisible[col]);
            p->setColumnWidth(col, storedColumnWidths[col]);
        }
    }

    void setColumnVisible(int column, bool isVisible) {
        int widgetWidth = p->size().width() - p->verticalScrollBar()->size().width() - 8;
        widgetWidth = qMax(widgetWidth, 500);

        storedColumnVisible[column] = isVisible;
        if (isVisible)
            storedColumnWidths[column] = widgetWidth / 8;
    }

    bool isColumnVisible(int column) const {
        return storedColumnVisible[column];
    }
};

BibTeXFileView::BibTeXFileView(const QString &name, QWidget * parent)
        : QTreeView(parent), d(new BibTeXFileViewPrivate(name, this))
{
    /// general visual appearance and behaviour
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameStyle(QFrame::NoFrame);
    setAlternatingRowColors(true);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);

    /// header appearance and behaviour
    header()->setClickable(true);
    header()->setSortIndicatorShown(true);
    header()->setSortIndicator(-1, Qt::AscendingOrder);
    connect(header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(sort(int, Qt::SortOrder)));
    header()->setContextMenuPolicy(Qt::ActionsContextMenu);

    /// restore header appearance
    KConfigGroup configGroup(d->config, d->configGroupName);
    QByteArray headerState = configGroup.readEntry(d->configHeaderState.arg(d->name), QByteArray());
    if (headerState.isEmpty())
        d->resetColumnsToDefault();
    else {
        header()->restoreState(headerState);
        d->storeColumns();
    }

    /// build context menu for header to show/hide single columns
    int col = 0;
    foreach(const FieldDescription *fd,  *BibTeXFields::self()) {
        KAction *action = new KAction(fd->label, header());
        action->setData(col);
        action->setCheckable(true);
        action->setChecked(d->isColumnVisible(col));
        connect(action, SIGNAL(triggered()), this, SLOT(headerActionToggled()));
        header()->addAction(action);
        ++col;
    }

    /// add separator to header's context menu
    KAction *action = new KAction(header());
    action->setSeparator(true);
    header()->addAction(action);

    /// add action to reset to defaults (regarding column visibility) to header's context menu
    action = new KAction(i18n("Reset to defaults"), header());
    connect(action, SIGNAL(triggered()), this, SLOT(headerResetToDefaults()));
    header()->addAction(action);
}

BibTeXFileView::~BibTeXFileView()
{
    delete d;
}

void BibTeXFileView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    d->sortFilterProxyModel = NULL;
    d->bibTeXFileModel = dynamic_cast<BibTeXFileModel*>(model);
    if (d->bibTeXFileModel == NULL) {
        d->sortFilterProxyModel = dynamic_cast<QSortFilterProxyModel*>(model);
        Q_ASSERT(d->sortFilterProxyModel != NULL);
        d->bibTeXFileModel = dynamic_cast<BibTeXFileModel*>(d->sortFilterProxyModel->sourceModel());
    }

    /// sort according to session
    if (header()->isSortIndicatorShown())
        sort(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

    Q_ASSERT(d->bibTeXFileModel != NULL);
}

BibTeXFileModel *BibTeXFileView::bibTeXModel()
{
    return d->bibTeXFileModel;
}

QSortFilterProxyModel *BibTeXFileView::sortFilterProxyModel()
{
    return d->sortFilterProxyModel;
}

void BibTeXFileView::resizeEvent(QResizeEvent *event)
{
    d->restoreColumns();
    QTreeView::resizeEvent(event);
}

void BibTeXFileView::columnResized(int column, int oldSize, int newSize)
{
    d->storeColumns();
    QTreeView::columnResized(column, oldSize, newSize);
}

void BibTeXFileView::headerActionToggled()
{
    KAction *action = static_cast<KAction*>(sender());
    bool ok = false;
    int col = (int)action->data().toInt(&ok);
    if (!ok) return;

    d->storeColumns();
    d->setColumnVisible(col, action->isChecked());
    d->restoreColumns();
}

void BibTeXFileView::headerResetToDefaults()
{
    d->resetColumnsToDefault();
}

void BibTeXFileView::sort(int t, Qt::SortOrder s)
{
    SortFilterBibTeXFileModel *sortedModel = dynamic_cast<SortFilterBibTeXFileModel*>(model());
    if (sortedModel != NULL)
        sortedModel->sort(t, s);
}
