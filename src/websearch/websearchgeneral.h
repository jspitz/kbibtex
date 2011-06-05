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
#ifndef KBIBTEX_WEBSEARCH_GENERAL_H
#define KBIBTEX_WEBSEARCH_GENERAL_H

#include <KSharedConfig>

#include "websearchabstract.h"

class QSpinBox;

class KLineEdit;

class KBIBTEXWS_EXPORT WebSearchQueryFormGeneral : public WebSearchQueryFormAbstract
{
    Q_OBJECT

public:
    WebSearchQueryFormGeneral(QWidget *parent);

    bool readyToStart() const;

    QMap<QString, QString> getQueryTerms();
    int getNumResults();

private:
    QMap<QString, KLineEdit*> queryFields;
    QSpinBox *numResultsField;
    const QString configGroupName;

    void loadState();
    void saveState();
};

#endif // KBIBTEX_WEBSEARCH_GENERAL_H
