/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#include <QIODevice>
#include <QRegExp>
#include <QCoreApplication>
#include <QDebug>

#include <file.h>
#include <comment.h>
#include <macro.h>
#include <preamble.h>
#include <entry.h>
#include <element.h>
#include <encoderlatex.h>
#include <value.h>

#include "fileimporterbibtex.h"

using namespace KBibTeX::IO;

const QString extraAlphaNumChars = QString("?'`-_:.+/$\\\"&");

FileImporterBibTeX::FileImporterBibTeX(const QString& encoding, bool ignoreComments)
        : FileImporter(), m_currentChar(' '), m_ignoreComments(ignoreComments), m_lineBufferSize(4096), m_encoding(encoding)
{
    cancelFlag = FALSE;
    m_lineBuffer = new char[m_lineBufferSize];
    m_textStream = NULL;

    const char *encodingFrom = m_encoding == "latex" ? "utf-8\0" : m_encoding.append("\0").toAscii();
    iconv_t m_iconvHandle = iconv_open("utf-8", encodingFrom);
}


FileImporterBibTeX::~FileImporterBibTeX()
{
    iconv_close(m_iconvHandle);
    delete[] m_lineBuffer;
}

File* FileImporterBibTeX::load(QIODevice *iodevice)
{
    m_mutex.lock();
    cancelFlag = FALSE;

    QString rawText;
    char *convertedLine = new char[m_lineBufferSize];
    int len;
    while (iodevice->isReadable() && (len = iodevice->readLine(m_lineBuffer, m_lineBufferSize)) > 0) {
        char *raw = m_lineBuffer;
        char *enc = convertedLine;
        size_t encLen = m_lineBufferSize, rawLen = (size_t)len;
        size_t result = iconv(m_iconvHandle, &raw, &rawLen, &enc, &encLen);
        if (result != 0) {
            qCritical() << "iconv resulted in error code " << result << " for source encoding " << m_encoding;
            break;
        }
        if (rawLen > 0) {
            qDebug("iconv could not convert complete string, only %i out of %i chars", len - rawLen, len);
            break;
        }
        enc[0] = '\0';
        QString line = QString::fromUtf8(convertedLine);
        rawText.append(line);
    }
    delete[] convertedLine;

    /** Cleaning up code comming from DBLP */
    rawText = rawText.replace("<pre>", "\n\n").replace("</pre>", "\n\n").remove("<a href=\"http://www.informatik.uni-trier.de/~ley/db/about/bibtex.html\">DBLP</a>:");

    rawText = EncoderLaTeX::currentEncoderLaTeX() ->decode(rawText);
    unescapeLaTeXChars(rawText);
    m_textStream = new QTextStream(&rawText, QIODevice::ReadOnly);
    m_textStream->setCodec("UTF-8");

    File *result = new File();
    QIODevice *streamDevice = m_textStream->device();
    while (!cancelFlag && !m_textStream->atEnd()) {
        emit progress(streamDevice->pos(), streamDevice->size());
        qApp->processEvents();
        Element * element = nextElement();
        if (element != NULL) {
            Comment *comment = dynamic_cast<Comment*>(element);
            if (!m_ignoreComments || comment == NULL)
                result->appendElement(element);
            else
                delete element;
        }
        qApp->processEvents();
    }
    emit progress(100, 100);

    if (cancelFlag) {
        qDebug("Loading file has been canceled");
        delete result;
        result = NULL;
    }

    delete m_textStream;

    m_mutex.unlock();
    return result;
}

bool FileImporterBibTeX::guessCanDecode(const QString & rawText)
{
    QString text = EncoderLaTeX::currentEncoderLaTeX() ->decode(rawText);
    return text.indexOf(QRegExp("@\\w+\\{.+\\}")) >= 0;
}

void FileImporterBibTeX::cancel()
{
    cancelFlag = TRUE;
}

Element *FileImporterBibTeX::nextElement()
{
    Token token = nextToken();

    if (token == tAt) {
        QString elementType = readSimpleString();
        if (elementType.toLower() == "comment")
            return readCommentElement();
        else if (elementType.toLower() == "string")
            return readMacroElement();
        else if (elementType.toLower() == "preamble")
            return readPreambleElement();
        else if (!elementType.isEmpty())
            return readEntryElement(elementType);
        else {
            qDebug("ElementType is empty");
            return NULL;
        }
    } else if (token == tUnknown)
        return readPlainCommentElement();

    if (token != tEOF)
        qDebug("Don't know how to parse next token: %i", (int)token);

    return NULL;
}

Comment *FileImporterBibTeX::readCommentElement()
{
    while (m_currentChar != '{' && m_currentChar != '(' && !m_textStream->atEnd())
        *m_textStream >> m_currentChar;

    return new Comment(readBracketString(m_currentChar));
}

Comment *FileImporterBibTeX::readPlainCommentElement()
{
    QString result = readLine();
    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd() && m_currentChar != '@' && !m_currentChar.isSpace()) {
        result.append('\n').append(m_currentChar);
        *m_textStream >> m_currentChar;
        result.append(readLine());
        *m_textStream >> m_currentChar;
    }
    return new Comment(result);
}

Macro *FileImporterBibTeX::readMacroElement()
{
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            qDebug("Error in parsing unknown macro: Opening curly brace ({) expected");
            return NULL;
        }
        token = nextToken();
    }

    QString key = readSimpleString();
    if (nextToken() != tAssign) {
        qDebug() << "Error in parsing macro '" << key << "': Assign symbol (=) expected";
        return NULL;
    }

    Macro *macro = new Macro(key);
    do {
        bool isStringKey = FALSE;
        QString text = readString(isStringKey).replace(QRegExp("\\s+"), " ");
        if (isStringKey)
            macro->value()->items.append(new MacroKey(text));
        else
            macro->value()->items.append(new PlainText(text));

        token = nextToken();
    } while (token == tDoublecross);

    return macro;
}

Preamble *FileImporterBibTeX::readPreambleElement()
{
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            qDebug("Error in parsing unknown preamble: Opening curly brace ({) expected");
            return NULL;
        }
        token = nextToken();
    }

    Preamble *preamble = new Preamble();
    do {
        bool isStringKey = FALSE;
        QString text = readString(isStringKey).replace(QRegExp("\\s+"), " ");
        if (isStringKey)
            preamble->value()->items.append(new MacroKey(text));
        else
            preamble->value()->items.append(new PlainText(text));

        token = nextToken();
    } while (token == tDoublecross);

    return preamble;
}

Entry *FileImporterBibTeX::readEntryElement(const QString& typeString)
{
    Token token = nextToken();
    while (token != tBracketOpen) {
        if (token == tEOF) {
            qDebug("Error in parsing unknown entry: Opening curly brace ({) expected");
            return NULL;
        }
        token = nextToken();
    }

    QString key = readSimpleString();
    Entry *entry = new Entry(typeString, key);

    token = nextToken();
    do {
        if (token == tBracketClose || token == tEOF)
            break;
        else if (token != tComma) {
            qCritical() << "Error in parsing entry '" << key << "': Comma symbol (,) expected";
            delete entry;
            return NULL;
        }

        QString fieldTypeName = readSimpleString();
        token = nextToken();
        if (fieldTypeName == QString::null || token == tBracketClose) {
            // entry is buggy, but we still accept it
            break;
        } else if (token != tAssign) {
            qCritical() << "Error in parsing entry '" << key << "': Assign symbol (=) expected after field name '" << fieldTypeName << "'";
            delete entry;
            return NULL;
        }

        EntryField *entryField = new EntryField(fieldTypeName);

        token = readValue(entryField->value(), entryField->fieldType());

        entry->addField(entryField);
    } while (TRUE);

    return entry;
}

FileImporterBibTeX::Token FileImporterBibTeX::nextToken()
{
    if (m_textStream->atEnd())
        return tEOF;

    Token curToken = tUnknown;

    while (m_currentChar.isSpace() && !m_textStream->atEnd())
        *m_textStream >> m_currentChar;

    switch (m_currentChar.toAscii()) {
    case '@':
        curToken = tAt;
        break;
    case '{':
    case '(':
        curToken = tBracketOpen;
        break;
    case '}':
    case ')':
        curToken = tBracketClose;
        break;
    case ',':
        curToken = tComma;
        break;
    case ';':
        curToken = tSemicolon;
        break;
    case '=':
        curToken = tAssign;
        break;
    case '#':
        curToken = tDoublecross;
        break;
    default:
        if (m_textStream->atEnd())
            curToken = tEOF;
    }

    if (curToken != tUnknown && curToken != tEOF)
        *m_textStream >> m_currentChar;

    return curToken;
}

QString FileImporterBibTeX::readString(bool &isStringKey)
{
    if (m_currentChar.isSpace()) {
        m_textStream->skipWhiteSpace();
        *m_textStream >> m_currentChar;
    }

    isStringKey = FALSE;
    switch (m_currentChar.toAscii()) {
    case '{':
    case '(':
        return readBracketString(m_currentChar);
    case '"':
        return readQuotedString();
    default:
        isStringKey = TRUE;
        return readSimpleString();
    }
}

QString FileImporterBibTeX::readSimpleString(QChar until)
{
    QString result;

    while (m_currentChar.isSpace()) {
        m_textStream->skipWhiteSpace();
        *m_textStream >> m_currentChar;
    }

    if (m_currentChar.isLetterOrNumber() || extraAlphaNumChars.contains(m_currentChar)) {
        result.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }

    while (!m_textStream->atEnd()) {
        if (until != '\0') {
            if (m_currentChar != until)
                result.append(m_currentChar);
            else
                break;
        } else
            if (m_currentChar.isLetterOrNumber() || extraAlphaNumChars.contains(m_currentChar))
                result.append(m_currentChar);
            else
                break;
        *m_textStream >> m_currentChar;
    }
    return result;
}

QString FileImporterBibTeX::readQuotedString()
{
    QString result;
    QChar lastChar = m_currentChar;
    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd()) {
        if (m_currentChar != '"' || lastChar == '\\')
            result.append(m_currentChar);
        else
            break;
        lastChar = m_currentChar;
        *m_textStream >> m_currentChar;
    }

    /** read character after closing " */
    *m_textStream >> m_currentChar;

    return result;
}

QString FileImporterBibTeX::readLine()
{
    QString result;
    while (!m_textStream->atEnd() && m_currentChar != '\n') {
        result.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }
    return result;
}

QString FileImporterBibTeX::readBracketString(const QChar openingBracket)
{
    QString result;
    QChar closingBracket = '}';
    if (openingBracket == '(')
        closingBracket = ')';
    int counter = 1;
    *m_textStream >> m_currentChar;
    while (!m_textStream->atEnd()) {
        if (m_currentChar == openingBracket)
            counter++;
        else if (m_currentChar == closingBracket)
            counter--;

        if (counter == 0)
            break;
        else
            result.append(m_currentChar);
        *m_textStream >> m_currentChar;
    }
    *m_textStream >> m_currentChar;
    return result;
}

FileImporterBibTeX::Token FileImporterBibTeX::readValue(Value *value, EntryField::FieldType fieldType)
{
    Token token = tUnknown;

    do {
        bool isStringKey = FALSE;
        QString text = readString(isStringKey).replace(QRegExp("\\s+"), " ");

        switch (fieldType) {
        case EntryField::ftKeywords: {
            if (isStringKey)
                qDebug("WARNING: Cannot handle keywords that are macros");
            else
                value->items.append(new KeywordContainer(text));
        }
        break;
        case EntryField::ftAuthor:
        case EntryField::ftEditor: {
            if (isStringKey)
                qDebug("WARNING: Cannot handle authors/editors that are macros");
            else {
                QStringList persons;
                splitPersons(text, persons);
                PersonContainer *container = new PersonContainer();
                for (QStringList::ConstIterator pit = persons.constBegin(); pit != persons.constEnd(); ++pit)
                    container->persons.append(new Person(*pit));
                value->items.append(container);
            }
        }
        break;
        case EntryField::ftPages:
            text.replace(QRegExp("\\s*--?\\s*"), QChar(0x2013));
        default: {
            if (isStringKey)
                value->items.append(new MacroKey(text));
            else
                value->items.append(new PlainText(text));
        }
        }

        token = nextToken();
    } while (token == tDoublecross);

    return token;
}

void FileImporterBibTeX::unescapeLaTeXChars(QString &text)
{
    text.replace("\\&", "&");
}

void FileImporterBibTeX::splitPersons(const QString& text, QStringList &persons)
{
    QStringList wordList;
    QString word;
    int bracketCounter = 0;

    for (unsigned int pos = 0;pos < text.length();++pos) {
        if (text[pos] == '{')
            ++bracketCounter;
        else if (text[pos] == '}')
            --bracketCounter;

        if (text[pos] == ' ' || text[pos] == '\n' || text[pos] == '\r') {
            if (word == "and" && bracketCounter == 0) {
                persons.append(wordList.join(" "));
                wordList.clear();
            } else if (!word.isEmpty())
                wordList.append(word);

            word = "";
        } else
            word.append(text[pos]);
    }

    wordList.append(word);
    persons.append(wordList.join(" "));
}
