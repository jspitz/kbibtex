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

#ifndef KBIBTEX_GUI_SETTINGSUSERINTERFACEWIDGET_H
#define KBIBTEX_GUI_SETTINGSUSERINTERFACEWIDGET_H

#include <kbibtexgui_export.h>

#include "settingsabstractwidget.h"

/**
@author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT SettingsUserInterfaceWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsUserInterfaceWidget(QWidget *parent);
    ~SettingsUserInterfaceWidget() override;

    QString label() const override;
    QIcon icon() const override;

public slots:
    void loadState() override;
    void saveState() override;
    void resetToDefaults() override;

private:
    class SettingsUserInterfaceWidgetPrivate;
    SettingsUserInterfaceWidgetPrivate *d;
};


#endif // KBIBTEX_GUI_SETTINGSUSERINTERFACEWIDGET_H
