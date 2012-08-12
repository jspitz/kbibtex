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
#ifndef KBIBTEX_WEBSEARCH_ABSTRACT_H
#define KBIBTEX_WEBSEARCH_ABSTRACT_H

#include "kbibtexws_export.h"

#include <QObject>
#include <QMap>
#include <QString>
#include <QWidget>
#include <QMetaType>
#include <QNetworkCookieJar>

#include <KIcon>
#include <KUrl>
#include <KSharedConfig>

#include <entry.h>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;


class HTTPEquivCookieJar: public QNetworkCookieJar
{
    Q_OBJECT

public:
    HTTPEquivCookieJar(QNetworkAccessManager *parent);

    void checkForHttpEqiuv(const QString &htmlCode, const QUrl &url);

private:
    QNetworkAccessManager *m_nam;
};

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXWS_EXPORT WebSearchQueryFormAbstract : public QWidget
{
    Q_OBJECT

public:
    WebSearchQueryFormAbstract(QWidget *parent)
            : QWidget(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        // nothing
    }

    virtual bool readyToStart() const = 0;

    virtual void copyFromEntry(const Entry&) = 0;

protected:
    KSharedConfigPtr config;

    QStringList authorLastNames(const Entry &entry);

signals:
    void returnPressed();
};

Q_DECLARE_METATYPE(WebSearchQueryFormAbstract*)

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXWS_EXPORT WebSearchAbstract : public QObject
{
    Q_OBJECT

public:
    WebSearchAbstract(QWidget *parent);

    static const QString queryKeyFreeText;
    static const QString queryKeyTitle;
    static const QString queryKeyAuthor;
    static const QString queryKeyYear;

    static const int resultCancelled;
    static const int resultNoError;
    static const int resultUnspecifiedError;

    virtual void startSearch() = 0;
    virtual void startSearch(const QMap<QString, QString> &query, int numResults) = 0;
    virtual QString label() const = 0;
    QString name();
    virtual KIcon icon() const;
    virtual WebSearchQueryFormAbstract* customWidget(QWidget *parent) = 0;
    virtual KUrl homepage() const = 0;

public slots:
    void cancel();

protected:
    QWidget *m_parent;
    bool m_hasBeenCanceled;

    virtual QString favIconUrl() const = 0;

    /**
     * Split a string along spaces, but keep text in quotation marks together
     */
    QStringList splitRespectingQuotationMarks(const QString &text);

    /**
    * Will check for common problems with downloads via QNetworkReply. It will return true
    * if there is no problem and you may process this job result. If there is a problem,
    * this function will notify the user if necessary (KMessageBox), emit a
    * "stoppedSearch" signal, and return false.
    * @see handleErrors(KJob*)
    */
    bool handleErrors(QNetworkReply *reply);

    /**
     * Encode a text to be HTTP URL save, e.g. replace '=' by '%3D'.
     */
    QString encodeURL(QString rawText);

    QString decodeURL(QString rawText);

    QMap<QString, QString> formParameters(const QString &htmlText, const QString &formTagBegin);

    /**
     * Get the unique application-wide QNetworkAccessManager
     */
    QNetworkAccessManager *networkAccessManager();

    void setNetworkReplyTimeout(QNetworkReply *reply, int timeOutSec = 15);

    void setSuggestedHttpHeaders(QNetworkRequest &request, QNetworkReply *oldReply = NULL);

    /**
     * Delay sending of stop signal by a few milliseconds.
     * Necessary if search stops (is cancelled) already in one
     * of the startSearch functions.
     */
    void delayedStoppedSearch(int returnCode);

private:
    QString m_name;
    static const char *httpUnsafeChars;
    static QNetworkAccessManager *m_networkAccessManager;
    QMap<QTimer*, QNetworkReply*> m_mapTimerToReply;
    static const QStringList m_userAgentList;
    QString m_userAgent;
    int m_delayedStoppedSearchReturnCode;

private slots:
    void networkReplyTimeout();
    void networkReplyFinished();
    void delayedStoppedSearchTimer();

signals:
    void foundEntry(Entry*);
    void stoppedSearch(int);
    void progress(int, int);
};

#endif // KBIBTEX_WEBSEARCH_ABSTRACT_H