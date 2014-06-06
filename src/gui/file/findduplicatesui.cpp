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

#include "findduplicatesui.h"

#include <QApplication>
#include <QWidget>
#include <QBoxLayout>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QRadioButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSplitter>
#include <QtCore/QPointer>

#include <KPushButton>
#include <KAction>
#include <KDialog>
#include <KActionCollection>
#include <KLocale>
#include <KXMLGUIClient>
#include <KStandardDirs>
#include <kparts/part.h>
#include <KMessageBox>
#include <KDebug>
#include <KLineEdit>

#include <kdeversion.h>

#include "fileimporterbibtex.h"
#include "bibtexentries.h"
#include "radiobuttontreeview.h"
#include "fileview.h"
#include "filemodel.h"
#include "findduplicates.h"

const int FieldNameRole = Qt::UserRole + 101;
const int UserInputRole = Qt::UserRole + 103;

const int maxFieldsCount = 1024;

/**
 * Model to hold alternative values as visualized in the RadioTreeView
 */
class AlternativesItemModel : public QAbstractItemModel
{
private:
    /// marker to memorize in an index's internal id that it is a top-level index
    static const quint32 noParentInternalId;

    /// parent widget, needed to get font from (for text in italics)
    QTreeView *p;

    EntryClique *currentClique;

public:
    enum SelectionType {SelectionTypeNone, SelectionTypeRadio, SelectionTypeCheck};

    AlternativesItemModel(QTreeView *parent)
            : QAbstractItemModel(parent), p(parent), currentClique(NULL) {
        // nothing
    }

    static SelectionType selectionType(const QString &fieldName) {
        if (fieldName.isEmpty())
            return SelectionTypeNone;
        if (fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl)
            return SelectionTypeCheck;
        return SelectionTypeRadio;
    }

    void setCurrentClique(EntryClique *currentClique) {
        this->currentClique = currentClique;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const {
        if (parent == QModelIndex())
            return createIndex(row, column, noParentInternalId);
        else if (parent.parent() == QModelIndex())
            return createIndex(row, column, parent.row());
        return QModelIndex();
    }

    QModelIndex parent(const QModelIndex &index) const {
        if (index.internalId() >= noParentInternalId)
            return QModelIndex();
        else
            return createIndex(index.internalId(), 0, noParentInternalId);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        if (currentClique == NULL)
            return 0;

        if (parent == QModelIndex()) {
            /// top-level index, check how many lists of lists of alternatives exist
            return currentClique->fieldCount();
        } else if (parent.parent() == QModelIndex()) {
            /// first, find the map of alternatives for this chosen field name (see parent)
            QString fieldName = parent.data(FieldNameRole).toString();
            QList<Value> alt = currentClique->values(fieldName);
            /// second, return number of alternatives for list of alternatives
            /// plus one for an "else" option
            return alt.count() + (fieldName.startsWith('^') || fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl ? 0 : 1);
        }

        return 0;
    }

    int columnCount(const QModelIndex &) const {
        /// only one column in use
        return 1;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const {
        Q_UNUSED(section)
        Q_UNUSED(orientation)

        if (role == Qt::DisplayRole)
            return i18n("Alternatives");
        return QVariant();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        if (index.parent() == QModelIndex()) {
            /// top-level elements showing field names like "Title", "Authors", etc
            const QString fieldName = currentClique->fieldList().at(index.row()).toLower();
            switch (role) {
            case FieldNameRole:
                /// plain-and-simple field name (all lower case)
                return fieldName;
            case Qt::ToolTipRole:
            case Qt::DisplayRole:
                /// nicely formatted field names for visual representation
                if (fieldName == QLatin1String("^id"))
                    return i18n("Identifier");
                else if (fieldName == QLatin1String("^type"))
                    return i18n("Type");
                else
                    return BibTeXEntries::self()->format(fieldName, KBibTeX::cUpperCamelCase);
            case IsRadioRole:
                /// this is not to be a radio widget
                return QVariant::fromValue(false);
            case Qt::FontRole:
                if (fieldName.startsWith('^')) {
                    QFont f = p->font();
                    f.setItalic(true);
                    return f;
                }
            }
        } else if (index.parent().parent() == QModelIndex()) {
            /// second-level entries for alternatives

            /// start with determining which list of alternatives actually to use
            QString fieldName = index.parent().data(FieldNameRole).toString();
            QList<Value> values = currentClique->values(fieldName);

            switch (role) {
            case Qt::EditRole:
            case Qt::ToolTipRole:
            case Qt::DisplayRole:
                if (index.row() < values.count()) {
                    QString text = PlainTextValue::text(values.at(index.row()));
                    if (fieldName == QLatin1String("^type")) {
                        BibTeXEntries *be = BibTeXEntries::self();
                        text = be->format(text, KBibTeX::cUpperCamelCase);
                    }

                    /// textual representation of the alternative's value
                    return text;
                } else
                    /// add an "else" option
                    return i18n("None of the above");
            case Qt::FontRole:
                /// for the "else" option, make font italic
                if (index.row() >= values.count()) {
                    QFont f = p->font();
                    f.setItalic(true);
                    return f;
                }
            case Qt::CheckStateRole: {
                if (selectionType(fieldName) != SelectionTypeCheck)
                    return QVariant();

                QList<Value> chosenValues = currentClique->chosenValues(fieldName);
                QString text = PlainTextValue::text(values.at(index.row()));
                foreach(const Value &value, chosenValues) {
                    if (PlainTextValue::text(value) == text)
                        return Qt::Checked;
                }

                return Qt::Unchecked;
            }
            case RadioSelectedRole: {
                if (selectionType(fieldName) != SelectionTypeRadio)
                    return QVariant::fromValue(false);

                /// return selection status (true or false) for this alternative
                Value chosen = currentClique->chosenValue(fieldName);
                if (chosen.isEmpty())
                    return QVariant::fromValue(index.row() >= values.count());
                else if (index.row() < values.count()) {
                    QString chosenPlainText = PlainTextValue::text(chosen);
                    QString rowPlainText = PlainTextValue::text(values[index.row()]);
                    return QVariant::fromValue(chosenPlainText == rowPlainText);
                }
                return QVariant::fromValue(false);
            }
            case IsRadioRole:
                /// this is to be a radio widget
                return QVariant::fromValue(selectionType(fieldName) == SelectionTypeRadio);
            }
        }

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = RadioSelectedRole) {
        if (index.parent() != QModelIndex()) {
            bool isInt;
            int checkState = value.toInt(&isInt);

            const QString fieldName = index.parent().data(FieldNameRole).toString();
            QList<Value> &values = currentClique->values(fieldName);

            if (role == RadioSelectedRole && value.canConvert<bool>() && value.toBool() == true && selectionType(fieldName) == SelectionTypeRadio) {
                /// start with determining which list of alternatives actually to use

                /// store which alternative was selected
                if (index.row() < values.count())
                    currentClique->setChosenValue(fieldName, values[index.row()]);
                else {
                    Value v;
                    currentClique->setChosenValue(fieldName, v);
                }

                /// update view on neighbouring alternatives
                emit dataChanged(index.sibling(0, 0), index.sibling(rowCount(index.parent()), 0));

                return true;
            } else if (role == Qt::CheckStateRole && isInt && selectionType(fieldName) == SelectionTypeCheck) {
                if (checkState == Qt::Checked)
                    currentClique->setChosenValue(fieldName, values[index.row()], EntryClique::AddValue);
                else if (checkState == Qt::Unchecked)
                    currentClique->setChosenValue(fieldName, values[index.row()], EntryClique::RemoveValue);
                else
                    return false; ///< tertium non datur

                emit dataChanged(index, index);

                return true;
            } else if (role == UserInputRole) {
                const QString text = value.toString();
                if (text.isEmpty()) return false;
                const Value old = values.at(index.row());
                if (old.isEmpty()) return false;

                Value v;

                QSharedPointer<PlainText> pt = old.first().dynamicCast<PlainText>();
                if (!pt.isNull())
                    v.append(QSharedPointer<PlainText>(new PlainText(text)));
                else {
                    QSharedPointer<VerbatimText> vt = old.first().dynamicCast<VerbatimText>();
                    if (!vt.isNull())
                        v.append(QSharedPointer<VerbatimText>(new VerbatimText(text)));
                    else {
                        QSharedPointer<MacroKey> mk = old.first().dynamicCast<MacroKey>();
                        if (!mk.isNull())
                            v.append(QSharedPointer<MacroKey>(new MacroKey(text)));
                        else {
                            QSharedPointer<Person> ps = old.first().dynamicCast<Person>();
                            if (!ps.isNull())
                                FileImporterBibTeX::parsePersonList(text, v);
                            else {
                                QSharedPointer<Keyword> kw = old.first().dynamicCast<Keyword>();
                                if (!kw.isNull()) {
                                    QList<QSharedPointer<Keyword> > keywordList = FileImporterBibTeX::splitKeywords(text);
                                    for (QList<QSharedPointer<Keyword> >::ConstIterator it = keywordList.constBegin(); it != keywordList.constEnd(); ++it)
                                        v.append(*it);
                                } else {
                                    kDebug() << "Not know how to set this text:" << text;
                                }
                            }
                        }
                    }
                }

                if (!v.isEmpty()) {
                    values.removeAt(index.row());
                    values.insert(index.row(), v);
                    emit dataChanged(index, index);
                    return true;
                } else
                    return false;
            }
        }

        return false;
    }

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const {
        /// depth-two tree
        return parent == QModelIndex() || parent.parent() == QModelIndex();
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const {
        Qt::ItemFlags f = QAbstractItemModel::flags(index);
        if (index.parent() != QModelIndex()) {
            QString fieldName = index.parent().data(FieldNameRole).toString();
            if (selectionType(fieldName) == SelectionTypeCheck)
                f |= Qt::ItemIsUserCheckable;

            QList<Value> values = currentClique->values(fieldName);
            if (index.row() < values.count())
                f |= Qt::ItemIsEditable;
        }
        return f;
    }
};

const quint32 AlternativesItemModel::noParentInternalId = 0xffffff;


/**
 * Specialization of RadioButtonItemDelegate which allows to edit
 * values in a AlternativesItemModel.
 */
class AlternativesItemDelegate: public RadioButtonItemDelegate
{
public:
    AlternativesItemDelegate(QObject *p)
            : RadioButtonItemDelegate(p) {
        // nothing
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const {
        if (index.parent() != QModelIndex()) {
            /// Only second-level indices in the model can be edited
            /// (those are the actual values).
            /// Use a plain, border-less KLineEdit.
            KLineEdit *lineEdit = new KLineEdit(parent);
            lineEdit->setStyleSheet(QLatin1String("border: none;"));
            return lineEdit;
        }
        return NULL;
    }

    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const {
        if (KLineEdit *lineEdit = qobject_cast<KLineEdit *>(editor)) {
            /// Set line edit's default value to string fetched from model
            lineEdit->setText(index.data(Qt::EditRole).toString());
        }
    }

    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
        if (KLineEdit *lineEdit = qobject_cast<KLineEdit *>(editor)) {
            /// Set user-entered text to model (and underlying value)
            model->setData(index, lineEdit->text(), UserInputRole);

            /// Ensure that the edited value is checked if it is
            /// a checkbox-checkable value, or gets a "dot" in its
            /// radio button if it is radio-checkable.
            const QString fieldName = index.parent().data(FieldNameRole).toString();
            if (AlternativesItemModel::selectionType(fieldName) == AlternativesItemModel::SelectionTypeCheck)
                model->setData(index, Qt::Checked, Qt::CheckStateRole);
            else if (AlternativesItemModel::selectionType(fieldName) == AlternativesItemModel::SelectionTypeRadio)
                model->setData(index, true, RadioSelectedRole);
        }
    }

    virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const {
        QRect rect = option.rect;

        // TODO better placement of editing widget?
        //int radioButtonWidth = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &option);
        //int spacing = QApplication::style()->pixelMetric(QStyle::PM_RadioButtonLabelSpacing, &option);
        //rect.setLeft(rect.left() +spacing*3/2 + radioButtonWidth);

        editor->setGeometry(rect);
    }
};


class CheckableFileModel : public FileModel
{
private:
    QList<EntryClique *> cl;
    int currentClique;
    QTreeView *tv;

public:
    CheckableFileModel(QList<EntryClique *> &cliqueList, QTreeView *treeView, QObject *parent = NULL)
            : FileModel(parent), cl(cliqueList), currentClique(0), tv(treeView) {
        // nothing
    }

    void setCurrentClique(EntryClique *currentClique) {
        this->currentClique = cl.indexOf(currentClique);
    }

    virtual QVariant data(const QModelIndex &index, int role) const {
        if (role == Qt::CheckStateRole && index.column() == 1) {
            QSharedPointer<Entry> entry = element(index.row()).dynamicCast<Entry>();
            Q_ASSERT_X(!entry.isNull(), "CheckableFileModel::data", "entry is NULL");
            if (!entry.isNull()) {
                QList<QSharedPointer<Entry> > entryList = cl[currentClique]->entryList();
                if (entryList.contains(QSharedPointer<Entry>(entry))) // TODO does this work?
                    return cl[currentClique]->isEntryChecked(QSharedPointer<Entry>(entry)) ? Qt::Checked : Qt::Unchecked;
            }
        }

        return FileModel::data(index, role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {
        bool ok;
        int checkState = value.toInt(&ok);
        Q_ASSERT_X(ok, "CheckableFileModel::setData", QString("Could not convert value " + value.toString()).toLatin1());
        if (ok && role == Qt::CheckStateRole && index.column() == 1) {
            QSharedPointer<Entry> entry = element(index.row()).dynamicCast<Entry>();
            if (!entry.isNull()) {
                QList<QSharedPointer<Entry> > entryList = cl[currentClique]->entryList();
                if (entryList.contains(QSharedPointer<Entry>(entry))) { // TODO does this work?
                    EntryClique *ec = cl[currentClique];
                    ec->setEntryChecked(QSharedPointer<Entry>(entry), checkState == Qt::Checked);
                    cl[currentClique] = ec;
                    emit dataChanged(index, index);
                    tv->reset();
                    tv->expandAll();
                    return true;
                }
            }
        }

        return false;
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const {
        Qt::ItemFlags f = FileModel::flags(index);
        if (index.column() == 1)
            f |= Qt::ItemIsUserCheckable;
        return f;
    }
};


class FilterIdFileModel : public QSortFilterProxyModel
{
private:
    CheckableFileModel *internalModel;
    EntryClique *currentClique;

public:
    FilterIdFileModel(QObject *parent = NULL)
            : QSortFilterProxyModel(parent), internalModel(NULL), currentClique(NULL) {
        // nothing
    }

    void setCurrentClique(EntryClique *currentClique) {
        Q_ASSERT_X(internalModel != NULL, "FilterIdFileModel::setCurrentClique(EntryClique *currentClique)", "internalModel is NULL");
        internalModel->setCurrentClique(currentClique);
        this->currentClique = currentClique;
        invalidate();
    }

    void setSourceModel(QAbstractItemModel *model) {
        QSortFilterProxyModel::setSourceModel(model);
        internalModel = dynamic_cast<CheckableFileModel *>(model);
        Q_ASSERT_X(internalModel != NULL, "FilterIdFileModel::setSourceModel(QAbstractItemModel *model)", "internalModel is NULL");
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
        Q_UNUSED(source_parent)

        if (internalModel != NULL && currentClique != NULL) {
            QSharedPointer<Entry> entry = internalModel->element(source_row).dynamicCast<Entry>();
            if (!entry.isNull()) {
                QList<QSharedPointer<Entry> > entryList = currentClique->entryList();
                if (entryList.contains(QSharedPointer<Entry>(entry))) return true; // TODO does this work?
            }
        }
        return false;
    }
};


class MergeWidget::MergeWidgetPrivate
{
private:
    MergeWidget *p;

public:
    File *file;
    FileView *editor;
    KPushButton *buttonNext, *buttonPrev;
    QLabel *labelWhichClique;
    static const char *whichCliqueText;

    CheckableFileModel *model;
    FilterIdFileModel *filterModel;

    RadioButtonTreeView *alternativesView;
    AlternativesItemModel *alternativesItemModel;
    AlternativesItemDelegate *alternativesItemDelegate;

    int currentClique;
    QList<EntryClique *> cl;

    MergeWidgetPrivate(MergeWidget *parent, File *bibTeXFile, QList<EntryClique *> &cliqueList)
            : p(parent), file(bibTeXFile), currentClique(0), cl(cliqueList) {
        setupGUI();
    }

    void setupGUI() {
        p->setMinimumSize(p->fontMetrics().xHeight() * 96, p->fontMetrics().xHeight() * 64);
        p->setBaseSize(p->fontMetrics().xHeight() * 128, p->fontMetrics().xHeight() * 96);

        QBoxLayout *layout = new QVBoxLayout(p);

        QLabel *label = new QLabel(i18n("Select your duplicates"), p);
        layout->addWidget(label);

        QSplitter *splitter = new QSplitter(Qt::Vertical, p);
        layout->addWidget(splitter);

        editor = new FileView(QLatin1String("MergeWidget"), splitter);
        editor->setItemDelegate(new FileDelegate(editor));
        editor->setReadOnly(true);

        alternativesView = new RadioButtonTreeView(splitter);

        model = new CheckableFileModel(cl, alternativesView, p);
        model->setBibliographyFile(file);

        QBoxLayout *containerLayout = new QHBoxLayout();
        layout->addLayout(containerLayout);
        containerLayout->addStretch(10);
        labelWhichClique = new QLabel(p);
        containerLayout->addWidget(labelWhichClique);
        buttonPrev = new KPushButton(KIcon("go-previous"), i18n("Previous Clique"), p);
        containerLayout->addWidget(buttonPrev, 1);
        buttonNext = new KPushButton(KIcon("go-next"), i18n("Next Clique"), p);
        containerLayout->addWidget(buttonNext, 1);

        filterModel = new FilterIdFileModel(p);
        filterModel->setSourceModel(model);
        alternativesItemModel = new AlternativesItemModel(alternativesView);
        alternativesItemDelegate = new AlternativesItemDelegate(alternativesView);

        showCurrentClique();

        connect(buttonPrev, SIGNAL(clicked()), p, SLOT(previousClique()));
        connect(buttonNext, SIGNAL(clicked()), p, SLOT(nextClique()));

        connect(editor, SIGNAL(doubleClicked(QModelIndex)), editor, SLOT(viewCurrentElement()));
    }

    void showCurrentClique() {
        EntryClique *ec = cl[currentClique];
        filterModel->setCurrentClique(ec);
        alternativesItemModel->setCurrentClique(ec);
        editor->setModel(filterModel);
        alternativesView->setModel(alternativesItemModel);
        alternativesView->setItemDelegate(alternativesItemDelegate);
        editor->reset();
        alternativesView->reset();
        alternativesView->expandAll();

        buttonNext->setEnabled(currentClique >= 0 && currentClique < cl.count() - 1);
        buttonPrev->setEnabled(currentClique > 0);
        labelWhichClique->setText(i18n(whichCliqueText, currentClique + 1, cl.count()));
    }

};

const char *MergeWidget::MergeWidgetPrivate::whichCliqueText = "Showing clique %1 of %2.";

MergeWidget::MergeWidget(File *file, QList<EntryClique *> &cliqueList, QWidget *parent)
        : QWidget(parent), d(new MergeWidgetPrivate(this, file, cliqueList))
{
    // nothing
}

MergeWidget::~MergeWidget()
{
    delete d;
}

void MergeWidget::previousClique()
{
    if (d->currentClique > 0) {
        --d->currentClique;
        d->showCurrentClique();
    }
}

void MergeWidget::nextClique()
{
    if (d->currentClique >= 0 && d->currentClique < d->cl.count() - 1) {
        ++d->currentClique;
        d->showCurrentClique();
    }
}


class FindDuplicatesUI::FindDuplicatesUIPrivate
{
private:
    // UNUSED FindDuplicatesUI *p;

public:
    KParts::Part *part;
    FileView *view;

    FindDuplicatesUIPrivate(FindDuplicatesUI */* UNUSED parent*/, KParts::Part *kpart, FileView *fileView)
        : /* UNUSED p(parent),*/ part(kpart), view(fileView) {
        /// nothing
    }
};

FindDuplicatesUI::FindDuplicatesUI(KParts::Part *part, FileView *fileView)
        : QObject(), d(new FindDuplicatesUIPrivate(this, part, fileView))
{
    KAction *newAction = new KAction(KIcon("tab-duplicate"), i18n("Find Duplicates"), this);
    part->actionCollection()->addAction(QLatin1String("findduplicates"), newAction);
    connect(newAction, SIGNAL(triggered()), this, SLOT(slotFindDuplicates()));
#if KDE_IS_VERSION(4, 4, 0)
    part->replaceXMLFile(KStandardDirs::locate("data", "kbibtex/findduplicatesui.rc"), KStandardDirs::locateLocal("data", "kbibtex/findduplicatesui.rc"), true);
#endif
}

FindDuplicatesUI::~FindDuplicatesUI()
{
    delete d;
}

void FindDuplicatesUI::slotFindDuplicates()
{
    // FIXME move to settings
    //bool ok = false;
    //int sensitivity = KInputDialog::getInteger(i18n("Sensitivity"), i18n("Enter a value for sensitivity.\n\nLow values (close to 0) require very similar entries for duplicate detection, larger values (close to 10000) are more likely to count entries as duplicates.\n\nPlease provide feedback to the developers if you have a suggestion for a better default value than 4000."), 4000, 0, 10000, 10, &ok, d->part->widget());
    //if (!ok) sensitivity = 4000;
    int sensitivity = 4000;

    QPointer<KDialog> dlg = new KDialog(d->part->widget());
    FindDuplicates fd(dlg, sensitivity);
    /// File to be used to find duplicate in,
    /// may be only a subset of the original one if selection is used (see below)
    File *file = d->view->fileModel()->bibliographyFile();
    /// Full file, used to remove merged elements from
    /// Stays the same even when merging is restricted to selected elements
    File *originalFile = file;
    bool deleteFileLater = false;

    int rowCount = d->view->selectedElements().count() / d->view->model()->columnCount();
    if (rowCount > 1 && rowCount < d->view->model()->rowCount() && KMessageBox::questionYesNo(d->part->widget(), i18n("Multiple elements are selected. Do you want to search for duplicates only within the selection or in the whole document?"), i18n("Search only in selection?"), KGuiItem(i18n("Only in selection")), KGuiItem(i18n("Whole document"))) == KMessageBox::Yes) {
        QModelIndexList mil = d->view->selectionModel()->selectedRows();
        file = new File();
        deleteFileLater = true;
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
            file->append(d->view->fileModel()->element(d->view->sortFilterProxyModel()->mapToSource(*it).row()));
        }
    }

    QList<EntryClique *> cliques;
    bool gotCanceled = fd.findDuplicateEntries(file, cliques);
    if (gotCanceled) {
        if (deleteFileLater) delete file;
        delete dlg;
        return;
    }

    if (cliques.isEmpty()) {
        KMessageBox::information(d->part->widget(), i18n("No duplicates have been found."), i18n("No duplicates found"));
    } else {
        MergeWidget mw(d->view->fileModel()->bibliographyFile(), cliques, dlg);
        dlg->setMainWidget(&mw);

        if (dlg->exec() == QDialog::Accepted) {
            MergeDuplicates md(dlg);
            if (md.mergeDuplicateEntries(cliques, originalFile)) {
                d->view->fileModel()->reset();
                d->view->externalModification();
            }
        }

        while (!cliques.isEmpty()) {
            EntryClique *ec = cliques.first();
            cliques.removeFirst();
            delete ec;
        }
    }

    if (deleteFileLater) delete file;
    delete dlg;
}