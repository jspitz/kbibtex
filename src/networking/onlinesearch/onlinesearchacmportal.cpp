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

#include "onlinesearchacmportal.h"

#include <QBuffer>
#include <QLayout>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#include <KLocalizedString>
#include <KMessageBox>

#include "file.h"
#include "entry.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchAcmPortal::OnlineSearchAcmPortalPrivate
{
private:
    // UNUSED OnlineSearchAcmPortal *p;

public:
    QString joinedQueryString;
    int numExpectedResults, numFoundResults;
    const QString acmPortalBaseUrl;
    int currentSearchPosition;
    QStringList bibTeXUrls;

    int curStep, numSteps;

    OnlineSearchAcmPortalPrivate(OnlineSearchAcmPortal */* UNUSED parent*/)
        : /* UNUSED p(parent),*/ numExpectedResults(0), numFoundResults(0),
          acmPortalBaseUrl(QStringLiteral("http://dl.acm.org/")), currentSearchPosition(0), curStep(0), numSteps(0) {
        // nothing
    }

    void sanitizeBibTeXCode(QString &code) {
        const QRegExp htmlEncodedChar("&#(\\d+);");
        while (htmlEncodedChar.indexIn(code) >= 0) {
            bool ok = false;
            QChar c(htmlEncodedChar.cap(1).toInt(&ok));
            if (ok) {
                code = code.replace(htmlEncodedChar.cap(0), c);
            }
        }

        /// ACM's BibTeX code does not properly use various commands.
        /// For example, ACM writes "Ke\ssler" instead of "Ke{\ss}ler"
        static const QStringList inlineCommands = QStringList() << QStringLiteral("ss");
        for (QStringList::ConstIterator it = inlineCommands.constBegin(); it != inlineCommands.constEnd(); ++it) {
            const QString cmd = QString(QStringLiteral("\\%1")).arg(*it);
            const QString wrappedCmd = QString(QStringLiteral("{%1}")).arg(cmd);
            code = code.replace(cmd, wrappedCmd);
        }
    }
};

OnlineSearchAcmPortal::OnlineSearchAcmPortal(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchAcmPortalPrivate(this))
{
    // nothing
}

OnlineSearchAcmPortal::~OnlineSearchAcmPortal()
{
    delete d;
}

void OnlineSearchAcmPortal::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->joinedQueryString.clear();
    d->currentSearchPosition = 1;
    d->bibTeXUrls.clear();
    d->numFoundResults = 0;
    d->curStep = 0;
    d->numSteps = numResults + 2;

    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        d->joinedQueryString.append(it.value() + ' ');
    }
    d->numExpectedResults = numResults;

    QNetworkRequest request(d->acmPortalBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
    emit progress(0, d->numSteps);
}

void OnlineSearchAcmPortal::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

QString OnlineSearchAcmPortal::label() const
{
    return i18n("ACM Digital Library");
}

QString OnlineSearchAcmPortal::favIconUrl() const
{
    return QStringLiteral("http://dl.acm.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchAcmPortal::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchAcmPortal::homepage() const
{
    return QUrl(QStringLiteral("http://dl.acm.org/"));
}

void OnlineSearchAcmPortal::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchAcmPortal::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlSource = QString::fromUtf8(reply->readAll().constData());
        int p1 = -1, p2 = -1, p3 = -1;
        if ((p1 = htmlSource.indexOf(QStringLiteral("<form name=\"qiksearch\""))) >= 0
                && (p2 = htmlSource.indexOf(QStringLiteral("action="), p1)) >= 0
                && (p3 = htmlSource.indexOf(QStringLiteral("\""), p2 + 8)) >= 0) {
            QString action = decodeURL(htmlSource.mid(p2 + 8, p3 - p2 - 8));
            QUrl url(d->acmPortalBaseUrl + action);
            QString body = QString(QStringLiteral("Go=&query=%1")).arg(d->joinedQueryString).simplified();

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->post(request, body.toUtf8());
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSearchPage()));
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Search using" << label() << "failed.";
            KMessageBox::error(m_parent, i18n("Searching '%1' failed: Could not extract form from ACM's start page.", label()));
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchAcmPortal::doneFetchingSearchPage()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlSource = QString::fromUtf8(reply->readAll().constData());
        static QRegExp paramRegExp("<a [^>]+\\?id=([0-9]+)\\.([0-9]+).*CFID=([0-9]+).*CFTOKEN=([0-9]+)", Qt::CaseInsensitive);
        int p1 = -1;
        while ((p1 = htmlSource.indexOf(paramRegExp, p1 + 1)) >= 0) {
            d->bibTeXUrls << d->acmPortalBaseUrl + QString(QStringLiteral("/downformats.cfm?id=%1&parent_id=%2&expformat=bibtex&CFID=%3&CFTOKEN=%4")).arg(paramRegExp.cap(2), paramRegExp.cap(1), paramRegExp.cap(3), paramRegExp.cap(4));
        }

        if (d->currentSearchPosition + 20 < d->numExpectedResults) {
            d->currentSearchPosition += 20;
            QUrl url(reply->url());
            QUrlQuery query(url);
            query.addQueryItem(QStringLiteral("start"), QString::number(d->currentSearchPosition));
            url.setQuery(query);

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingSearchPage()));
        } else if (!d->bibTeXUrls.isEmpty()) {
            QNetworkRequest request(d->bibTeXUrls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->bibTeXUrls.removeFirst();
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    }  else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchAcmPortal::doneFetchingBibTeX()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        FileImporterBibTeX importer;
        d->sanitizeBibTeXCode(bibTeXcode);
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                if (publishEntry(entry))
                    ++d->numFoundResults;
            }
            delete bibtexFile;
        }

        if (!d->bibTeXUrls.isEmpty() && d->numFoundResults < d->numExpectedResults) {
            QNetworkRequest request(d->bibTeXUrls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
            d->bibTeXUrls.removeFirst();
        } else {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}
