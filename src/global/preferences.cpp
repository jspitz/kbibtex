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

#include "preferences.h"

#include <QDebug>

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#else // HAVE_KF5
#define I18N_NOOP(text) QObject::tr(text)
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#ifdef HAVE_KF5
#include "notificationhub.h"
#endif // HAVE_KF5

const QString Preferences::groupColor = QStringLiteral("Color Labels");
const QString Preferences::keyColorCodes = QStringLiteral("colorCodes");
const QStringList Preferences::defaultColorCodes {QStringLiteral("#cc3300"), QStringLiteral("#0033ff"), QStringLiteral("#009966"), QStringLiteral("#f0d000")};
const QString Preferences::keyColorLabels = QStringLiteral("colorLabels");
// FIXME
// clazy warns: QString(const char*) being called [-Wclazy-qstring-uneeded-heap-allocations]
// ... but using QStringLiteral may break the translation process?
const QStringList Preferences::defaultColorLabels {I18N_NOOP("Important"), I18N_NOOP("Unread"), I18N_NOOP("Read"), I18N_NOOP("Watch")};

const QString Preferences::groupGeneral = QStringLiteral("General");
const QString Preferences::keyBackupScope = QStringLiteral("backupScope");
const Preferences::BackupScope Preferences::defaultBackupScope = LocalOnly;
const QString Preferences::keyNumberOfBackups = QStringLiteral("numberOfBackups");
const int Preferences::defaultNumberOfBackups = 5;

const QString Preferences::groupUserInterface = QStringLiteral("User Interface");
const QString Preferences::keyElementDoubleClickAction = QStringLiteral("elementDoubleClickAction");
const Preferences::ElementDoubleClickAction Preferences::defaultElementDoubleClickAction = ActionOpenEditor;

const QString Preferences::keyEncoding = QStringLiteral("encoding");
const QString Preferences::defaultEncoding = QStringLiteral("LaTeX");
const QString Preferences::keyStringDelimiter = QStringLiteral("stringDelimiter");
const QString Preferences::defaultStringDelimiter = QStringLiteral("{}");
const QString Preferences::keyQuoteComment = QStringLiteral("quoteComment");
const Preferences::QuoteComment Preferences::defaultQuoteComment = qcNone;
const QString Preferences::keyKeywordCasing = QStringLiteral("keywordCasing");
const KBibTeX::Casing Preferences::defaultKeywordCasing = KBibTeX::cLowerCase;
const QString Preferences::keyProtectCasing = QStringLiteral("protectCasing");
const Qt::CheckState Preferences::defaultProtectCasing = Qt::PartiallyChecked;
const QString Preferences::keyListSeparator = QStringLiteral("ListSeparator");
const QString Preferences::defaultListSeparator = QStringLiteral("; ");

/**
 * Preferences for Data objects
 */
const QString Preferences::keyPersonNameFormatting = QStringLiteral("personNameFormatting");
const QString Preferences::personNameFormatLastFirst = QStringLiteral("<%l><, %s><, %f>");
const QString Preferences::personNameFormatFirstLast = QStringLiteral("<%f ><%l>< %s>");
const QString Preferences::defaultPersonNameFormatting = personNameFormatLastFirst;

const Preferences::BibliographySystem Preferences::defaultBibliographySystem = Preferences::BibTeX;

Preferences::BibliographySystem Preferences::bibliographySystem()
{
#ifdef HAVE_KF5
    static KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
    static const KConfigGroup configGroup(config, QStringLiteral("General"));
    config->reparseConfiguration();
    const int index = configGroup.readEntry(QStringLiteral("BibliographySystem"), static_cast<int>(defaultBibliographySystem));
    if (index != static_cast<int>(Preferences::BibTeX) && index != static_cast<int>(Preferences::BibLaTeX)) {
        qWarning() << "Configuration file setting for Bibliography System has an invalid value, using default as fallback";
        setBibliographySystem(defaultBibliographySystem);
        return defaultBibliographySystem;
    } else
        return static_cast<Preferences::BibliographySystem>(index);
#else // HAVE_KF5
    return defaultBibliographySystem;
#endif // HAVE_KF5
}

bool Preferences::setBibliographySystem(const Preferences::BibliographySystem bibliographySystem)
{
#ifdef HAVE_KF5
    static KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
    static KConfigGroup configGroup(config, QStringLiteral("General"));
    config->reparseConfiguration();
    const int prevIndex = configGroup.readEntry(QStringLiteral("BibliographySystem"), static_cast<int>(defaultBibliographySystem));
    const int newIndex = static_cast<int>(bibliographySystem);
    if (prevIndex == newIndex) return false; /// If old and new bibliography system are the same, return 'false' directly
    configGroup.writeEntry(QStringLiteral("BibliographySystem"), newIndex);
    config->sync();
//    NotificationHub::publishEvent(NotificationHub::EventBibliographySystemChanged);
#else // HAVE_KF5
    Q_UNUSED(bibliographySystem);
#endif // HAVE_KF5
    return true;
}

const QMap<Preferences::BibliographySystem, QString> Preferences::availableBibliographySystems()
{
    static const QMap<Preferences::BibliographySystem, QString> result {{Preferences::BibTeX, i18n("BibTeX")}, {Preferences::BibLaTeX, i18n("BibLaTeX")}};
    return result;
}
