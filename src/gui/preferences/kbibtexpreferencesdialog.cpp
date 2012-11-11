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

#include <QSet>
#include <QFileInfo>

#include <KLocale>
#include <KComboBox>
#include <KStandardDirs>
#include <KIO/NetAccess>
#include <KDebug>
#include <KMessageBox>
#include <KGuiItem>

#include "notificationhub.h"
#include "settingsgeneralwidget.h"
#include "settingsglobalkeywordswidget.h"
#include "settingsfileexporterpdfpswidget.h"
#include "settingsfileexporterwidget.h"
#include "settingscolorlabelwidget.h"
#include "settingsuserinterfacewidget.h"
#include "settingsidsuggestionswidget.h"
#include "kbibtexpreferencesdialog.h"

class KBibTeXPreferencesDialog::KBibTeXPreferencesDialogPrivate
{
private:
    KBibTeXPreferencesDialog *p;
    KComboBox *listOfEncodings;
    QSet<SettingsAbstractWidget *> settingWidgets;

public:
    bool notifyOfChanges;

    KBibTeXPreferencesDialogPrivate(KBibTeXPreferencesDialog *parent)
            : p(parent) {
        notifyOfChanges = false;
    }

    void addPages() {
        SettingsAbstractWidget *settingsWidget = new SettingsGeneralWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *pageGlobal = p->addPage(settingsWidget, i18n("General"));
        pageGlobal->setIcon(KIcon("kbibtex"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsGlobalKeywordsWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *page = p->addSubPage(pageGlobal, settingsWidget, i18n("Keywords"));
        page->setIcon(KIcon("checkbox")); // TODO find better icon
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsColorLabelWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageGlobal, settingsWidget, i18n("Color & Labels"));
        page->setIcon(KIcon("preferences-desktop-color"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsIdSuggestionsWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addSubPage(pageGlobal, settingsWidget, i18n("Id Suggestions"));
        page->setIcon(KIcon("view-filter"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsUserInterfaceWidget(p);
        settingWidgets.insert(settingsWidget);
        page = p->addPage(settingsWidget, i18n("User Interface"));
        page->setIcon(KIcon("user-identity"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));

        settingsWidget = new SettingsFileExporterWidget(p);
        settingWidgets.insert(settingsWidget);
        KPageWidgetItem *pageSaving = p->addPage(settingsWidget, i18n("Saving and Exporting"));
        pageSaving->setIcon(KIcon("document-save"));
        connect(settingsWidget, SIGNAL(changed()), p, SLOT(gotChanged()));
    }

    void loadState() {
        foreach(SettingsAbstractWidget *settingsWidget, settingWidgets) {
            settingsWidget->loadState();
        }
    }

    void saveState() {
        foreach(SettingsAbstractWidget *settingsWidget, settingWidgets) {
            settingsWidget->saveState();
        }
    }

    void resetToDefaults() {
        switch (KMessageBox::warningYesNoCancel(p, i18n("This will reset the settings to factory defaults. Should this affect only the current page or all settings?"), i18n("Reset to Defaults"), KGuiItem(i18n("All settings"), QLatin1String("edit-undo")), KGuiItem(i18n("Only current page"), QLatin1String("document-revert")))) {
        case KMessageBox::Yes:
            foreach(SettingsAbstractWidget *settingsWidget, settingWidgets) {
                settingsWidget->resetToDefaults();
            }
            break;
        case KMessageBox::No: {
            SettingsAbstractWidget *widget = dynamic_cast<SettingsAbstractWidget *>(p->currentPage()->widget());
            if (widget != NULL)
                widget->resetToDefaults();
        }
        break;
        }
    }

    KIcon iconFromFavicon(const QString &url) {
        static const QRegExp invalidChars("[^-a-z0-9_]", Qt::CaseInsensitive);
        QString fileName = url;
        fileName = fileName.replace(invalidChars, "");
        fileName.prepend(KStandardDirs::locateLocal("cache", "favicons/")).append(".png");

        if (!QFileInfo(fileName).exists()) {
            if (!KIO::NetAccess::file_copy(KUrl(url), KUrl(fileName), NULL))
                return KIcon();
        }

        kDebug() << "icon fileName" << fileName;
        return KIcon(fileName);
    }
};

KBibTeXPreferencesDialog::KBibTeXPreferencesDialog(QWidget *parent, Qt::WFlags flags)
        : KPageDialog(parent, flags), d(new KBibTeXPreferencesDialogPrivate(this))
{
    setFaceType(KPageDialog::Tree);
    setWindowTitle(i18n("Preferences"));
    setButtons(Default | Reset | Ok | Apply | Cancel);
    setDefaultButton(Ok);
    enableButtonApply(false);
    setModal(true);
    showButtonSeparator(true);

    connect(this, SIGNAL(applyClicked()), this, SLOT(apply()));
    connect(this, SIGNAL(okClicked()), this, SLOT(ok()));
    connect(this, SIGNAL(defaultClicked()), this, SLOT(resetToDefaults()));
    connect(this, SIGNAL(resetClicked()), this, SLOT(reset()));

    d->addPages();
}

KBibTeXPreferencesDialog::~KBibTeXPreferencesDialog()
{
    delete d;
}

void KBibTeXPreferencesDialog::hideEvent(QHideEvent *)
{
    if (d->notifyOfChanges)
        NotificationHub::publishEvent(NotificationHub::EventConfigurationChanged);
}

void KBibTeXPreferencesDialog::apply()
{
    enableButtonApply(false);
    d->saveState();
    d->notifyOfChanges = true;
}

void KBibTeXPreferencesDialog::reset()
{
    enableButtonApply(false);
    d->loadState();
}

void KBibTeXPreferencesDialog::ok()
{
    d->notifyOfChanges = true;
    apply();
    accept();
}

void KBibTeXPreferencesDialog::resetToDefaults()
{
    d->resetToDefaults();
}

void KBibTeXPreferencesDialog::gotChanged()
{
    enableButtonApply(true);
}
