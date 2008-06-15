/***************************************************************************
*   Copyright (C) 2004-2006 by Thomas Fischer                             *
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
#ifndef BIBTEXFILEIMPORTERBIBTEX_H
#define BIBTEXFILEIMPORTERBIBTEX_H

#include <QTextStream>

#include <fileimporter.h>
#include <entryfield.h>

namespace KBibTeX
{
namespace IO {

class Element;
class Comment;
class Preamble;
class Macro;
class Entry;
class Value;

/**
@author Thomas Fischer
*/
class FileImporterBibTeX : public FileImporter
{
public:
    FileImporterBibTeX(const QString& = "utf-8", bool = true);
    ~FileImporterBibTeX();

    File* load(QIODevice *iodevice);
    static bool guessCanDecode(const QString & text);

public slots:
    void cancel();

private:
    enum Token {
        tAt, tBracketOpen, tBracketClose, tAlphaNumText, tComma, tSemicolon, tAssign, tDoublecross, tEOF, tUnknown
    };

    bool cancelFlag;
    QTextStream *m_textStream;
    QChar m_currentChar;
    bool m_ignoreComments;
    char *m_lineBuffer;
    const int m_lineBufferSize;
    QString m_encoding;
    iconv_t m_iconvHandle;

    Comment *readCommentElement();
    Comment *readPlainCommentElement();
    Macro *readMacroElement();
    Preamble *readPreambleElement();
    Entry *readEntryElement(const QString& typeString);
    Element *nextElement();
    Token nextToken();
    QString readString(bool &isStringKey);
    QString readSimpleString(QChar until = '\0');
    QString readQuotedString();
    QString readLine();
    QString readBracketString(const QChar openingBracket);
    Token readValue(Value *value, EntryField::FieldType fieldType);

    void unescapeLaTeXChars(QString &text);
    void splitPersons(const QString& test, QStringList &persons);
};

}
}

#endif
