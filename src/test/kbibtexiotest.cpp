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

#include <QtTest/QtTest>

#include "encoderxml.h"
#include "value.h"
#include "fileimporter.h"

class KBibTeXIOTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void encoderXMLdecode_data();
    void encoderXMLdecode();
    void encoderXMLencode_data();
    void encoderXMLencode();
    void fileImporterSplitName_data();
    void fileImporterSplitName();

private:
};

void KBibTeXIOTest::encoderXMLdecode_data()
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QString>("unicode");

    QTest::newRow("Just ASCII") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.");
    QTest::newRow("Quotation marks") << QStringLiteral("Caesar said: &quot;Veni, vidi, vici&quot;") << QStringLiteral("Caesar said: \"Veni, vidi, vici\"");
    QTest::newRow("Characters from EncoderXMLCharMapping") << QStringLiteral("&quot;&amp;&lt;&gt;") << QStringLiteral("\"\\&<>");
    QTest::newRow("Characters from backslashSymbols") << QStringLiteral("&amp;%_") << QStringLiteral("\\&\\%\\_");

    for (int start = 0; start < 10; ++start) {
        QString xmlString, unicodeString;
        for (int offset = 1561; offset < 6791; offset += 621) {
            const int unicode = start * 3671 + offset;
            xmlString += QStringLiteral("&#") + QString::number(unicode) + QStringLiteral(";");
            unicodeString += QChar(unicode);
        }
        QTest::newRow(QString(QStringLiteral("Some arbitary Unicode characters (%1)")).arg(start).toLatin1().constData()) << xmlString << unicodeString;
    }
}

void KBibTeXIOTest::encoderXMLdecode()
{
    QFETCH(QString, xml);
    QFETCH(QString, unicode);

    QCOMPARE(EncoderXML::instance().decode(xml), unicode);
}

void KBibTeXIOTest::encoderXMLencode_data()
{
    encoderXMLdecode_data();
}

void KBibTeXIOTest::encoderXMLencode()
{
    QFETCH(QString, xml);
    QFETCH(QString, unicode);

    QCOMPARE(EncoderXML::instance().encode(unicode, Encoder::TargetEncodingASCII), xml);
}

void KBibTeXIOTest::fileImporterSplitName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<Person *>("person");

    QTest::newRow("Empty name") << QStringLiteral("") << new Person(QString(), QString(), QString());
    QTest::newRow("PubMed style") << QStringLiteral("Jones A B C") << new Person(QStringLiteral("A B C"), QStringLiteral("Jones"), QString());
    QTest::newRow("Just last name") << QStringLiteral("Dido") << new Person(QString(), QStringLiteral("Dido"), QString());
    QTest::newRow("Name with 'von'") << QStringLiteral("Theodor von Sickel") << new Person(QStringLiteral("Theodor"), QStringLiteral("von Sickel"), QString());
    QTest::newRow("Name with 'von', reversed") << QStringLiteral("von Sickel, Theodor") << new Person(QStringLiteral("Theodor"), QStringLiteral("von Sickel"), QString());
    QTest::newRow("Name with 'van der'") << QStringLiteral("Adriaen van der Werff") << new Person(QStringLiteral("Adriaen"), QStringLiteral("van der Werff"), QString());
    QTest::newRow("Name with 'van der', reversed") << QStringLiteral("van der Werff, Adriaen") << new Person(QStringLiteral("Adriaen"), QStringLiteral("van der Werff"), QString());
    QTest::newRow("Name with suffix") << QStringLiteral("Anna Eleanor Roosevelt Jr.") << new Person(QStringLiteral("Anna Eleanor"), QStringLiteral("Roosevelt"), QStringLiteral("Jr."));
}

void KBibTeXIOTest::fileImporterSplitName()
{
    QFETCH(QString, name);
    QFETCH(Person *, person);

    Person *computedPerson = FileImporter::splitName(name);
    QCOMPARE(*computedPerson, *person);

    delete person;
    delete computedPerson;
}

void KBibTeXIOTest::initTestCase()
{
    /// nothing
}

QTEST_MAIN(KBibTeXIOTest)

#include "kbibtexiotest.moc"
