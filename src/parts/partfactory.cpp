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

#include "partfactory.h"

#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>

#include "part.h"
#include "version.h"
#include "logging_parts.h"

class KBibTeXPartFactory::Private
{
public:
    KAboutData aboutData;

    Private()
            : aboutData(QStringLiteral("kbibtexpart"), i18n("KBibTeXPart"), QLatin1String(versionNumber), i18n("A BibTeX editor by KDE"), KAboutLicense::GPL_V2, i18n("Copyright 2004-2017 Thomas Fischer"), QString(), QStringLiteral("https://userbase.kde.org/KBibTeX"))
    {
        aboutData.setOrganizationDomain(QByteArray("kde.org"));
        aboutData.setDesktopFileName(QStringLiteral("org.kde.kbibtex"));
        aboutData.addAuthor(i18n("Thomas Fischer"), i18n("Maintainer"), QStringLiteral("fischer@unix-ag.uni-kl.de"));
    }
};

KBibTeXPartFactory::KBibTeXPartFactory()
        : KPluginFactory(), d(new KBibTeXPartFactory::Private())
{
    /// nothing
}

KBibTeXPartFactory::~KBibTeXPartFactory() {
    delete d;
}

QObject *KBibTeXPartFactory::create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString &keyword) {
    Q_UNUSED(iface);
    Q_UNUSED(args)
    Q_UNUSED(keyword);

    KBibTeXPart *part = new KBibTeXPart(parentWidget, parent, d->aboutData);
    return part;
}
