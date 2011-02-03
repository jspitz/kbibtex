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

#include <QString>
#include <QLatin1String>
#include <QTimer>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KUrl>
#include <KMessageBox>
#include <kparts/part.h>

#include <fileimporterpdf.h>
#include "openfileinfo.h"

const QString OpenFileInfo::propertyEncoding = QLatin1String("encoding");
const QString OpenFileInfo::mimetypeBibTeX = QLatin1String("text/x-bibtex");

class OpenFileInfo::OpenFileInfoPrivate
{
private:
    static int globalCounter;
    int m_counter;

public:
    static const QString keyLastAccess;
    static const QString keyURL;
    static const QString dateTimeFormat;

    OpenFileInfo *p;

    QMap<QString, QString> properties;
    KParts::ReadWritePart* part;
    KService::Ptr internalServicePtr;
    QWidget *internalWidgetParent;
    QDateTime lastAccessDateTime;
    StatusFlags flags;
    OpenFileInfoManager *openFileInfoManager;
    QString mimeType;
    KUrl url;

    OpenFileInfoPrivate(OpenFileInfoManager *openFileInfoManager, const KUrl &url, const QString &mimeType, OpenFileInfo *p)
            :  m_counter(-1), p(p), part(NULL), internalServicePtr(KService::Ptr()), internalWidgetParent(NULL), flags(0) {
        this->openFileInfoManager = openFileInfoManager;
        this->url = url;
        this->mimeType = mimeType;
    }

    ~OpenFileInfoPrivate() {
        if (part != NULL) {
            part->closeUrl(true);
            delete part;
        }
    }

    KParts::ReadWritePart* createPart(QWidget *newWidgetParent, KService::Ptr newServicePtr = KService::Ptr()) {
        Q_ASSERT(internalWidgetParent == NULL || internalWidgetParent == newWidgetParent);

        /** use cached part for this parent if possible */
        if (internalWidgetParent == newWidgetParent && (newServicePtr == KService::Ptr() || internalServicePtr == newServicePtr)) {
            Q_ASSERT(part != NULL);
            return part;
        } else if (part != NULL) {
            part->closeUrl(true);
            delete part;
        }

        /// reset to invalid values in case something goes wrong
        internalServicePtr = KService::Ptr();
        internalWidgetParent = NULL;

        if (newServicePtr.isNull()) {
            /// no valid KService has been passed
            /// try to find a read-write part to open file
            newServicePtr = p->defaultService();
        }
        if (newServicePtr.isNull()) {
            kError() << "Cannot find service to handle mimetype " << mimeType << endl;
            return NULL;
        }

        kDebug() << "using service " << newServicePtr->name() << newServicePtr->genericName() << newServicePtr->keywords().join(",");
        part = newServicePtr->createInstance<KParts::ReadWritePart>(newWidgetParent, (QObject*)newWidgetParent);
        if (part == NULL) {
            kError() << "Cannot find part for mimetype " << mimeType << endl;
            return NULL;
        }

        if (url.isValid()) {
            /// open URL in part
            part->openUrl(url);
            /// update document list widget accordingly
            p->addFlags(OpenFileInfo::RecentlyUsed);
            p->addFlags(OpenFileInfo::HasName);
        } else {
            /// initialize part with empty document
            part->openUrl(KUrl());
        }
        p->addFlags(OpenFileInfo::Open);

        internalServicePtr = newServicePtr;
        internalWidgetParent = newWidgetParent;

        Q_ASSERT(part != NULL); /// test should not be necessary, but just to be save ...
        return part;
    }

    int counter() {
        if (!url.isValid() && m_counter < 0)
            m_counter = ++globalCounter;
        else if (url.isValid())
            kWarning() << "This function should not be called if URL is valid";
        return m_counter;
    }

};

int OpenFileInfo::OpenFileInfoPrivate::globalCounter = 0;
const QString OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat = QLatin1String("yyyy-MM-dd-hh-mm-ss-zzz");
const QString OpenFileInfo::OpenFileInfoPrivate::keyLastAccess = QLatin1String("LastAccess");
const QString OpenFileInfo::OpenFileInfoPrivate::keyURL = QLatin1String("URL");

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const KUrl &url)
        : d(new OpenFileInfoPrivate(openFileInfoManager, url, KMimeType::findByUrl(url)->name(), this))
{
    // nothing
}

OpenFileInfo::OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType)
        : d(new OpenFileInfoPrivate(openFileInfoManager, KUrl(), mimeType, this))
{
    // nothing
}

OpenFileInfo::~OpenFileInfo()
{
    delete d;
}

void OpenFileInfo::setUrl(const KUrl& url)
{
    Q_ASSERT(url.isValid());
    d->url = url;
    d->mimeType = KMimeType::findByUrl(url)->name();
    addFlags(OpenFileInfo::HasName);
}

KUrl OpenFileInfo::url() const
{
    return d->url;
}

QString OpenFileInfo::mimeType() const
{
    return d->mimeType;
}

void OpenFileInfo::setProperty(const QString &key, const QString &value)
{
    d->properties[key] = value;
}

QString OpenFileInfo::property(const QString &key) const
{
    return d->properties[key];
}

QString OpenFileInfo::shortCaption() const
{
    if (d->url.isValid())
        return d->url.fileName();
    else
        return i18n("Unnamed-%1", d->counter());
}

QString OpenFileInfo::fullCaption() const
{
    if (d->url.isValid())
        return d->url.pathOrUrl();
    else
        return shortCaption();
}

KParts::ReadWritePart* OpenFileInfo::part(QWidget *parent, KService::Ptr servicePtr)
{
    return d->createPart(parent, servicePtr);
}

OpenFileInfo::StatusFlags OpenFileInfo::flags() const
{
    return d->flags;
}

void OpenFileInfo::setFlags(StatusFlags statusFlags)
{
    /// disallow files without name or valid url to become favorites
    if (!d->url.isValid() || !d->flags.testFlag(HasName)) statusFlags &= ~Favorite;

    bool hasChanged = d->flags != statusFlags;
    d->flags = statusFlags;
    if (hasChanged)
        emit flagsChanged(statusFlags);
}

void OpenFileInfo::addFlags(StatusFlags statusFlags)
{
    /// disallow files without name or valid url to become favorites
    if (!d->url.isValid() || !d->flags.testFlag(HasName)) statusFlags &= ~Favorite;

    bool hasChanged = (~d->flags & statusFlags) > 0;
    d->flags |= statusFlags;
    if (hasChanged)
        emit flagsChanged(statusFlags);
}

void OpenFileInfo::removeFlags(StatusFlags statusFlags)
{
    bool hasChanged = (d->flags & statusFlags) > 0;
    d->flags &= ~statusFlags;
    if (hasChanged)
        emit flagsChanged(statusFlags);
}

QDateTime OpenFileInfo::lastAccess() const
{
    return d->lastAccessDateTime;
}

void OpenFileInfo::setLastAccess(const QDateTime& dateTime)
{
    d->lastAccessDateTime = dateTime;
    emit flagsChanged(OpenFileInfo::RecentlyUsed);
}

KService::List OpenFileInfo::listOfServices()
{
    KService::List result = KMimeTypeTrader::self()->query(mimeType(), QLatin1String("KParts/ReadWritePart"));
    if (result.isEmpty())
        result = KMimeTypeTrader::self()->query(mimeType(), QLatin1String("KParts/ReadOnlyPart"));
    return result;
}

KService::Ptr OpenFileInfo::defaultService()
{
    KService::Ptr result = KMimeTypeTrader::self()->preferredService(mimeType(), QLatin1String("KParts/ReadWritePart"));
    if (result.isNull())
        result = KMimeTypeTrader::self()->preferredService(mimeType(), QLatin1String("KParts/ReadOnlyPart"));
    return result;
}

KService::Ptr OpenFileInfo::currentService()
{
    return d->internalServicePtr;
}

class OpenFileInfoManager::OpenFileInfoManagerPrivate
{
private:
    static const QString configGroupNameRecentlyUsed;
    static const QString configGroupNameFavorites;
    static const int maxNumRecentlyUsedFiles, maxNumFiles;

public:
    OpenFileInfoManager *p;

    QList<OpenFileInfo*> openFileInfoList;
    OpenFileInfo *currentFileInfo;

    OpenFileInfoManagerPrivate(OpenFileInfoManager *parent)
            : p(parent), currentFileInfo(NULL) {
        // nothing
    }

    ~OpenFileInfoManagerPrivate() {
        for (QList<OpenFileInfo*>::Iterator it = openFileInfoList.begin(); it != openFileInfoList.end(); ++it) {
            OpenFileInfo *ofi = *it;
            delete ofi;
        }
    }

    static bool byNameLessThan(const OpenFileInfo *left, const OpenFileInfo *right) {
        return left->shortCaption() < right->shortCaption();
    }

    static  bool byLRULessThan(const OpenFileInfo *left, const OpenFileInfo *right) {
        return left->lastAccess() > right->lastAccess(); /// reverse sorting!
    }

    void readConfig() {
        readConfig(OpenFileInfo::RecentlyUsed, configGroupNameRecentlyUsed);
        readConfig(OpenFileInfo::Favorite, configGroupNameFavorites);
    }

    void writeConfig() {
        writeConfig(OpenFileInfo::RecentlyUsed, configGroupNameRecentlyUsed);
        writeConfig(OpenFileInfo::Favorite, configGroupNameFavorites);
    }

    void readConfig(OpenFileInfo::StatusFlag statusFlag, const QString& configGroupName) {
        KSharedConfig::Ptr config = KGlobal::config();

        KConfigGroup cg(config, configGroupName);
        for (int i = 0; i < maxNumFiles; ++i) {
            KUrl fileUrl = KUrl(cg.readEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), ""));
            if (!fileUrl.isValid()) break;
            OpenFileInfo *ofi = p->contains(fileUrl);
            if (ofi == NULL) {
                ofi = p->open(fileUrl);
            }
            ofi->addFlags(statusFlag);
            ofi->addFlags(OpenFileInfo::HasName);
            QString encoding = cg.readEntry(QString("%1-%2").arg(OpenFileInfo::propertyEncoding).arg(i), "");
            ofi->setLastAccess(QDateTime::fromString(cg.readEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ""), OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
            ofi->setProperty(OpenFileInfo::propertyEncoding, encoding);
        }
    }

    void writeConfig(OpenFileInfo::StatusFlag statusFlag, const QString& configGroupName) {
        KSharedConfig::Ptr config = KGlobal::config();
        KConfigGroup cg(config, configGroupName);
        QList<OpenFileInfo*> list = p->filteredItems(statusFlag);

        int i = 0;
        for (QList<OpenFileInfo*>::Iterator it = list.begin(); i < maxNumFiles && it != list.end(); ++it, ++i) {
            OpenFileInfo *ofi = *it;

            cg.writeEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyURL).arg(i), ofi->url().pathOrUrl());
            cg.writeEntry(QString("%1-%2").arg(OpenFileInfo::propertyEncoding).arg(i), ofi->property(OpenFileInfo::propertyEncoding));
            cg.writeEntry(QString("%1-%2").arg(OpenFileInfo::OpenFileInfoPrivate::keyLastAccess).arg(i), ofi->lastAccess().toString(OpenFileInfo::OpenFileInfoPrivate::dateTimeFormat));
        }
        config->sync();
    }
};

const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameRecentlyUsed = QLatin1String("DocumentList-RecentlyUsed");
const QString OpenFileInfoManager::OpenFileInfoManagerPrivate::configGroupNameFavorites = QLatin1String("DocumentList-Favorites");
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumFiles = 256;
const int OpenFileInfoManager::OpenFileInfoManagerPrivate::maxNumRecentlyUsedFiles = 8;

OpenFileInfoManager *OpenFileInfoManager::singletonOpenFileInfoManager = NULL;

OpenFileInfoManager* OpenFileInfoManager::getOpenFileInfoManager()
{
    if (singletonOpenFileInfoManager == NULL)
        singletonOpenFileInfoManager = new OpenFileInfoManager();
    return singletonOpenFileInfoManager;
}

OpenFileInfoManager::OpenFileInfoManager()
        : d(new OpenFileInfoManagerPrivate(this))
{
    d->readConfig();
}

OpenFileInfoManager::~OpenFileInfoManager()
{
    d->writeConfig();
    delete d;
}

OpenFileInfo *OpenFileInfoManager::createNew(const QString& mimeType)
{
    OpenFileInfo *result = new OpenFileInfo(this, mimeType);
    connect(result, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)));
    d->openFileInfoList << result;
    result->setLastAccess();
    return result;
}

OpenFileInfo *OpenFileInfoManager::open(const KUrl & url)
{
    Q_ASSERT(url.isValid());

    OpenFileInfo *result = contains(url);
    if (result == NULL) {
        /// file not yet open
        result = new OpenFileInfo(this, url);
        connect(result, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)));
        d->openFileInfoList << result;
    }
    result->setLastAccess();
    return result;
}

OpenFileInfo *OpenFileInfoManager::contains(const KUrl&url) const
{
    if (!url.isValid()) return NULL; /// can only be unnamed file

    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if (ofi->url().equals(url))
            return ofi;
    }
    return NULL;
}

bool OpenFileInfoManager::changeUrl(OpenFileInfo *openFileInfo, const KUrl & url)
{
    OpenFileInfo *previouslyContained = contains(url);

    /// check if old url differs from new url and old url is valid
    if (previouslyContained != NULL && previouslyContained->flags().testFlag(OpenFileInfo::Open) && previouslyContained != openFileInfo) {
        kDebug() << "Open file with same URL already exists, forcefully closing it" << endl;
        close(previouslyContained);
    }

    KUrl oldUrl = openFileInfo->url();
    openFileInfo->setUrl(url);

    if (!url.equals(oldUrl) && oldUrl.isValid()) {
        /// current document was most probabily renamed (e.g. due to "Save As")
        /// add old URL to recently used files, but exclude the open files list
        OpenFileInfo *ofi = open(oldUrl);
        OpenFileInfo::StatusFlags statusFlags = (openFileInfo->flags() & (~OpenFileInfo::Open)) | OpenFileInfo::RecentlyUsed;
        ofi->setFlags(statusFlags);
        ofi->setProperty(OpenFileInfo::propertyEncoding, openFileInfo->property(OpenFileInfo::propertyEncoding));
    }
    if (previouslyContained != NULL) {
        /// keep Favorite flag if set in file that have previously same URL
        if (previouslyContained->flags().testFlag(OpenFileInfo::Favorite))
            openFileInfo->setFlags(openFileInfo->flags() | OpenFileInfo::Favorite);

        /// remove the old entry with the same url has it will be replaced by the new one
        d->openFileInfoList.removeOne(previouslyContained);
        previouslyContained->deleteLater();
        OpenFileInfo::StatusFlags statusFlags = OpenFileInfo::Open;
        statusFlags |= OpenFileInfo::RecentlyUsed;
        statusFlags |= OpenFileInfo::Favorite;
        emit flagsChanged(statusFlags);
    }

    if (openFileInfo == d->currentFileInfo)
        emit currentChanged(openFileInfo, KService::Ptr());
    emit flagsChanged(openFileInfo->flags());

    return true;
}

void OpenFileInfoManager::close(OpenFileInfo *openFileInfo)
{
    Q_ASSERT_X(openFileInfo != NULL, "void OpenFileInfoManager::close(OpenFileInfo *openFileInfo)", "Cannot close openFileInfo which is NULL");
    bool isClosing = false;
    openFileInfo->setLastAccess();

    /// remove flag "open" from file to be closed and determine which file to show instead
    OpenFileInfo *nextCurrent = (d->currentFileInfo == openFileInfo) ? NULL : d->currentFileInfo;
    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if (!isClosing && ofi == openFileInfo) {
            isClosing = true;
            openFileInfo->removeFlags(OpenFileInfo::Open);
            openFileInfo->addFlags(OpenFileInfo::RecentlyUsed);
        } else if (nextCurrent == NULL && ofi->flags().testFlag(OpenFileInfo::Open))
            nextCurrent = ofi;
    }
    setCurrentFile(nextCurrent);

    if (isClosing)
        emit closing(openFileInfo);
}

OpenFileInfo *OpenFileInfoManager::currentFile() const
{
    return d->currentFileInfo;
}

void OpenFileInfoManager::setCurrentFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr)
{
    bool hasChanged = d->currentFileInfo != openFileInfo;
    OpenFileInfo *previous = d->currentFileInfo;
    d->currentFileInfo = openFileInfo;

    if (d->currentFileInfo != NULL) {
        d->currentFileInfo->addFlags(OpenFileInfo::Open);
        d->currentFileInfo->setLastAccess();
    }
    if (hasChanged) {
        if (previous != NULL) previous->setLastAccess();
        emit currentChanged(openFileInfo, servicePtr);
    } else if (servicePtr != openFileInfo->currentService())
        emit currentChanged(openFileInfo, servicePtr);
}

QList<OpenFileInfo*> OpenFileInfoManager::filteredItems(OpenFileInfo::StatusFlags required, OpenFileInfo::StatusFlags forbidden)
{
    QList<OpenFileInfo*> result;

    for (QList<OpenFileInfo*>::Iterator it = d->openFileInfoList.begin(); it != d->openFileInfoList.end(); ++it) {
        OpenFileInfo *ofi = *it;
        if ((ofi->flags() & required) == required && (ofi->flags() & forbidden) == 0)
            result << ofi;
    }

    if (required == OpenFileInfo::RecentlyUsed)
        qSort(result.begin(), result.end(), OpenFileInfoManagerPrivate::byLRULessThan);
    else if (required == OpenFileInfo::Favorite || required == OpenFileInfo::Open)
        qSort(result.begin(), result.end(), OpenFileInfoManagerPrivate::byNameLessThan);


    return result;
}

void OpenFileInfoManager::deferredListsChanged()
{
    kDebug() << "deferredListsChanged" << endl;
    OpenFileInfo::StatusFlags statusFlags = OpenFileInfo::Open;
    statusFlags |= OpenFileInfo::RecentlyUsed;
    statusFlags |= OpenFileInfo::Favorite;
    emit flagsChanged(statusFlags);
}
