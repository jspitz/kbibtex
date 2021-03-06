/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchoclcworldcat.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrlQuery>

#include <KLocalizedString>

#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchOCLCWorldCat::Private
{
private:
    static const QString APIkey;

    OnlineSearchOCLCWorldCat *p;
public:
    static const int countPerStep;
    int maxSteps, curStep;

    QUrl baseUrl;

    XSLTransform *xslt;

    Private(OnlineSearchOCLCWorldCat *parent)
            : p(parent), maxSteps(0), curStep(0) {
        const QString xsltFilename = QStringLiteral("kbibtex/worldcatdc2bibtex.xsl");
        xslt = XSLTransform::createXSLTransform(QStandardPaths::locate(QStandardPaths::GenericDataLocation, xsltFilename));
        if (xslt == NULL)
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not create XSLT transformation for" << xsltFilename;
    }

    ~Private() {
        delete xslt;
    }

    void setBaseUrl(const QMap<QString, QString> &query) {
        QStringList queryFragments;

        /// Add words from "free text" field
        /// WorldCat's Open Search does not support "free" search,
        /// instead, title, keyword, subject etc are searched OR-connected
        const QStringList freeTextWords = p->splitRespectingQuotationMarks(query[queryKeyFreeText]);
        for (const QString &text : freeTextWords) {
            static const QString freeWorldTemplate = QStringLiteral("(+srw.ti+all+\"%1\"+or+srw.kw+all+\"%1\"+or+srw.au+all+\"%1\"+or+srw.bn+all+\"%1\"+or+srw.su+all+\"%1\"+)");
            queryFragments.append(freeWorldTemplate.arg(text));
        }
        /// Add words from "title" field
        const QStringList titleWords = p->splitRespectingQuotationMarks(query[queryKeyTitle]);
        for (const QString &text : titleWords) {
            static const QString titleTemplate = QStringLiteral("srw.ti+all+\"%1\"");
            queryFragments.append(titleTemplate.arg(text));
        }
        /// Add words from "author" field
        const QStringList authorWords = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        for (const QString &text : authorWords) {
            static const QString authorTemplate = QStringLiteral("srw.au+all+\"%1\"");
            queryFragments.append(authorTemplate.arg(text));
        }

        /// Field year cannot stand alone, therefore if no query fragments
        /// have been assembled yet, no valid search will be possible.
        /// Invalidate base URL and return in this case.
        if (queryFragments.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "For WorldCat OCLC search to work, either a title or an author has get specified; free text or year only is not sufficient";
            baseUrl.clear();
            return;
        }

        /// Add words from "year" field
        const QStringList yearWords = p->splitRespectingQuotationMarks(query[queryKeyYear]);
        for (const QString &text : yearWords) {
            static const QString yearTemplate = QStringLiteral("srw.yr+any+\"%1\"");
            queryFragments.append(yearTemplate.arg(text));
        }

        const QString queryString = queryFragments.join(QStringLiteral("+and+"));
        baseUrl = QUrl(QString(QStringLiteral("http://www.worldcat.org/webservices/catalog/search/worldcat/sru?query=%1&recordSchema=%3&wskey=%2&maximumRecords=%4")).arg(queryString).arg(OnlineSearchOCLCWorldCat::Private::APIkey).arg(QStringLiteral("info%3Asrw%2Fschema%2F1%2Fdc")).arg(OnlineSearchOCLCWorldCat::Private::countPerStep));
    }
};

const int OnlineSearchOCLCWorldCat::Private::countPerStep = 11; /// pseudo-randomly chosen prime number between 10 and 20
const QString OnlineSearchOCLCWorldCat::Private::APIkey = QStringLiteral("Bt6h4KIHrfbSXEahwUzpFQD6SNjQZfQUG3W2LN9oNEB5tROFGeRiDVntycEEyBe0aH17sH4wrNlnVANH");

OnlineSearchOCLCWorldCat::OnlineSearchOCLCWorldCat(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchOCLCWorldCat::Private(this)) {
    // TODO
}

OnlineSearchOCLCWorldCat::~OnlineSearchOCLCWorldCat() {
    delete d;
}

void OnlineSearchOCLCWorldCat::startSearch() {
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchOCLCWorldCat::startSearch(const QMap<QString, QString> &query, int numResults) {
    if (d->xslt == NULL) {
        /// Don't allow searches if xslt is not defined
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot allow searching" << label() << "if XSL Transformation not available";
        delayedStoppedSearch(resultUnspecifiedError);
        return;
    }

    m_hasBeenCanceled = false;

    d->maxSteps = (numResults + OnlineSearchOCLCWorldCat::Private::countPerStep - 1) / OnlineSearchOCLCWorldCat::Private::countPerStep;
    d->curStep = 0;
    emit progress(d->curStep, d->maxSteps);

    d->setBaseUrl(query);
    if (d->baseUrl.isEmpty()) {
        /// No base URL set for some reason, no search possible
        m_hasBeenCanceled = false;
        delayedStoppedSearch(resultInvalidArguments);
        return;
    }
    QUrl startUrl = d->baseUrl;
    QNetworkRequest request(startUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, SLOT(downloadDone()));
}

QString OnlineSearchOCLCWorldCat::label() const {
    return i18n("OCLC WorldCat");
}

OnlineSearchQueryFormAbstract *OnlineSearchOCLCWorldCat::customWidget(QWidget *) {
    return NULL;
}

QUrl OnlineSearchOCLCWorldCat::homepage() const {
    return QUrl(QStringLiteral("http://www.worldcat.org/"));
}

void OnlineSearchOCLCWorldCat::cancel() {
    OnlineSearchAbstract::cancel();
}

QString OnlineSearchOCLCWorldCat::favIconUrl() const {
    return QStringLiteral("http://www.worldcat.org/favicon.ico");
}

void OnlineSearchOCLCWorldCat::downloadDone() {
    emit progress(++d->curStep, d->maxSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// Ensure proper treatment of UTF-8 characters
        const QString atomCode = QString::fromUtf8(reply->readAll().constData()).remove(QStringLiteral("xmlns=\"http://www.w3.org/2005/Atom\"")).remove(QStringLiteral(" xmlns=\"http://www.loc.gov/zing/srw/\"")); // FIXME fix worldcatdc2bibtex.xsl to handle namespace

        /// Use XSL transformation to get BibTeX document from XML result
        const QString bibTeXcode = d->xslt->transform(atomCode).remove(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                hasEntries |= publishEntry(entry);
            }
            delete bibtexFile;

            if (!hasEntries) {
                emit progress(d->maxSteps, d->maxSteps);
                stopSearch(resultNoError);
            } else if (d->curStep < d->maxSteps) {
                QUrl nextUrl = d->baseUrl;
                QUrlQuery query(nextUrl);
                query.addQueryItem(QStringLiteral("start"), QString::number(d->curStep *
                                   OnlineSearchOCLCWorldCat::Private::countPerStep + 1));
                nextUrl.setQuery(query);
                QNetworkRequest request(nextUrl);
                QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
                InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
                connect(newReply, &QNetworkReply::finished, this, SLOT(downloadDone()));
            } else {
                emit progress(d->maxSteps, d->maxSteps);
                stopSearch(resultNoError);
            }
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toDisplayString();
            stopSearch(resultUnspecifiedError);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}
