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

#include <QGridLayout>
#include <QListView>
#include <QAbstractListModel>
#include <QUrl>
#include <QPainter>
#include <QToolButton>
#include <QApplication>
#include <QAction>
#include <QButtonGroup>
#include <QRadioButton>
#include <QLabel>

#include <KDialog>
#include <KLocale>
#include <KDebug>
#include <KSqueezedTextLabel>
#include <KMenu>
#include <KPushButton>
#include <KMimeType>
#include <KRun>
#include <KTemporaryFile>
#include <KFileDialog>
#include <KIO/NetAccess>

#include <fieldlistedit.h>
#include "findpdfui.h"
#include "fileinfo.h"

class PDFListModel;

const int posLabelUrl = 0;
const int posLabelPreview = 1;
const int posViewButton = 2;
const int posRadioNoDownload = 3;
const int posRadioDownload = 4;
const int posRadioURLonly = 5;

const int URLRole = Qt::UserRole + 1234;
const int TextualPreviewRole = Qt::UserRole + 1237;
const int TempFileNameRole = Qt::UserRole + 1236;
const int DownloadModeRole = Qt::UserRole + 1235;

/// inspired by KNewStuff3's ItemsViewDelegate
PDFItemDelegate::PDFItemDelegate(QListView *itemView, QObject *parent)
        : KWidgetItemDelegate(itemView, parent), m_parent(itemView)
{
    // nothing
}

void PDFItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    painter->save();

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlightedText().color()));
    } else {
        painter->setPen(QPen(option.palette.text().color()));
    }

    /// draw icon based on mime-type
    QPixmap icon = index.data(Qt::DecorationRole).value<QPixmap>();
    if (!icon.isNull()) {
        int margin = option.fontMetrics.height() / 3;
        painter->drawPixmap(margin, margin + option.rect.top(), KIconLoader::SizeMedium, KIconLoader::SizeMedium, icon);
    }

    painter->restore();
}

QList<QWidget *> PDFItemDelegate::createItemWidgets() const
{
    QList<QWidget *> list;

    /// first, the label with shows the found PDF file's origin (URL)
    KSqueezedTextLabel *label = new KSqueezedTextLabel();
    label->setBackgroundRole(QPalette::NoRole);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    list << label;
    Q_ASSERT(list.count() == posLabelUrl + 1);

    /// a label with shows either the PDF's title or a text snipplet
    QLabel *previewLabel = new QLabel();
    previewLabel->setBackgroundRole(QPalette::NoRole);
    previewLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    list << previewLabel;
    Q_ASSERT(list.count() == posLabelPreview + 1);

    /// add a push button to view the PDF file
    KPushButton *pushButton = new KPushButton(KIcon("application-pdf"), i18n("View"));
    list << pushButton;
    connect(pushButton, SIGNAL(clicked()), this, SLOT(slotViewPDF()));
    Q_ASSERT(list.count() == posViewButton + 1);

    /// a button group to choose what to do with this particular PDF file
    QButtonGroup *bg = new QButtonGroup();

    /// button group's first choice: ignore file (discard it)
    QRadioButton *radioButton = new QRadioButton(i18n("Ignore"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, SIGNAL(toggled(bool)), this, SLOT(slotRadioNoDownloadToggled(bool)));
    Q_ASSERT(list.count() == posRadioNoDownload + 1);

    /// download this file and store it locally, user will be asked for "Save As"
    radioButton = new QRadioButton(i18n("Download"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, SIGNAL(toggled(bool)), this, SLOT(slotRadioDownloadToggled(bool)));
    Q_ASSERT(list.count() == posRadioDownload + 1);

    /// paste URL into BibTeX entry, no local copy is stored
    radioButton = new QRadioButton(i18n("Use URL only"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, SIGNAL(toggled(bool)), this, SLOT(slotRadioURLonlyToggled(bool)));
    Q_ASSERT(list.count() == posRadioURLonly + 1);

    return list;
}

/// update the widgets
void PDFItemDelegate::updateItemWidgets(const QList<QWidget *> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    if (!index.isValid()) return;

    const PDFListModel *model = qobject_cast<const PDFListModel *>(index.model());
    if (model == NULL) {
        kDebug() << "WARNING - INVALID MODEL!";
        return;
    }

    /// determine some variables used for layout
    int margin = option.fontMetrics.height() / 3;
    int buttonHeight = option.fontMetrics.height() * 7 / 4;
    int maxTextWidth = qMax(qMax(option.fontMetrics.width(i18n("Use URL only")), option.fontMetrics.width(i18n("Ignore"))), qMax(option.fontMetrics.width(i18n("Download")), option.fontMetrics.width(i18n("View"))));
    int buttonWidth = maxTextWidth * 3 / 2;
    int labelWidth = option.rect.width() - 3 * margin - KIconLoader::SizeMedium;
    int labelHeight = (option.rect.height() - 4 * margin - buttonHeight) / 2;

    /// setup label which will show the PDF file's URL
    KSqueezedTextLabel *label = qobject_cast<KSqueezedTextLabel *>(widgets[posLabelUrl]);
    if (label != NULL) {
        label->setText(index.data(Qt::DisplayRole).toString());
        label->move(margin * 2 + KIconLoader::SizeMedium, margin);
        label->resize(labelWidth, labelHeight);
    }

    /// setup label which will show the PDF's title or textual beginning
    QLabel *previewLabel = qobject_cast<QLabel *>(widgets[posLabelPreview]);
    if (previewLabel != NULL) {
        previewLabel->setText(index.data(TextualPreviewRole).toString());
        previewLabel->setToolTip(QLatin1String("<qt>") + previewLabel->text() + QLatin1String("</qt>"));
        previewLabel->move(margin * 2 + KIconLoader::SizeMedium, margin * 2 + labelHeight);
        previewLabel->resize(labelWidth, labelHeight);
    }

    /// setup the view button
    KPushButton *viewButton = qobject_cast<KPushButton *>(widgets[posViewButton]);
    if (viewButton != NULL) {
        viewButton->move(margin * 2 + KIconLoader::SizeMedium, option.rect.height() - margin - buttonHeight);
        viewButton->resize(buttonWidth, buttonHeight);
    }

    /// setup each of the three radio buttons
    for (int i = 0; i < 3; ++i) {
        QRadioButton *radioButton = qobject_cast<QRadioButton *>(widgets[posRadioNoDownload + i]);
        if (radioButton != NULL) {
            radioButton->move(option.rect.width() - margin - (3 - i) * (buttonWidth + margin), option.rect.height() - margin - buttonHeight);
            radioButton->resize(buttonWidth, buttonHeight);
            bool ok = false;
            radioButton->setChecked(i + FindPDF::NoDownload == index.data(DownloadModeRole).toInt(&ok) && ok);
        }
    }
}

QSize PDFItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    /// set a size that is suiteable
    QSize size;
    size.setWidth(option.fontMetrics.width(i18n("Download")) * 6);
    size.setHeight(qMax(option.fontMetrics.height() * 6, (int)KIconLoader::SizeMedium));
    return size;
}

/**
 * Method is called when the "View PDF" button of a list item is clicked.
 * Opens the associated URL or its local copy using the system's default viewer.
 */
void PDFItemDelegate::slotViewPDF()
{
    QModelIndex index = focusedIndex();

    if (index.isValid()) {
        QString tempfileName = index.data(TempFileNameRole).toString();
        QUrl url = index.data(URLRole).toUrl();
        if (!tempfileName.isEmpty()) {
            /// Guess mime type for url to open
            KUrl tempUrl(tempfileName);
            KMimeType::Ptr mimeType = FileInfo::mimeTypeForUrl(tempUrl);
            QString mimeTypeName = mimeType->name();
            if (mimeTypeName == QLatin1String("application/octet-stream"))
                mimeTypeName = QLatin1String("text/html");
            /// Ask KDE subsystem to open url in viewer matching mime type
            KRun::runUrl(tempUrl, mimeTypeName, NULL, false, false);
        } else if (url.isValid()) {
            /// Guess mime type for url to open
            KMimeType::Ptr mimeType = FileInfo::mimeTypeForUrl(url);
            QString mimeTypeName = mimeType->name();
            if (mimeTypeName == QLatin1String("application/octet-stream"))
                mimeTypeName = QLatin1String("text/html");
            /// Ask KDE subsystem to open url in viewer matching mime type
            KRun::runUrl(url, mimeTypeName, NULL, false, false);
        }
    }
}

/**
 * Updated the model when the user selects the radio button for ignoring a PDF file.
 */
void PDFItemDelegate::slotRadioNoDownloadToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::NoDownload, DownloadModeRole);
    }
}

/**
 * Updated the model when the user selects the radio button for downloading a PDF file.
 */
void PDFItemDelegate::slotRadioDownloadToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::Download, DownloadModeRole);
    }
}

/**
 * Updated the model when the user selects the radio button for keeping a PDF file's URL.
 */
void PDFItemDelegate::slotRadioURLonlyToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::URLonly, DownloadModeRole);
    }
}

PDFListModel::PDFListModel(QList<FindPDF::ResultItem> &resultList, QObject *parent)
        : QAbstractListModel(parent), m_resultList(resultList)
{
    // nothing
}

int PDFListModel::rowCount(const QModelIndex &parent) const
{
    /// row cout depends on number of found PDF references
    int count = parent == QModelIndex() ? m_resultList.count() : 0;
    return count;
}

QVariant PDFListModel::data(const QModelIndex &index, int role) const
{
    if (index != QModelIndex() && index.parent() == QModelIndex() && index.row() < m_resultList.count()) {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
            return m_resultList[index.row()].url.toString();
        else if (role == URLRole)
            return  m_resultList[index.row()].url;
        else if (role == TextualPreviewRole)
            return  m_resultList[index.row()].textPreview;
        else if (role == TempFileNameRole) {
            if (m_resultList[index.row()].tempFilename != NULL)
                return m_resultList[index.row()].tempFilename->fileName();
            else
                return QVariant();
        } else if (role == DownloadModeRole)
            return m_resultList[index.row()].downloadMode;
        else if (role == Qt::DecorationRole) {
            /// make an educated guess on the icon, based on URL or path
            QString iconName = FileInfo::mimeTypeForUrl(m_resultList[index.row()].url)->iconName();
            iconName = iconName == QLatin1String("application-octet-stream") ? FileInfo::mimeTypeForUrl(m_resultList[index.row()].url)->iconName() : iconName;
            return KIcon(iconName).pixmap(KIconLoader::SizeMedium, KIconLoader::SizeMedium);
        } else
            return QVariant();
    }
    return QVariant();
}

bool PDFListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index != QModelIndex() && index.row() < m_resultList.count() && role == DownloadModeRole) {
        bool ok = false;
        FindPDF::DownloadMode downloadMode = (FindPDF::DownloadMode)value.toInt(&ok);
        kDebug() << "FindPDF::DownloadMode=" << downloadMode << ok;
        if (ok) {
            kDebug()    << "Setting row " << index.row();
            m_resultList[index.row()].downloadMode = downloadMode;
            return true;
        }
    }
    return false;
}

QVariant PDFListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    if (section == 0) {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            return i18n("Result");
        } else
            return QVariant();
    }
    return QVariant();
}


FindPDFUI::FindPDFUI(Entry &entry, QWidget *parent)
        : QWidget(parent), m_findpdf(new FindPDF(this))
{
    createGUI();

    m_labelMessage->show();
    m_labelMessage->setText(i18n("Starting to search ..."));

    connect(m_findpdf, SIGNAL(finished()), this, SLOT(searchFinished()));
    connect(m_findpdf, SIGNAL(progress(int, int)), this, SLOT(searchProgress(int, int)));
    m_findpdf->search(entry);
}

FindPDFUI::~FindPDFUI()
{
    for (QList<FindPDF::ResultItem>::Iterator it = m_resultList.begin(); it != m_resultList.end();) {
        delete it->tempFilename;
        it = m_resultList.erase(it);
    }
}

void FindPDFUI::interactiveFindPDF(Entry &entry, const File &bibtexFile, QWidget *parent)
{
    KDialog dlg(parent);
    FindPDFUI widget(entry, &dlg);
    dlg.setCaption(i18n("Find PDF"));
    dlg.setMainWidget(&widget);
    dlg.enableButtonOk(false);

    connect(&widget, SIGNAL(resultAvailable(bool)), &dlg, SLOT(enableButtonOk(bool)));

    if (dlg.exec() == KDialog::Accepted) {
        widget.apply(entry, bibtexFile);
    }
}

void FindPDFUI::apply(Entry &entry, const File &bibtexFile)
{
    QAbstractItemModel *model = m_listViewResult->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        bool ok = false;
        FindPDF::DownloadMode downloadMode = (FindPDF::DownloadMode)model->data(model->index(i, 0), DownloadModeRole).toInt(&ok);
        if (!ok) {
            kDebug() << "Could not interprete download mode";
            downloadMode = FindPDF::NoDownload;
        }

        QUrl url = model->data(model->index(i, 0), URLRole).toUrl();
        QString tempfileName = model->data(model->index(i, 0), TempFileNameRole).toString();

        if (downloadMode == FindPDF::URLonly && url.isValid()) {
            bool alreadyContained = false;
            for (QMap<QString, Value>::ConstIterator it = entry.constBegin(); !alreadyContained && it != entry.constEnd(); ++it)
                alreadyContained |= it.key().toLower().startsWith(Entry::ftUrl) && PlainTextValue::text(it.value()) == url.toString();
            if (!alreadyContained) {
                Value value;
                value.append(QSharedPointer<VerbatimText>(new VerbatimText(url.toString())));
                if (!entry.contains(Entry::ftUrl))
                    entry.insert(Entry::ftUrl, value);
                else
                    for (int i = 2; i < 256; ++i) {
                        const QString keyName = QString("%1%2").arg(Entry::ftUrl).arg(i);
                        if (!entry.contains(keyName)) {
                            entry.insert(keyName, value);
                            break;
                        }
                    }
            }
        } else if (downloadMode == FindPDF::Download && !tempfileName.isEmpty()) {
            KUrl startUrl = bibtexFile.property(File::Url, QUrl()).toUrl();
            QString localFilename = KFileDialog::getSaveFileName(KUrl::fromLocalFile(startUrl.directory()), QLatin1String("application/pdf"), this, i18n("Save URL \"%1\"", url.toString()));
            localFilename = UrlListEdit::askRelativeOrStaticFilename(this, localFilename, startUrl);

            if (!localFilename.isEmpty()) {
                KIO::NetAccess::file_copy(KUrl::fromLocalFile(tempfileName), KUrl::fromLocalFile(localFilename), this);

                bool alreadyContained = false;
                for (QMap<QString, Value>::ConstIterator it = entry.constBegin(); !alreadyContained && it != entry.constEnd(); ++it)
                    alreadyContained |= (it.key().toLower().startsWith(Entry::ftLocalFile) || it.key().toLower().startsWith(Entry::ftUrl)) && PlainTextValue::text(it.value()) == url.toString();
                if (!alreadyContained) {
                    Value value;
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(localFilename)));
                    if (!entry.contains(Entry::ftLocalFile))
                        entry.insert(Entry::ftLocalFile, value);
                    else
                        for (int i = 2; i < 256; ++i) {
                            const QString keyName = QString("%1%2").arg(Entry::ftLocalFile).arg(i);
                            if (!entry.contains(keyName)) {
                                entry.insert(keyName, value);
                                break;
                            }
                        }
                }
            }
        }
    }
}

void FindPDFUI::createGUI()
{
    QGridLayout *layout = new QGridLayout(this);

    const int minWidth = fontMetrics().height() * 40;
    const int minHeight = fontMetrics().height() * 20;
    setMinimumSize(minWidth, minHeight);

    m_listViewResult = new QListView(this);
    layout->addWidget(m_listViewResult, 0, 0);
    m_listViewResult->setEnabled(false);
    m_listViewResult->hide();

    m_labelMessage = new QLabel(this);
    layout->addWidget(m_labelMessage, 1, 0);
    m_labelMessage->setMinimumSize(minWidth, minHeight);
    m_labelMessage->setAlignment(Qt::AlignVCenter | Qt::AlignCenter);

    static_cast<QWidget *>(parent())->setCursor(Qt::WaitCursor);
}

void FindPDFUI::searchFinished()
{
    m_labelMessage->hide();
    m_listViewResult->show();

    m_resultList = m_findpdf->results();
    m_listViewResult->setModel(new PDFListModel(m_resultList, m_listViewResult));
    m_listViewResult->setItemDelegate(new PDFItemDelegate(m_listViewResult, m_listViewResult));
    m_listViewResult->setEnabled(true);
    m_listViewResult->reset();

    static_cast<QWidget *>(parent())->unsetCursor();
    emit resultAvailable(true);
}

void FindPDFUI::searchProgress(int visitedPages, int foundDocuments)
{
    m_listViewResult->hide();
    m_labelMessage->show();
    m_labelMessage->setText(i18n("Searching ...\nNumber of visited pages: %1\nNumber of found documents: %2", visitedPages, foundDocuments));
}
