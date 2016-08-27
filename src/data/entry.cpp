/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "entry.h"
#include "file.h"

#define max(a,b) ((a)>(b)?(a):(b))

// FIXME: Check if using those constants in the program is really necessary
// or can be replace by config files
const QLatin1String Entry::ftAbstract = QLatin1String("abstract");
const QLatin1String Entry::ftAddress = QLatin1String("address");
const QLatin1String Entry::ftAuthor = QLatin1String("author");
const QLatin1String Entry::ftBookTitle = QLatin1String("booktitle");
const QLatin1String Entry::ftChapter = QLatin1String("chapter");
const QLatin1String Entry::ftColor = QLatin1String("x-color");
const QLatin1String Entry::ftComment = QLatin1String("comment");
const QLatin1String Entry::ftCrossRef = QLatin1String("crossref");
const QLatin1String Entry::ftDOI = QLatin1String("doi");
const QLatin1String Entry::ftEditor = QLatin1String("editor");
const QLatin1String Entry::ftFile = QLatin1String("file");
const QLatin1String Entry::ftISSN = QLatin1String("issn");
const QLatin1String Entry::ftISBN = QLatin1String("isbn");
const QLatin1String Entry::ftJournal = QLatin1String("journal");
const QLatin1String Entry::ftKeywords = QLatin1String("keywords");
const QLatin1String Entry::ftLocalFile = QLatin1String("localfile");
const QLatin1String Entry::ftLocation = QLatin1String("location");
const QLatin1String Entry::ftMonth = QLatin1String("month");
const QLatin1String Entry::ftNote = QLatin1String("note");
const QLatin1String Entry::ftNumber = QLatin1String("number");
const QLatin1String Entry::ftPages = QLatin1String("pages");
const QLatin1String Entry::ftPublisher = QLatin1String("publisher");
const QLatin1String Entry::ftSchool = QLatin1String("school");
const QLatin1String Entry::ftSeries = QLatin1String("series");
const QLatin1String Entry::ftStarRating = QLatin1String("x-stars");
const QLatin1String Entry::ftTitle = QLatin1String("title");
const QLatin1String Entry::ftUrl = QLatin1String("url");
const QLatin1String Entry::ftUrlDate = QLatin1String("urldate");
const QLatin1String Entry::ftVolume = QLatin1String("volume");
const QLatin1String Entry::ftYear = QLatin1String("year");

const QLatin1String Entry::etArticle = QLatin1String("article");
const QLatin1String Entry::etBook = QLatin1String("book");
const QLatin1String Entry::etInBook = QLatin1String("inbook");
const QLatin1String Entry::etInProceedings = QLatin1String("inproceedings");
const QLatin1String Entry::etMisc = QLatin1String("misc");
const QLatin1String Entry::etPhDThesis = QLatin1String("phdthesis");
const QLatin1String Entry::etMastersThesis = QLatin1String("mastersthesis");
const QLatin1String Entry::etTechReport = QLatin1String("techreport");
const QLatin1String Entry::etUnpublished = QLatin1String("unpublished");

quint64 Entry::internalUniqueIdCounter = 0;

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class Entry::EntryPrivate
{
public:
    QString type;
    QString id;
};

Entry::Entry(const QString &type, const QString &id)
        : Element(), QMap<QString, Value>(), internalUniqueId(++internalUniqueIdCounter), d(new Entry::EntryPrivate)
{
    d->type = type;
    d->id = id;
}

Entry::Entry(const Entry &other)
        : Element(), QMap<QString, Value>(), internalUniqueId(++internalUniqueIdCounter), d(new Entry::EntryPrivate)
{
    operator=(other);
}

Entry::~Entry()
{
    clear();
    delete d;
}

quint64 Entry::uniqueId() const
{
    return internalUniqueId;
}

Entry &Entry::operator= (const Entry &other)
{
    if (this != &other) {
        d->type = other.type();
        d->id = other.id();
        clear();
        for (Entry::ConstIterator it = other.constBegin(); it != other.constEnd(); ++it)
            insert(it.key(), it.value());
    }
    return *this;
}

Value &Entry::operator[](const QString &key)
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return QMap<QString, Value>::operator[](it.key());

    return QMap<QString, Value>::operator[](key);
}

const  Value Entry::operator[](const QString &key) const
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return QMap<QString, Value>::operator[](it.key());

    return QMap<QString, Value>::operator[](key);
}


void Entry::setType(const QString &type)
{
    d->type = type;
}

QString Entry::type() const
{
    return d->type;
}

void Entry::setId(const QString &id)
{
    d->id = id;
}

QString Entry::id() const
{
    return d->id;
}

const Value Entry::value(const QString &key) const
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return QMap<QString, Value>::value(it.key());

    return QMap<QString, Value>::value(key);
}

int Entry::remove(const QString &key)
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return QMap<QString, Value>::remove(it.key());

    return QMap<QString, Value>::remove(key);
}

bool Entry::contains(const QString &key) const
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return true;

    return false;
}

Entry *Entry::resolveCrossref(const File *bibTeXfile) const
{
    return resolveCrossref(*this, bibTeXfile);
}

Entry *Entry::resolveCrossref(const Entry &original, const File *bibTeXfile)
{
    Entry *result = new Entry(original);

    if (bibTeXfile == NULL)
        return result;

    const QString crossRef = PlainTextValue::text(original.value(ftCrossRef));
    const QSharedPointer<Entry> crossRefEntry = bibTeXfile->containsKey(crossRef, File::etEntry).dynamicCast<Entry>();
    if (!crossRefEntry.isNull()) {
        /// copy all fields from crossref'ed entry to new entry which do not (yet) exist in the new entry
        for (Entry::ConstIterator it = crossRefEntry->constBegin(); it != crossRefEntry->constEnd(); ++it)
            if (!result->contains(it.key()))
                result->insert(it.key(), Value(it.value()));

        if (crossRefEntry->contains(ftTitle)) {
            /// translate crossref's title into new entry's booktitle
            result->insert(ftBookTitle, Value(crossRefEntry->operator [](ftTitle)));
        }

        /// remove crossref field (no longer of use)
        result->remove(ftCrossRef);
    }

    return result;
}

QStringList Entry::authorsLastName(const Entry &entry)
{
    Value value;
    if (entry.contains(Entry::ftAuthor))
        value = entry.value(Entry::ftAuthor);
    if (value.isEmpty() && entry.contains(Entry::ftEditor))
        value = entry.value(Entry::ftEditor);
    if (value.isEmpty())
        return QStringList();

    QStringList result;
    int maxAuthors = 16; ///< limit the number of authors considered
    foreach(const QSharedPointer<const ValueItem> &item, value) {
        QSharedPointer<const Person> person = item.dynamicCast<const Person>();
        if (!person.isNull()) {
            const QString lastName = person->lastName();
            if (!lastName.isEmpty())
                result << lastName;
        }
        if (--maxAuthors <= 0) break;   ///< limit the number of authors considered
    }

    return result;
}

QStringList Entry::authorsLastName() const
{
    return authorsLastName(*this);
}
