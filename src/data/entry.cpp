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

#include "entry.h"

#include <typeinfo>

#include <QDebug>

#include "file.h"

// FIXME: Check if using those constants in the program is really necessary
// or can be replace by config files
const QString Entry::ftAbstract = QStringLiteral("abstract");
const QString Entry::ftAddress = QStringLiteral("address");
const QString Entry::ftAuthor = QStringLiteral("author");
const QString Entry::ftBookTitle = QStringLiteral("booktitle");
const QString Entry::ftChapter = QStringLiteral("chapter");
const QString Entry::ftColor = QStringLiteral("x-color");
const QString Entry::ftComment = QStringLiteral("comment");
const QString Entry::ftCrossRef = QStringLiteral("crossref");
const QString Entry::ftDOI = QStringLiteral("doi");
const QString Entry::ftEditor = QStringLiteral("editor");
const QString Entry::ftFile = QStringLiteral("file");
const QString Entry::ftISSN = QStringLiteral("issn");
const QString Entry::ftISBN = QStringLiteral("isbn");
const QString Entry::ftJournal = QStringLiteral("journal");
const QString Entry::ftKeywords = QStringLiteral("keywords");
const QString Entry::ftLocalFile = QStringLiteral("localfile");
const QString Entry::ftLocation = QStringLiteral("location");
const QString Entry::ftMonth = QStringLiteral("month");
const QString Entry::ftNote = QStringLiteral("note");
const QString Entry::ftNumber = QStringLiteral("number");
const QString Entry::ftPages = QStringLiteral("pages");
const QString Entry::ftPublisher = QStringLiteral("publisher");
const QString Entry::ftSchool = QStringLiteral("school");
const QString Entry::ftSeries = QStringLiteral("series");
const QString Entry::ftStarRating = QStringLiteral("x-stars");
const QString Entry::ftTitle = QStringLiteral("title");
const QString Entry::ftUrl = QStringLiteral("url");
const QString Entry::ftUrlDate = QStringLiteral("urldate");
const QString Entry::ftVolume = QStringLiteral("volume");
const QString Entry::ftYear = QStringLiteral("year");
const QString Entry::ftXData = QStringLiteral("xdata");

const QString Entry::etArticle = QStringLiteral("article");
const QString Entry::etBook = QStringLiteral("book");
const QString Entry::etInBook = QStringLiteral("inbook");
const QString Entry::etInProceedings = QStringLiteral("inproceedings");
const QString Entry::etMisc = QStringLiteral("misc");
const QString Entry::etPhDThesis = QStringLiteral("phdthesis");
const QString Entry::etMastersThesis = QStringLiteral("mastersthesis");
const QString Entry::etTechReport = QStringLiteral("techreport");
const QString Entry::etUnpublished = QStringLiteral("unpublished");

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

const Value Entry::operator[](const QString &key) const
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

Entry *Entry::resolveCrossref(const File *bibTeXfile, QMap<QString, QString> xmaps) const
{
    return resolveCrossref(*this, bibTeXfile, xmaps);
}

Entry *Entry::resolveCrossref(const Entry &original, const File *bibTeXfile, QMap<QString, QString> xmaps)
{
    Entry *result = new Entry(original);

    if (bibTeXfile == nullptr)
        return result;

    const QString crossRef = PlainTextValue::text(original.value(ftCrossRef));
    if (crossRef.isEmpty())
        return result;

    const QSharedPointer<Entry> crossRefEntry = bibTeXfile->containsKey(crossRef, File::etEntry).dynamicCast<Entry>();
    if (!crossRefEntry.isNull()) {
        /// copy all fields from crossref'ed entry to new entry which do not (yet) exist in the new entry
        for (Entry::ConstIterator it = crossRefEntry->constBegin(); it != crossRefEntry->constEnd(); ++it) {
                QString item = it.key();
                // check for field mappings (biblatex)
                // search for <type>:item
                QMap<QString, QString>::const_iterator ci = xmaps.find(crossRefEntry->type().toLower() + ":" + item);
                if (ci != xmaps.end()) {
                        QStringList const targets = ci.value().split("&");
                        for (int i = 0; i < targets.size(); ++i) {
                                item = targets.at(i);
                                if (!result->contains(item))
                                        result->insert(item, Value(it.value()));
                        }
                } else if (!result->contains(item))
                        result->insert(item, Value(it.value()));
        }

        /// remove crossref field (no longer of use)
        result->remove(ftCrossRef);
    }

    // Also inherit XData references (biblatex).
    // Note that xdata fields can contain multiple (comma-separated) keys
    // and can be infinitly nested (xdata references can refer to further xdata entries).
    QString xData = PlainTextValue::text(result->value(ftXData));
    // Keep track of visited keys in order to prevent infinite loops.
    QStringList xDatasVisited;
    // Stack multiple references for later procession
    QStringList xDatasStack;
    while (!xData.isEmpty()) {
        // XData can be a comma-separated list of keys.
        QStringList const xDatas = xData.split(",");
        for (int i = 0; i < xDatas.size(); ++i) {
                QString const xDataToken = xDatas.at(i);
                if (xDatasVisited.contains(xDataToken))
                        // We processed this one already. Skip.
                        continue;
                // Record that we visit this one.
                xDatasVisited.append(xDataToken);
                QSharedPointer<Entry> xDataEntry = bibTeXfile->containsKey(xDataToken, File::etEntry).dynamicCast<Entry>();
                if (!xDataEntry.isNull()) {
                        /// copy all fields from xdata entry to parent entry which do not (yet) exist in the parent entry
                        for (Entry::ConstIterator it = xDataEntry->constBegin(); it != xDataEntry->constEnd(); ++it) {
                                if (!result->contains(it.key()))
                                        result->insert(it.key(), Value(it.value()));
                                else if (it.key().toLower() == ftXData && !xDatasVisited.contains(PlainTextValue::text(it.value())))
                                        // Stack inherited xdata references for later
                                        xDatasStack.append(PlainTextValue::text(it.value()));
                        }
                }
        }
        xData = xDatasStack.join(",");
        xDatasStack.clear();
    }
    /// remove xdata field (not needed any longer)
    result->remove(ftXData);

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
    result.reserve(maxAuthors);
    for (const QSharedPointer<const ValueItem> &item : const_cast<const Value &>(value)) {
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

bool Entry::isEntry(const Element &other) {
    return typeid(other) == typeid(Entry);
}

QDebug operator<<(QDebug dbg, const Entry &entry) {
    dbg.nospace() << "Entry " << entry.id() << " (uniqueId=" << entry.uniqueId() << "), has " << entry.count() << " key-value pairs";
    return dbg;
}
