/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "onlinesearchacmportal.h"

#include <QBuffer>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

#ifdef HAVE_KF5
#include <KLocalizedString>
#else // HAVE_KF5
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#include "file.h"
#include "entry.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchAcmPortal::OnlineSearchAcmPortalPrivate
{
public:
    QString joinedQueryString;
    int numExpectedResults, numFoundResults;
    const QString acmPortalBaseUrl;
    int currentSearchPosition;
    QStringList citationUrls;

    OnlineSearchAcmPortalPrivate(OnlineSearchAcmPortal *)
            : numExpectedResults(0), numFoundResults(0),
          acmPortalBaseUrl(QStringLiteral("https://dl.acm.org/")), currentSearchPosition(0) {
        /// nothing
    }

    void sanitizeBibTeXCode(QString &code) {
        static const QRegularExpression htmlEncodedChar(QStringLiteral("&#(\\d+);"));
        QRegularExpressionMatch match;
        while ((match = htmlEncodedChar.match(code)).hasMatch()) {
            bool ok = false;
            QChar c(match.captured(1).toInt(&ok));
            if (ok)
                code = code.replace(match.captured(0), c);
        }

        /// ACM's BibTeX code does not properly use various commands.
        /// For example, ACM writes "Ke\ssler" instead of "Ke{\ss}ler"
        static const QStringList inlineCommands {QStringLiteral("\\ss")};
        for (const QString &cmd : inlineCommands) {
            const QString wrappedCmd = QString(QStringLiteral("{%1}")).arg(cmd);
            code = code.replace(cmd, wrappedCmd);
        }
    }
};

OnlineSearchAcmPortal::OnlineSearchAcmPortal(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchAcmPortalPrivate(this))
{
    /// nothing
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
    d->citationUrls.clear();
    d->numFoundResults = 0;
    emit progress(curStep = 0, numSteps = numResults * 2 + 2);

    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        d->joinedQueryString.append(it.value() + QStringLiteral(" "));
    }
    d->numExpectedResults = numResults;

    QNetworkRequest request(d->acmPortalBaseUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingStartPage);

    refreshBusyProperty();
}

QString OnlineSearchAcmPortal::label() const
{
    return i18n("ACM Digital Library");
}

QString OnlineSearchAcmPortal::favIconUrl() const
{
    return QStringLiteral("https://dl.acm.org/favicon.ico");
}

QUrl OnlineSearchAcmPortal::homepage() const
{
    return QUrl(QStringLiteral("https://dl.acm.org/"));
}

void OnlineSearchAcmPortal::doneFetchingStartPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlSource = QString::fromUtf8(reply->readAll().constData());
        int p1 = -1, p2 = -1, p3 = -1;
        if ((p1 = htmlSource.indexOf(QStringLiteral("<form name=\"qiksearch\""))) >= 0
                && (p2 = htmlSource.indexOf(QStringLiteral("action="), p1)) >= 0
                && (p3 = htmlSource.indexOf(QStringLiteral("\""), p2 + 8)) >= 0) {
            const QString body = QString(QStringLiteral("Go.x=0&Go.y=0&query=%1")).arg(d->joinedQueryString).simplified();
            const QString action = decodeURL(htmlSource.mid(p2 + 8, p3 - p2 - 8));
            const QUrl url(reply->url().resolved(QUrl(action + QStringLiteral("?") + body)));

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingSearchPage);
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Search using" << label() << "failed.";
            stopSearch(resultUnspecifiedError);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchAcmPortal::doneFetchingSearchPage()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QString htmlSource = QString::fromUtf8(reply->readAll().constData());

        static const QRegularExpression citationUrlRegExp(QStringLiteral("citation.cfm\\?id=[0-9][0-9.]+[0-9]"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator citationUrlRegExpMatchIt = citationUrlRegExp.globalMatch(htmlSource);
        while (citationUrlRegExpMatchIt.hasNext()) {
            const QRegularExpressionMatch citationUrlRegExpMatch = citationUrlRegExpMatchIt.next();
            const QString newUrl = d->acmPortalBaseUrl + citationUrlRegExpMatch.captured(0);
            d->citationUrls << newUrl;
        }

        if (d->currentSearchPosition + 20 < d->numExpectedResults) {
            d->currentSearchPosition += 20;
            QUrl url(reply->url());
            QUrlQuery query(url);
            query.addQueryItem(QStringLiteral("start"), QString::number(d->currentSearchPosition));
            url.setQuery(query);

            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingSearchPage);
        } else if (!d->citationUrls.isEmpty()) {
            QNetworkRequest request(d->citationUrls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingCitation);
            d->citationUrls.removeFirst();
        } else
            stopSearch(resultNoError);
    }

    refreshBusyProperty();
}

void OnlineSearchAcmPortal::doneFetchingCitation()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    QSet<const QUrl> bibTeXUrls;

    if (handleErrors(reply)) {
        const QString htmlSource = QString::fromUtf8(reply->readAll().constData());

        static const QRegularExpression idRegExp(QStringLiteral("citation_abstract_html_url\" content=\"http://dl.acm.org/citation.cfm\\?id=([0-9]+)[.]([0-9]+)"));
        QRegularExpressionMatchIterator idRegExpMatchIt = idRegExp.globalMatch(htmlSource);
        while (idRegExpMatchIt.hasNext()) {
            const QRegularExpressionMatch idRegExpMatch = idRegExpMatchIt.next();
            const QString parentId = idRegExpMatch.captured(1);
            const QString id = idRegExpMatch.captured(2);
            if (!parentId.isEmpty() && !id.isEmpty()) {
                const QUrl bibTeXUrl(d->acmPortalBaseUrl + QString(QStringLiteral("/downformats.cfm?id=%1&parent_id=%2&expformat=bibtex")).arg(id, parentId));
                bibTeXUrls.insert(bibTeXUrl);
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No citation link found in " << reply->url().toDisplayString() << "  parentId=" << parentId;
                stopSearch(resultNoError);
                emit progress(curStep = numSteps, numSteps);
                return;
            }
        }
        if (bibTeXUrls.isEmpty())
            qCWarning(LOG_KBIBTEX_NETWORKING) << "No citation link found in " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
    }

    if (bibTeXUrls.isEmpty()) {
        if (!d->citationUrls.isEmpty()) {
            QNetworkRequest request(d->citationUrls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingCitation);
            d->citationUrls.removeFirst();
        } else
            stopSearch(resultNoError);
    } else {
        numSteps += bibTeXUrls.count() - 1;
        for (QSet<const QUrl>::ConstIterator it = bibTeXUrls.constBegin(); it != bibTeXUrls.constEnd(); ++it) {
            const QUrl &bibTeXUrl = *it;
            QNetworkRequest request(bibTeXUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingBibTeX);
        }
    }

    refreshBusyProperty();
}

void OnlineSearchAcmPortal::doneFetchingBibTeX()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        FileImporterBibTeX importer(this);
        d->sanitizeBibTeXCode(bibTeXcode);
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != nullptr) {
            for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                if (publishEntry(entry))
                    ++d->numFoundResults;
            }
            delete bibtexFile;
        }

        if (!d->citationUrls.isEmpty() && d->numFoundResults < d->numExpectedResults) {
            QNetworkRequest request(d->citationUrls.first());
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request, reply);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchAcmPortal::doneFetchingCitation);
            d->citationUrls.removeFirst();
        } else
            stopSearch(resultNoError);
    }

    refreshBusyProperty();
}
