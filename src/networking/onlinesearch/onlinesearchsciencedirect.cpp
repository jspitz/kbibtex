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

#include "onlinesearchsciencedirect.h"

#include <QNetworkReply>
#include <QUrlQuery>

#include <KLocalizedString>

#include "encoderlatex.h"
#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchScienceDirect::OnlineSearchScienceDirectPrivate
{
private:
    // UNUSED OnlineSearchScienceDirect *p;

public:
    QString queryFreetext, queryAuthor;
    int currentSearchPosition;
    int numExpectedResults, numFoundResults;
    QStringList bibTeXUrls;
    int runningJobs;
    int numSteps, curStep;

    OnlineSearchScienceDirectPrivate(OnlineSearchScienceDirect */* UNUSED parent*/)
        : /* UNUSED p(parent), */ currentSearchPosition(0), numExpectedResults(0), numFoundResults(0), runningJobs(0), numSteps(0), curStep(0) {
        /// nothing
    }

    void sanitizeBibTeXCode(QString &code) {
        /// find and escape unprotected quotation marks in the text (e.g. abstract)
        const QRegExp quotationMarks("([^= ]\\s*)\"(\\s*[a-z.])", Qt::CaseInsensitive);
        int p = -2;
        while ((p = quotationMarks.indexIn(code, p + 2)) >= 0) {
            code = code.left(p - 1) + quotationMarks.cap(1) + '\\' + '"' + quotationMarks.cap(2) + code.mid(p + quotationMarks.cap(0).length());
        }
        /// Remove "<!-- Tag Not Handled -->" and other XML tags from keywords
        int i1 = -1;
        while ((i1 = code.indexOf(QStringLiteral("keywords = "), i1 + 1)) > 0) {
            const int i2 = code.indexOf(QStringLiteral("\n"), i1);
            int t1 = -1;
            while ((t1 = code.indexOf(QLatin1Char('<'), i1 + 7)) > 0) {
                const int t2 = code.indexOf(QLatin1Char('>'), t1);
                if (t2 > 0 && t1 < i2 && t2 < i2)
                    code = code.left(t1) + code.mid(t2 + 1);
                else
                    break;
            }
        }

        /// Fix some HTML-isms
        code.replace(QStringLiteral("\\&amp;"), QStringLiteral("\\&")).replace(QStringLiteral("&amp;"), QStringLiteral("\\&"));
    }
};

OnlineSearchScienceDirect::OnlineSearchScienceDirect(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchScienceDirectPrivate(this))
{
    // nothing
}

OnlineSearchScienceDirect::~OnlineSearchScienceDirect()
{
    delete d;
}

void OnlineSearchScienceDirect::startSearch()
{
    d->runningJobs = 0;
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchScienceDirect::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->runningJobs = 0;
    d->numFoundResults = 0;
    m_hasBeenCanceled = false;
    d->bibTeXUrls.clear();
    d->currentSearchPosition = 0;
    d->curStep = 0;
    d->numSteps = 2 + 2 * numResults;

    d->queryFreetext = query[queryKeyFreeText] + ' ' + query[queryKeyTitle] + ' ' + query[queryKeyYear];
    d->queryAuthor = query[queryKeyAuthor];
    d->numExpectedResults = numResults;

    ++d->runningJobs;
    QNetworkRequest request(QStringLiteral("http://www.sciencedirect.com/science/search"));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));

    emit progress(0, d->numSteps);
}

QString OnlineSearchScienceDirect::label() const
{
    return i18n("ScienceDirect");
}

QString OnlineSearchScienceDirect::favIconUrl() const
{
    return QStringLiteral("http://www.sciencedirect.com/scidirimg/faviconSD.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchScienceDirect::customWidget(QWidget *)
{
    return NULL;
}

QUrl OnlineSearchScienceDirect::homepage() const
{
    return QUrl(QStringLiteral("http://www.sciencedirect.com/"));
}

void OnlineSearchScienceDirect::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchScienceDirect::doneFetchingStartPage()
{
    emit progress(++d->curStep, d->numSteps);

    --d->runningJobs;
    if (d->runningJobs != 0)
        qCWarning(LOG_KBIBTEX_NETWORKING) << "In OnlineSearchScienceDirect::doneFetchingStartPage: Some jobs are running (" << d->runningJobs << "!= 0 )";

    QUrl redirUrl;
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply, redirUrl)) {
        const QString htmlText = QString::fromUtf8(reply->readAll().constData());

        if (redirUrl.isValid()) {
            ++d->numSteps;
            ++d->runningJobs;

            /// redirection to another url
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply->url());
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingStartPage()));
        } else {
            InternalNetworkAccessManager::self()->mergeHtmlHeadCookies(htmlText, reply->url());

            QUrl url(QStringLiteral("http://www.sciencedirect.com/science"));
            QMap<QString, QString> inputMap = formParameters(htmlText, QStringLiteral("<form id=\"quickSearch\" name=\"qkSrch\" metho"));
            inputMap[QStringLiteral("qs_all")] = d->queryFreetext.simplified();
            inputMap[QStringLiteral("qs_author")] = d->queryAuthor.simplified();
            inputMap[QStringLiteral("resultsPerPage")] = QString::number(d->numExpectedResults);
            inputMap[QStringLiteral("_ob")] = QStringLiteral("QuickSearchURL");
            inputMap[QStringLiteral("_method")] = QStringLiteral("submitForm");
            inputMap[QStringLiteral("sdSearch")] = QStringLiteral("Search");

            QUrlQuery query(url);
            static const QStringList orderOfParameters = QString(QStringLiteral("_ob|_method|_acct|_origin|_zone|md5|_eidkey|qs_issue|qs_pages|qs_title|qs_vol|sdSearch|qs_all|qs_author|resultsPerPage")).split(QStringLiteral("|"));
            foreach (const QString &key, orderOfParameters) {
                if (!inputMap.contains(key)) continue;
                query.addQueryItem(key, inputMap[key]);
            }
            url.setQuery(query);

            ++d->runningJobs;
            QNetworkRequest request(url);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchScienceDirect::doneFetchingResultPage()
{
    --d->runningJobs;
    if (d->runningJobs != 0)
        qCWarning(LOG_KBIBTEX_NETWORKING) << "In OnlineSearchScienceDirect::doneFetchingResultPage: Some jobs are running (" << d->runningJobs << "!= 0 )";

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        QUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirUrl.isEmpty()) {
            ++d->runningJobs;
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingResultPage()));
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
        } else {
            emit progress(++d->curStep, d->numSteps);

            const QString htmlText = QString::fromUtf8(reply->readAll().constData());
            InternalNetworkAccessManager::self()->mergeHtmlHeadCookies(htmlText, reply->url());

            QSet<QString> knownUrls;
            int p = -1, p2 = -1;
            while ((p = htmlText.indexOf(QStringLiteral("http://www.sciencedirect.com/science/article/pii/"), p + 1)) >= 0 && (p2 = htmlText.indexOf(QRegExp("[\"/ #]"), p + 50)) >= 0) {
                const QString urlText = htmlText.mid(p, p2 - p);
                if (knownUrls.contains(urlText)) continue;
                knownUrls.insert(urlText);

                if (d->numFoundResults < d->numExpectedResults) {
                    ++d->numFoundResults;
                    ++d->runningJobs;
                    QUrl url(urlText);
                    QNetworkRequest request(url);
                    QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
                    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
                    connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingAbstractPage()));
                }
            }
        }

        if (d->runningJobs <= 0) {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchScienceDirect::doneFetchingAbstractPage()
{
    --d->runningJobs;
    if (d->runningJobs < 0)
        qCWarning(LOG_KBIBTEX_NETWORKING) << "In OnlineSearchScienceDirect::doneFetchingAbstractPage: Counting jobs failed (" << d->runningJobs << "< 0 )";

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        QUrl redirUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirUrl.isEmpty()) {
            ++d->runningJobs;
            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
            connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingAbstractPage()));
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
        } else {
            emit progress(++d->curStep, d->numSteps);

            const QString htmlText = QString::fromUtf8(reply->readAll().constData());
            InternalNetworkAccessManager::self()->mergeHtmlHeadCookies(htmlText, reply->url());

            int p1 = -1, p2 = -1;
            if ((p1 = htmlText.indexOf(QStringLiteral("/science?_ob=DownloadURL&"))) >= 0 && (p2 = htmlText.indexOf(QRegExp("[ \"<>]"), p1 + 1)) >= 0) {
                QUrl url("http://www.sciencedirect.com" + htmlText.mid(p1, p2 - p1));
                QUrlQuery query(url);
                query.addQueryItem(QStringLiteral("citation-type"), QStringLiteral("BIBTEX"));
                query.addQueryItem(QStringLiteral("format"), QStringLiteral("cite-abs"));
                query.addQueryItem(QStringLiteral("export"), QStringLiteral("Export"));
                url.setQuery(query);
                ++d->runningJobs;
                QNetworkRequest request(url);
                QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request, reply);
                connect(newReply, SIGNAL(finished()), this, SLOT(doneFetchingBibTeX()));
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            }
        }

        if (d->runningJobs <= 0) {
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchScienceDirect::doneFetchingBibTeX()
{
    emit progress(++d->curStep, d->numSteps);

    --d->runningJobs;
    if (d->runningJobs < 0)
        qCWarning(LOG_KBIBTEX_NETWORKING) << "In OnlineSearchScienceDirect::doneFetchingAbstractPage: Counting jobs failed (" << d->runningJobs << "< 0 )";

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());
        d->sanitizeBibTeXCode(bibTeXcode);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntry = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                hasEntry |= publishEntry(entry);
            }
            delete bibtexFile;
        }

        if (d->runningJobs <= 0) {
            emit stoppedSearch(hasEntry ? resultNoError : resultUnspecifiedError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}
