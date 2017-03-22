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

#include "checkbibtex.h"

#include <QApplication>
#include <QBuffer>
#include <QTextStream>

#include <KLocalizedString>
#include <KMessageBox>

#include "fileexporterbibtexoutput.h"
#include "file.h"
#include "entry.h"
#include "element.h"
#include "macro.h"

CheckBibTeX::CheckBibTeXResult CheckBibTeX::checkBibTeX(QSharedPointer<Element> &element, const File *file, QWidget *parent)
{
    /// only entries are supported, no macros, preambles, ...
    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (entry.isNull())
        return InvalidData;
    else
        return checkBibTeX(entry, file, parent);
}

CheckBibTeX::CheckBibTeXResult CheckBibTeX::checkBibTeX(QSharedPointer<Entry> &entry, const File *file, QWidget *parent)
{
    /// disable GUI under process
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    /// use a dummy BibTeX file to collect all elements necessary for check
    File dummyFile;

    /// create temporary entry to work with
    dummyFile << entry;

    /// fetch and inser crossref'ed entry
    QString crossRefStr;
    Value crossRefVal = entry->value(Entry::ftCrossRef);
    if (!crossRefVal.isEmpty() && file != nullptr) {
        crossRefStr = PlainTextValue::text(crossRefVal);
        QSharedPointer<Entry> crossRefDest = file->containsKey(crossRefStr, File::etEntry).dynamicCast<Entry>();
        if (!crossRefDest.isNull())
            dummyFile << crossRefDest;
        else
            crossRefStr.clear(); /// memorize crossref'ing failed
    }

    /// include all macro definitions, in case they are referenced
    if (file != nullptr)
        for (const auto &element : const_cast<const File &>(*file))
            if (Macro::isMacro(*element))
                dummyFile << element;

    /// run special exporter to get BibTeX's output
    QStringList bibtexOuput;
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    FileExporterBibTeXOutput exporter(FileExporterBibTeXOutput::BibTeXLogFile, parent);
    bool exporterResult = exporter.save(&buffer, &dummyFile, &bibtexOuput);
    buffer.close();

    if (!exporterResult) {
        QApplication::restoreOverrideCursor();
        KMessageBox::errorList(parent, i18n("Running BibTeX failed.\n\nSee the following output to trace the error:"), bibtexOuput, i18n("Running BibTeX failed."));
        return FailedToCheck;
    }

    /// define variables how to parse BibTeX's output
    static const QString warningStart = QStringLiteral("Warning--");
    static const QRegExp warningEmptyField("empty (\\w+) in ");
    static const QRegExp warningEmptyField2("empty (\\w+) or (\\w+) in ");
    static const QRegExp warningThereIsBut("there's a (\\w+) but no (\\w+) in");
    static const QRegExp warningCantUseBoth("can't use both (\\w+) and (\\w+) fields");
    static const QRegExp warningSort2("to sort, need (\\w+) or (\\w+) in ");
    static const QRegExp warningSort3("to sort, need (\\w+), (\\w+), or (\\w+) in ");
    static const QRegExp errorLine("---line (\\d+)");

    /// go line-by-line through BibTeX output and collect warnings/errors
    QStringList warnings;
    QString errorPlainText;
    for (const QString &line : const_cast<const QStringList &>(bibtexOuput)) {
        if (errorLine.indexIn(line) > -1) {
            buffer.open(QIODevice::ReadOnly);
            QTextStream ts(&buffer);
            for (int i = errorLine.cap(1).toInt(); i > 1; --i) {
                errorPlainText = ts.readLine();
                buffer.close();
            }
        } else if (line.startsWith(QStringLiteral("Warning--"))) {
            /// is a warning ...

            if (warningEmptyField.indexIn(line) > -1) {
                /// empty/missing field
                warnings << i18n("Field <b>%1</b> is empty", warningEmptyField.cap(1));
            } else if (warningEmptyField2.indexIn(line) > -1) {
                /// two empty/missing fields
                warnings << i18n("Fields <b>%1</b> and <b>%2</b> are empty, but at least one is required", warningEmptyField2.cap(1), warningEmptyField2.cap(2));
            } else if (warningThereIsBut.indexIn(line) > -1) {
                /// there is a field which exists but another does not exist
                warnings << i18n("Field <b>%1</b> exists, but <b>%2</b> does not exist", warningThereIsBut.cap(1), warningThereIsBut.cap(2));
            } else if (warningCantUseBoth.indexIn(line) > -1) {
                /// there are two conflicting fields, only one may be used
                warnings << i18n("Fields <b>%1</b> and <b>%2</b> cannot be used at the same time", warningCantUseBoth.cap(1), warningCantUseBoth.cap(2));
            } else if (warningSort2.indexIn(line) > -1) {
                /// one out of two fields missing for sorting
                warnings << i18n("Fields <b>%1</b> or <b>%2</b> are required to sort entry", warningSort2.cap(1), warningSort2.cap(2));
            } else if (warningSort3.indexIn(line) > -1) {
                /// one out of three fields missing for sorting
                warnings << i18n("Fields <b>%1</b>, <b>%2</b>, <b>%3</b> are required to sort entry", warningSort3.cap(1), warningSort3.cap(2), warningSort3.cap(3));
            } else {
                /// generic/unknown warning
                warnings << i18n("Unknown warning: %1", line.mid(warningStart.length()));
            }
        }
    }

    CheckBibTeXResult result = NoProblem;
    QApplication::restoreOverrideCursor();
    if (!errorPlainText.isEmpty()) {
        result = BibTeXWarning;
        KMessageBox::information(parent, i18n("<qt><p>The following error was found:</p><pre>%1</pre></qt>", errorPlainText), i18n("Errors found"));
    } else if (!warnings.isEmpty()) {
        KMessageBox::informationList(parent, i18n("The following warnings were found:"), warnings, i18n("Warnings found"));
        result = BibTeXError;
    } else
        KMessageBox::information(parent, i18n("No warnings or errors were found.%1", crossRefStr.isEmpty() ? QString() : i18n("\n\nSome fields missing in this entry were taken from the crossref'ed entry '%1'.", crossRefStr)), i18n("No Errors or Warnings"));

    return result;
}
