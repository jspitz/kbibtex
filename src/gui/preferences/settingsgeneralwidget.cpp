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

#include <QFormLayout>

#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KComboBox>

#include <value.h>
#include "settingsgeneralwidget.h"

class SettingsGeneralWidget::SettingsGeneralWidgetPrivate
{
private:
    SettingsGeneralWidget *p;

    KComboBox *comboBoxPersonNameFormatting;
    static const Person *dummyPerson;
    QString restartRequiredMsg;

    KSharedConfigPtr config;
    static const QString configGroupName;

public:

    SettingsGeneralWidgetPrivate(SettingsGeneralWidget *parent)
            : p(parent), restartRequiredMsg(i18n("Changing this option requires a restart to take effect.")), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        QString personNameFormatting = configGroup.readEntry(Person::keyPersonNameFormatting, Person::defaultPersonNameFormatting);
        p->selectValue(comboBoxPersonNameFormatting, Person::transcribePersonName(dummyPerson, personNameFormatting));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(Person::keyPersonNameFormatting, comboBoxPersonNameFormatting->itemData(comboBoxPersonNameFormatting->currentIndex()));
        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxPersonNameFormatting, Person::transcribePersonName(dummyPerson, Person::defaultPersonNameFormatting));
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxPersonNameFormatting = new KComboBox(false, p);
        layout->addRow(i18n("Person Names Formatting:"), comboBoxPersonNameFormatting);
        comboBoxPersonNameFormatting->addItem(Person::transcribePersonName(dummyPerson, QLatin1String("<%f ><%l>")), QString("<%f ><%l>"));
        comboBoxPersonNameFormatting->addItem(Person::transcribePersonName(dummyPerson, QLatin1String("<%l><, %f>")), QString("<%l><, %f>"));
        comboBoxPersonNameFormatting->setToolTip(restartRequiredMsg);
    }
};

const QString SettingsGeneralWidget::SettingsGeneralWidgetPrivate::configGroupName = QLatin1String("General");
const Person *SettingsGeneralWidget::SettingsGeneralWidgetPrivate::dummyPerson = new Person(i18n("John"), i18n("Doe"), i18n("J."), i18n("Jr."));

SettingsGeneralWidget::SettingsGeneralWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsGeneralWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

void SettingsGeneralWidget::loadState()
{
    d->loadState();
}

void SettingsGeneralWidget::saveState()
{
    d->saveState();
}

void SettingsGeneralWidget::resetToDefaults()
{
    d->resetToDefaults();
}