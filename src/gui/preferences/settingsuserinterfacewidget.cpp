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

#include "settingsuserinterfacewidget.h"

#include <QFormLayout>
#include <QCheckBox>
#include <QBoxLayout>

#include <KComboBox>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include "preferences.h"
#include "elementwidgets.h"
#include "models/filemodel.h"

class SettingsUserInterfaceWidget::SettingsUserInterfaceWidgetPrivate
{
private:
    SettingsUserInterfaceWidget *p;

    QCheckBox *checkBoxShowComments;
    QCheckBox *checkBoxShowMacros;
    QCheckBox *checkBoxShowXDatas;
    KComboBox *comboBoxBibliographySystem;
    KComboBox *comboBoxElementDoubleClickAction;

    KSharedConfigPtr config;
    static const QString configGroupName;

public:
    SettingsUserInterfaceWidgetPrivate(SettingsUserInterfaceWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))) {
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        checkBoxShowComments->setChecked(configGroup.readEntry(FileModel::keyShowComments, FileModel::defaultShowComments));
        checkBoxShowMacros->setChecked(configGroup.readEntry(FileModel::keyShowMacros, FileModel::defaultShowMacros));
        checkBoxShowXDatas->setChecked(configGroup.readEntry(FileModel::keyShowXDatas, FileModel::defaultShowXDatas));

        int styleIndex = comboBoxBibliographySystem->findData(configGroup.readEntry("CurrentStyle", QString(QStringLiteral("bibtex"))));
        if (styleIndex < 0) styleIndex = 0;
        if (styleIndex < comboBoxBibliographySystem->count()) comboBoxBibliographySystem->setCurrentIndex(styleIndex);

        comboBoxElementDoubleClickAction->setCurrentIndex(configGroup.readEntry(Preferences::keyElementDoubleClickAction, static_cast<int>(Preferences::defaultElementDoubleClickAction)));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(FileModel::keyShowComments, checkBoxShowComments->isChecked());
        configGroup.writeEntry(FileModel::keyShowMacros, checkBoxShowMacros->isChecked());
        configGroup.writeEntry(FileModel::keyShowXDatas, checkBoxShowXDatas->isChecked());
        configGroup.writeEntry("CurrentStyle", comboBoxBibliographySystem->itemData(comboBoxBibliographySystem->currentIndex()).toString());
        configGroup.writeEntry(Preferences::keyElementDoubleClickAction, comboBoxElementDoubleClickAction->currentIndex());
        config->sync();
    }

    void resetToDefaults() {
        checkBoxShowComments->setChecked(FileModel::defaultShowComments);
        checkBoxShowMacros->setChecked(FileModel::defaultShowMacros);
        checkBoxShowXDatas->setChecked(FileModel::defaultShowXDatas);
        comboBoxBibliographySystem->setCurrentIndex(0);
        comboBoxElementDoubleClickAction->setCurrentIndex(Preferences::defaultElementDoubleClickAction);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        checkBoxShowComments = new QCheckBox(p);
        layout->addRow(i18n("Show Comments:"), checkBoxShowComments);
        connect(checkBoxShowComments, &QCheckBox::toggled, p, &SettingsUserInterfaceWidget::changed);

        checkBoxShowMacros = new QCheckBox(p);
        layout->addRow(i18n("Show Macros:"), checkBoxShowMacros);
        connect(checkBoxShowMacros, &QCheckBox::toggled, p, &SettingsUserInterfaceWidget::changed);

        checkBoxShowXDatas = new QCheckBox(p);
        layout->addRow(i18n("Show XData Entries:"), checkBoxShowXDatas);
        connect(checkBoxShowXDatas, SIGNAL(toggled(bool)), p, SIGNAL(changed()));

        comboBoxBibliographySystem = new KComboBox(p);
        comboBoxBibliographySystem->setObjectName(QStringLiteral("comboBoxBibtexStyle"));
        layout->addRow(i18n("Bibliography System:"), comboBoxBibliographySystem);
        connect(comboBoxBibliographySystem, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsUserInterfaceWidget::changed);

        comboBoxElementDoubleClickAction = new KComboBox(p);
        comboBoxElementDoubleClickAction->setObjectName(QStringLiteral("comboBoxElementDoubleClickAction"));
        comboBoxElementDoubleClickAction->addItem(i18n("Open Editor")); ///< ActionOpenEditor = 0
        comboBoxElementDoubleClickAction->addItem(i18n("View Document")); ///< ActionViewDocument = 1
        layout->addRow(i18n("When double-clicking an element:"), comboBoxElementDoubleClickAction);
        connect(comboBoxElementDoubleClickAction, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), p, &SettingsUserInterfaceWidget::changed);
    }
};

const QString SettingsUserInterfaceWidget::SettingsUserInterfaceWidgetPrivate::configGroupName = QStringLiteral("User Interface");


SettingsUserInterfaceWidget::SettingsUserInterfaceWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsUserInterfaceWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

QString SettingsUserInterfaceWidget::label() const
{
    return i18n("User Interface");
}

QIcon SettingsUserInterfaceWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("user-identity"));
}

SettingsUserInterfaceWidget::~SettingsUserInterfaceWidget()
{
    delete d;
}

void SettingsUserInterfaceWidget::loadState()
{
    d->loadState();
}

void SettingsUserInterfaceWidget::saveState()
{
    d->saveState();
}

void SettingsUserInterfaceWidget::resetToDefaults()
{
    d->resetToDefaults();
}
