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
#include <QLabel>
#include <QSpinBox>

#include <KLineEdit>
#include <KLocale>
#include <KConfigGroup>

#include "entry.h"
#include "onlinesearchgeneral.h"

OnlineSearchQueryFormGeneral::OnlineSearchQueryFormGeneral(QWidget *parent)
        : OnlineSearchQueryFormAbstract(parent),
        configGroupName(QLatin1String("Search Engine General"))
{
    QFormLayout *layout = new QFormLayout(this);
    layout->setMargin(0);

    QLabel *label = new QLabel(i18n("Free text:"), this);
    KLineEdit *lineEdit = new KLineEdit(this);
    layout->addRow(label, lineEdit);
    lineEdit->setClearButtonShown(true);
    lineEdit->setFocus(Qt::TabFocusReason);
    queryFields.insert(OnlineSearchAbstract::queryKeyFreeText, lineEdit);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Title:"), this);
    lineEdit = new KLineEdit(this);
    layout->addRow(label, lineEdit);
    lineEdit->setClearButtonShown(true);
    queryFields.insert(OnlineSearchAbstract::queryKeyTitle, lineEdit);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Author:"), this);
    lineEdit = new KLineEdit(this);
    layout->addRow(label, lineEdit);
    lineEdit->setClearButtonShown(true);
    queryFields.insert(OnlineSearchAbstract::queryKeyAuthor, lineEdit);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Year:"), this);
    lineEdit = new KLineEdit(this);
    layout->addRow(label, lineEdit);
    lineEdit->setClearButtonShown(true);
    queryFields.insert(OnlineSearchAbstract::queryKeyYear, lineEdit);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Number of Results:"), this);
    numResultsField = new QSpinBox(this);
    layout->addRow(label, numResultsField);
    numResultsField->setMinimum(3);
    numResultsField->setMaximum(100);
    numResultsField->setValue(20);
    label->setBuddy(numResultsField);

    loadState();
}

bool OnlineSearchQueryFormGeneral::readyToStart() const
{
    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it)
        if (!it.value()->text().isEmpty())
            return true;

    return false;
}

void OnlineSearchQueryFormGeneral::copyFromEntry(const Entry &entry)
{
    queryFields[OnlineSearchAbstract::queryKeyFreeText]->setText("");
    queryFields[OnlineSearchAbstract::queryKeyTitle]->setText(PlainTextValue::text(entry[Entry::ftTitle]));
    queryFields[OnlineSearchAbstract::queryKeyAuthor]->setText(authorLastNames(entry).join(" "));
    queryFields[OnlineSearchAbstract::queryKeyYear]->setText(PlainTextValue::text(entry[Entry::ftYear]));
}

QMap<QString, QString> OnlineSearchQueryFormGeneral::getQueryTerms()
{
    QMap<QString, QString> result;

    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
        if (!it.value()->text().isEmpty())
            result.insert(it.key(), it.value()->text());
    }

    saveState();
    return result;
}

int OnlineSearchQueryFormGeneral::getNumResults()
{
    return numResultsField->value();
}

void OnlineSearchQueryFormGeneral::loadState()
{
    KConfigGroup configGroup(config, configGroupName);
    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
        it.value()->setText(configGroup.readEntry(it.key(), QString()));
    }
    numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
}

void OnlineSearchQueryFormGeneral::saveState()
{
    KConfigGroup configGroup(config, configGroupName);
    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
        configGroup.writeEntry(it.key(), it.value()->text());
    }
    configGroup.writeEntry(QLatin1String("numResults"), numResultsField->value());
    config->sync();
}