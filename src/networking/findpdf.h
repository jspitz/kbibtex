/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#ifndef KBIBTEX_NETWORKING_FINDPDF_H
#define KBIBTEX_NETWORKING_FINDPDF_H

#include "kbibtexnetworking_export.h"

#include <QObject>
#include <QList>
#include <QSet>
#include <QUrl>

#include "entry.h"

class QNetworkAccessManager;
class QNetworkReply;
class KTemporaryFile;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT FindPDF : public QObject
{
    Q_OBJECT

public:
    enum DownloadMode {NoDownload = 0, Download, URLonly};

    typedef struct {
        QUrl url;
        QString textPreview;
        KTemporaryFile *tempFilename;
        float relevance;
        DownloadMode downloadMode;
    } ResultItem;

    FindPDF(QObject *parent = NULL);

    bool search(const Entry &entry);
    QList<ResultItem> results();

signals:
    void finished();

private slots:
    void downloadFinished();

private:
    static int fileCounter;

    int aliveCounter;
    QList<ResultItem> m_result;
    Entry m_currentEntry;
    QSet<QUrl> m_knownUrls;

    bool queueUrl(const QUrl &url, const QString &term, const QString &origin, int depth);
    void processGeneralHTML(QNetworkReply *reply, const QString &text);
    void processGoogleResult(QNetworkReply *reply, const QString &text);
    void processSpringerLink(QNetworkReply *reply, const QString &text);
    void processCiteSeerX(QNetworkReply *reply, const QString &text);
    void processPDF(QNetworkReply *reply, const QByteArray &data);
};

#endif // KBIBTEX_NETWORKING_FINDPDF_H
