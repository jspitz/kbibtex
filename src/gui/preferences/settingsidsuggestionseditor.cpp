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

#include "settingsidsuggestionseditor.h"

#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QSignalMapper>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QAction>
#include <QDialogButtonBox>

#include <KLineEdit>
#include <KComboBox>
#include <KLocalizedString>

#include <qxtspanslider.h>


TokenWidget::TokenWidget(QWidget *parent)
        : QGroupBox(parent)
{
    gridLayout = new QGridLayout(this);
    formLayout = new QFormLayout();
    gridLayout->addLayout(formLayout, 0, 0, 4, 1);
    gridLayout->setColumnStretch(0, 100);
}

void TokenWidget::addButtons(QPushButton *buttonUp, QPushButton *buttonDown, QPushButton *buttonRemove)
{
    gridLayout->setColumnMinimumWidth(1, 32);
    gridLayout->setColumnStretch(1, 1);
    gridLayout->setColumnStretch(2, 1);

    gridLayout->addWidget(buttonUp, 0, 2, 1, 1);
    buttonUp->setParent(this);
    gridLayout->addWidget(buttonDown, 1, 2, 1, 1);
    buttonDown->setParent(this);
    gridLayout->addWidget(buttonRemove, 2, 2, 1, 1);
    buttonRemove->setParent(this);
}


AuthorWidget::AuthorWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, IdSuggestionsEditWidget *isew, QWidget *parent)
        : TokenWidget(parent)
{
    setTitle(i18n("Authors"));

    QBoxLayout *boxLayout = new QVBoxLayout();
    boxLayout->setMargin(0);
    formLayout->addRow(i18n("Author Range:"), boxLayout);
    spanSliderAuthor = new QxtSpanSlider(Qt::Horizontal, this);
    boxLayout->addWidget(spanSliderAuthor);
    spanSliderAuthor->setRange(0, 9);
    spanSliderAuthor->setHandleMovementMode(QxtSpanSlider::NoCrossing);
    spanSliderAuthor->setLowerValue(info.startWord);
    spanSliderAuthor->setUpperValue(qMin(spanSliderAuthor->maximum(), info.endWord));

    checkBoxLastAuthor = new QCheckBox(i18n("... and last author"), this);
    boxLayout->addWidget(checkBoxLastAuthor);

    labelAuthorRange = new QLabel(this);
    boxLayout->addWidget(labelAuthorRange);
    const int maxWidth = qMax(labelAuthorRange->fontMetrics().width(i18n("From first author to author %1 and last author", 88)), labelAuthorRange->fontMetrics().width(i18n("From author %1 to author %2 and last author", 88, 88)));
    labelAuthorRange->setMinimumWidth(maxWidth);

    comboBoxChangeCase = new KComboBox(false, this);
    comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
    comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
    comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
    comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
    formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
    comboBoxChangeCase->setCurrentIndex((int)info.caseChange); /// enum has numbers assigned to cases and combo box has same indices

    lineEditTextInBetween = new KLineEdit(this);
    formLayout->addRow(i18n("Text in between:"), lineEditTextInBetween);
    lineEditTextInBetween->setText(info.inBetween);

    spinBoxLength = new QSpinBox(this);
    formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
    spinBoxLength->setSpecialValueText(i18n("No limitation"));
    spinBoxLength->setMinimum(0);
    spinBoxLength->setMaximum(9);
    spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

    connect(spanSliderAuthor, &QxtSpanSlider::lowerValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(spanSliderAuthor, &QxtSpanSlider::upperValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(spanSliderAuthor, &QxtSpanSlider::lowerValueChanged, this, &AuthorWidget::updateRangeLabel);
    connect(spanSliderAuthor, &QxtSpanSlider::upperValueChanged, this, &AuthorWidget::updateRangeLabel);
    connect(checkBoxLastAuthor, &QCheckBox::toggled, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(checkBoxLastAuthor, &QCheckBox::toggled, this, &AuthorWidget::updateRangeLabel);
    connect(comboBoxChangeCase, static_cast<void(KComboBox::*)(int)>(&KComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
    connect(lineEditTextInBetween, &KLineEdit::textEdited, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(spinBoxLength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), isew, &IdSuggestionsEditWidget::updatePreview);

    updateRangeLabel();
}

QString AuthorWidget::toString() const
{
    QString result = QStringLiteral("A");

    if (spinBoxLength->value() > 0)
        result.append(QString::number(spinBoxLength->value()));

    IdSuggestions::CaseChange caseChange = (IdSuggestions::CaseChange)comboBoxChangeCase->currentIndex();
    if (caseChange == IdSuggestions::ccToLower)
        result.append(QStringLiteral("l"));
    else if (caseChange == IdSuggestions::ccToUpper)
        result.append(QStringLiteral("u"));
    else if (caseChange == IdSuggestions::ccToCamelCase)
        result.append(QStringLiteral("c"));

    if (spanSliderAuthor->lowerValue() > spanSliderAuthor->minimum() || spanSliderAuthor->upperValue() < spanSliderAuthor->maximum())
        result.append(QString(QStringLiteral("w%1%2")).arg(spanSliderAuthor->lowerValue()).arg(spanSliderAuthor->upperValue() < spanSliderAuthor->maximum() ? QString::number(spanSliderAuthor->upperValue()) : QStringLiteral("I")));
    if (checkBoxLastAuthor->isChecked())
        result.append(QStringLiteral("L"));

    const QString text = lineEditTextInBetween->text();
    if (!text.isEmpty())
        result.append(QStringLiteral("\"")).append(text);

    return result;
}

void AuthorWidget::updateRangeLabel()
{
    const int lower = spanSliderAuthor->lowerValue();
    const int upper = spanSliderAuthor->upperValue();
    const int min = spanSliderAuthor->minimum();
    const int max = spanSliderAuthor->maximum();

    labelAuthorRange->setText(IdSuggestions::formatAuthorRange(lower == min ? 0 : lower, upper == max ? 0x00ffffff : upper, checkBoxLastAuthor->isChecked()));
}

class YearWidget : public TokenWidget
{
    Q_OBJECT

private:
    KComboBox *comboBoxDigits;

public:
    YearWidget(int digits, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Year"));

        comboBoxDigits = new KComboBox(false, this);
        comboBoxDigits->addItem(i18n("2 digits"), 2);
        comboBoxDigits->addItem(i18n("4 digits"), 4);
        formLayout->addRow(i18n("Digits:"), comboBoxDigits);
        comboBoxDigits->setCurrentIndex(comboBoxDigits->findData(digits));

        connect(comboBoxDigits, static_cast<void(KComboBox::*)(int)>(&KComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
    }

    QString toString() const {
        const int year = comboBoxDigits->itemData(comboBoxDigits->currentIndex()).toInt();
        QString result = year == 4 ? QStringLiteral("Y") : QStringLiteral("y");

        return result;
    }
};

TitleWidget::TitleWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, bool removeSmallWords, IdSuggestionsEditWidget *isew, QWidget *parent)
        : TokenWidget(parent)
{
    setTitle(i18n("Title"));

    QBoxLayout *boxLayout = new QVBoxLayout();
    boxLayout->setMargin(0);
    formLayout->addRow(i18n("Word Range:"), boxLayout);
    spanSliderWords = new QxtSpanSlider(Qt::Horizontal, this);
    boxLayout->addWidget(spanSliderWords);
    spanSliderWords->setRange(0, 9);
    spanSliderWords->setHandleMovementMode(QxtSpanSlider::NoCrossing);
    if (info.startWord > 0 || info.endWord < 0xffff) {
        spanSliderWords->setLowerValue(info.startWord);
        spanSliderWords->setUpperValue(qMin(spanSliderWords->maximum(), info.endWord));
    } else {
        spanSliderWords->setLowerValue(0);
        spanSliderWords->setUpperValue(spanSliderWords->maximum());
    }

    labelWordsRange = new QLabel(this);
    boxLayout->addWidget(labelWordsRange);
    const int a = qMax(labelWordsRange->fontMetrics().width(i18n("From first to last word")), labelWordsRange->fontMetrics().width(i18n("From word %1 to last word", 88)));
    const int b = qMax(labelWordsRange->fontMetrics().width(i18n("From first word to word %1", 88)), labelWordsRange->fontMetrics().width(i18n("From word %1 to word %2", 88, 88)));
    labelWordsRange->setMinimumWidth(qMax(a, b));

    checkBoxRemoveSmallWords = new QCheckBox(i18n("Remove"), this);
    formLayout->addRow(i18n("Small words:"), checkBoxRemoveSmallWords);
    checkBoxRemoveSmallWords->setChecked(removeSmallWords);

    comboBoxChangeCase = new KComboBox(false, this);
    comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
    comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
    comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
    comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
    formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
    comboBoxChangeCase->setCurrentIndex((int)info.caseChange); /// enum has numbers assigned to cases and combo box has same indices

    lineEditTextInBetween = new KLineEdit(this);
    formLayout->addRow(i18n("Text in between:"), lineEditTextInBetween);
    lineEditTextInBetween->setText(info.inBetween);

    spinBoxLength = new QSpinBox(this);
    formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
    spinBoxLength->setSpecialValueText(i18n("No limitation"));
    spinBoxLength->setMinimum(0);
    spinBoxLength->setMaximum(9);
    spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

    connect(spanSliderWords, &QxtSpanSlider::lowerValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(spanSliderWords, &QxtSpanSlider::upperValueChanged, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(spanSliderWords, &QxtSpanSlider::lowerValueChanged, this, &TitleWidget::updateRangeLabel);
    connect(spanSliderWords, &QxtSpanSlider::upperValueChanged, this, &TitleWidget::updateRangeLabel);
    connect(checkBoxRemoveSmallWords, &QCheckBox::toggled, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(comboBoxChangeCase, static_cast<void(KComboBox::*)(int)>(&KComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
    connect(lineEditTextInBetween, &KLineEdit::textEdited, isew, &IdSuggestionsEditWidget::updatePreview);
    connect(spinBoxLength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), isew, &IdSuggestionsEditWidget::updatePreview);

    updateRangeLabel();
}

QString TitleWidget::toString() const
{
    QString result = checkBoxRemoveSmallWords->isChecked() ? QStringLiteral("T") : QStringLiteral("t");

    if (spinBoxLength->value() > 0)
        result.append(QString::number(spinBoxLength->value()));

    IdSuggestions::CaseChange caseChange = (IdSuggestions::CaseChange)comboBoxChangeCase->currentIndex();
    if (caseChange == IdSuggestions::ccToLower)
        result.append(QStringLiteral("l"));
    else if (caseChange == IdSuggestions::ccToUpper)
        result.append(QStringLiteral("u"));
    else if (caseChange == IdSuggestions::ccToCamelCase)
        result.append(QStringLiteral("c"));

    if (spanSliderWords->lowerValue() > spanSliderWords->minimum() || spanSliderWords->upperValue() < spanSliderWords->maximum())
        result.append(QString(QStringLiteral("w%1%2")).arg(spanSliderWords->lowerValue()).arg(spanSliderWords->upperValue() < spanSliderWords->maximum() ? QString::number(spanSliderWords->upperValue()) : QStringLiteral("I")));

    const QString text = lineEditTextInBetween->text();
    if (!text.isEmpty())
        result.append(QStringLiteral("\"")).append(text);

    return result;
}

void TitleWidget::updateRangeLabel()
{
    const int lower = spanSliderWords->lowerValue();
    const int upper = spanSliderWords->upperValue();
    const int min = spanSliderWords->minimum();
    const int max = spanSliderWords->maximum();

    if (lower == min && upper == min)
        labelWordsRange->setText(i18n("First word only"));
    else if (lower == min + 1 && upper == max)
        labelWordsRange->setText(i18n("All but first word"));
    else if (lower == min && upper == max)
        labelWordsRange->setText(i18n("From first to last word"));
    else if (lower > min && upper == max)
        labelWordsRange->setText(i18n("From word %1 to last word", lower + 1));
    else if (lower == min && upper < max)
        labelWordsRange->setText(i18n("From first word to word %1", upper + 1));
    else
        labelWordsRange->setText(i18n("From word %1 to word %2", lower + 1, upper + 1));
}

/**
 * @author Thomas Fischer
 */
class JournalWidget : public TokenWidget
{
    Q_OBJECT

private:
    KComboBox *comboBoxChangeCase;
    QSpinBox *spinBoxLength;

public:
    JournalWidget(const struct IdSuggestions::IdSuggestionTokenInfo &info, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent)
    {
        setTitle(i18n("Journal"));

        QBoxLayout *boxLayout = new QVBoxLayout();
        boxLayout->setMargin(0);

        comboBoxChangeCase = new KComboBox(false, this);
        comboBoxChangeCase->addItem(i18n("No change"), IdSuggestions::ccNoChange);
        comboBoxChangeCase->addItem(i18n("To upper case"), IdSuggestions::ccToUpper);
        comboBoxChangeCase->addItem(i18n("To lower case"), IdSuggestions::ccToLower);
        comboBoxChangeCase->addItem(i18n("To CamelCase"), IdSuggestions::ccToCamelCase);
        formLayout->addRow(i18n("Change casing:"), comboBoxChangeCase);
        comboBoxChangeCase->setCurrentIndex((int)info.caseChange); /// enum has numbers assigned to cases and combo box has same indices

        spinBoxLength = new QSpinBox(this);
        formLayout->addRow(i18n("Only first characters:"), spinBoxLength);
        spinBoxLength->setSpecialValueText(i18n("No limitation"));
        spinBoxLength->setMinimum(0);
        spinBoxLength->setMaximum(9);
        spinBoxLength->setValue(info.len == 0 || info.len > 9 ? 0 : info.len);

        connect(comboBoxChangeCase, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), isew, &IdSuggestionsEditWidget::updatePreview);
        connect(spinBoxLength, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), isew, &IdSuggestionsEditWidget::updatePreview);
    }

    QString toString() const
    {
        QString result = QStringLiteral("j");

        if (spinBoxLength->value() > 0)
            result.append(QString::number(spinBoxLength->value()));

        IdSuggestions::CaseChange caseChange = (IdSuggestions::CaseChange)comboBoxChangeCase->currentIndex();
        if (caseChange == IdSuggestions::ccToLower)
            result.append(QStringLiteral("l"));
        else if (caseChange == IdSuggestions::ccToUpper)
            result.append(QStringLiteral("u"));
        else if (caseChange == IdSuggestions::ccToCamelCase)
            result.append(QStringLiteral("c"));

        return result;
    }
};

class TextWidget : public TokenWidget
{
    Q_OBJECT

private:
    KLineEdit *lineEditText;

public:
    TextWidget(const QString &text, IdSuggestionsEditWidget *isew, QWidget *parent)
            : TokenWidget(parent) {
        setTitle(i18n("Text"));

        lineEditText = new KLineEdit(this);
        formLayout->addRow(i18n("Text:"), lineEditText);
        lineEditText->setText(text);

        connect(lineEditText, &KLineEdit::textEdited, isew, &IdSuggestionsEditWidget::updatePreview);
    }

    QString toString() const {
        QString result = QStringLiteral("\"") + lineEditText->text();
        return result;
    }
};

class IdSuggestionsEditWidget::IdSuggestionsEditWidgetPrivate
{
private:
    IdSuggestionsEditWidget *p;
public:
    enum TokenType {ttTitle = 0, ttAuthor = 1, ttYear = 2, ttJournal = 3, ttText = 4};

    QWidget *container;
    QBoxLayout *containerLayout;
    QList<TokenWidget *> widgetList;
    QLabel *labelPreview;
    QPushButton *buttonAddTokenAtTop, *buttonAddTokenAtBottom;
    const Entry *previewEntry;
    QSignalMapper *signalMapperRemove, *signalMapperMoveUp, *signalMapperMoveDown;
    QScrollArea *area;

    IdSuggestionsEditWidgetPrivate(const Entry *pe, IdSuggestionsEditWidget *parent)
            : p(parent), previewEntry(pe) {
        setupGUI();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        labelPreview = new QLabel(p);
        layout->addWidget(labelPreview, 0, 0, 1, 1);
        layout->setColumnStretch(0, 100);

        area = new QScrollArea(p);
        layout->addWidget(area, 1, 0, 1, 1);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

        container = new QWidget(area);
        area->setWidget(container);
        area->setWidgetResizable(true);
        containerLayout = new QVBoxLayout(container);
        area->setMinimumSize(384, 256);

        buttonAddTokenAtTop = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add at top"), container);
        containerLayout->addWidget(buttonAddTokenAtTop, 0);

        containerLayout->addStretch(1);

        buttonAddTokenAtBottom = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add at bottom"), container);
        containerLayout->addWidget(buttonAddTokenAtBottom, 0);

        QMenu *menuAddToken = new QMenu(p);
        QSignalMapper *signalMapperAddMenu = new QSignalMapper(p);
        buttonAddTokenAtTop->setMenu(menuAddToken);
        QAction *action = menuAddToken->addAction(i18n("Title"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, -ttTitle);
        action = menuAddToken->addAction(i18n("Author"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, -ttAuthor);
        action = menuAddToken->addAction(i18n("Year"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, -ttYear);
        action = menuAddToken->addAction(i18n("Journal"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, -ttJournal);
        action = menuAddToken->addAction(i18n("Text"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, -ttText);
        connect(signalMapperAddMenu, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), p, &IdSuggestionsEditWidget::addToken);

        menuAddToken = new QMenu(p);
        signalMapperAddMenu = new QSignalMapper(p);
        buttonAddTokenAtBottom->setMenu(menuAddToken);
        action = menuAddToken->addAction(i18n("Title"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, ttTitle);
        action = menuAddToken->addAction(i18n("Author"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, ttAuthor);
        action = menuAddToken->addAction(i18n("Year"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, ttYear);
        action = menuAddToken->addAction(i18n("Journal"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, ttJournal);
        action = menuAddToken->addAction(i18n("Text"), signalMapperAddMenu, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
        signalMapperAddMenu->setMapping(action, ttText);
        connect(signalMapperAddMenu, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), p, &IdSuggestionsEditWidget::addToken);

        signalMapperMoveUp = new QSignalMapper(p);
        connect(signalMapperMoveUp, static_cast<void(QSignalMapper::*)(QWidget *)>(&QSignalMapper::mapped), p, &IdSuggestionsEditWidget::moveUpToken);
        signalMapperMoveDown = new QSignalMapper(p);
        connect(signalMapperMoveDown, static_cast<void(QSignalMapper::*)(QWidget *)>(&QSignalMapper::mapped), p, &IdSuggestionsEditWidget::moveDownToken);
        signalMapperRemove = new QSignalMapper(p);
        connect(signalMapperRemove, static_cast<void(QSignalMapper::*)(QWidget *)>(&QSignalMapper::mapped), p, &IdSuggestionsEditWidget::removeToken);

    }

    void addManagementButtons(TokenWidget *tokenWidget) {
        if (tokenWidget != NULL) {
            QPushButton *buttonUp = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), QStringLiteral(""), tokenWidget);
            QPushButton *buttonDown = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down")), QStringLiteral(""), tokenWidget);
            QPushButton *buttonRemove = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), QStringLiteral(""), tokenWidget);
            tokenWidget->addButtons(buttonUp, buttonDown, buttonRemove);
            connect(buttonUp, &QPushButton::clicked, signalMapperMoveUp, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
            signalMapperMoveUp->setMapping(buttonUp, tokenWidget);
            connect(buttonDown, &QPushButton::clicked, signalMapperMoveDown, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
            signalMapperMoveDown->setMapping(buttonDown, tokenWidget);
            connect(buttonRemove, &QPushButton::clicked, signalMapperRemove, static_cast<void(QSignalMapper::*)()>(&QSignalMapper::map));
            signalMapperRemove->setMapping(buttonRemove, tokenWidget);
        }
    }

    void add(TokenType tokenType, bool atTop) {
        const int pos = atTop ? 1 : containerLayout->count() - 2;
        TokenWidget *tokenWidget = NULL;
        switch (tokenType) {
        case ttTitle: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = -1;
            info.startWord = 0;
            info.endWord = 0x00ffffff;
            info.lastWord = false;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new TitleWidget(info, true, p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
        }
        break;
        case ttAuthor: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = -1;
            info.startWord = 0;
            info.endWord = 0x00ffffff;
            info.lastWord = false;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new AuthorWidget(info, p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
        }
        break;
        case ttYear:
            tokenWidget = new YearWidget(4, p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
            break;
        case ttJournal: {
            struct IdSuggestions::IdSuggestionTokenInfo info;
            info.inBetween = QString();
            info.len = 1;
            info.startWord = 0;
            info.endWord = 0x00ffffff;
            info.lastWord = false;
            info.caseChange = IdSuggestions::ccNoChange;
            tokenWidget = new JournalWidget(info, p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
        }
        break;
        case ttText:
            tokenWidget = new TextWidget(QString(), p, container);
            widgetList << tokenWidget;
            containerLayout->insertWidget(pos, tokenWidget, 1);
        }

        addManagementButtons(tokenWidget);
    }

    void reset(const QString &formatString) {
        while (!widgetList.isEmpty())
            delete widgetList.takeFirst();

        QStringList tokenList = formatString.split(QStringLiteral("|"), QString::SkipEmptyParts);
        foreach (const QString &token, tokenList) {
            TokenWidget *tokenWidget = NULL;

            if (token[0] == 'a' || token[0] == 'A' || token[0] == 'z') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                /// Support deprecated 'a' and 'z' cases
                if (token[0] == 'a')
                    info.startWord = info.endWord = 0;
                else if (token[0] == 'z') {
                    info.startWord = 1;
                    info.endWord = 0x00ffffff;
                }
                tokenWidget = new AuthorWidget(info, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'y') {
                tokenWidget = new YearWidget(2, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'Y') {
                tokenWidget = new YearWidget(4, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 't' || token[0] == 'T') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new TitleWidget(info, token[0] == 'T', p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == 'j') {
                struct IdSuggestions::IdSuggestionTokenInfo info = p->evalToken(token.mid(1));
                tokenWidget = new JournalWidget(info, p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            } else if (token[0] == '"') {
                tokenWidget = new TextWidget(token.mid(1), p, container);
                widgetList << tokenWidget;
                containerLayout->insertWidget(containerLayout->count() - 2, tokenWidget, 1);
            }

            addManagementButtons(tokenWidget);
        }

        p->updatePreview();
    }

    QString apply() {
        QStringList result;

        foreach (TokenWidget *widget, widgetList) {
            result << widget->toString();
        }

        return result.join(QStringLiteral("|"));
    }
};

IdSuggestionsEditWidget::IdSuggestionsEditWidget(const Entry *previewEntry, QWidget *parent, Qt::WindowFlags f)
        : QWidget(parent, f), IdSuggestions(), d(new IdSuggestionsEditWidgetPrivate(previewEntry, this))
{
    /// nothing
}

IdSuggestionsEditWidget::~IdSuggestionsEditWidget()
{
// TODO
}

void IdSuggestionsEditWidget::setFormatString(const QString &formatString)
{
    d->reset(formatString);
}

QString IdSuggestionsEditWidget::formatString() const
{
    return d->apply();
}

void IdSuggestionsEditWidget::updatePreview()
{
    const QString formatString = d->apply();
    d->labelPreview->setText(formatId(*d->previewEntry, formatString));
    d->labelPreview->setToolTip(i18n("<qt>Structure:<ul><li>%1</li></ul>Example: %2</qt>", formatStrToHuman(formatString).join(QStringLiteral("</li><li>")), formatId(*d->previewEntry, formatString)));
}

void IdSuggestionsEditWidget::moveUpToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget *>(widget);
    int curPos = d->widgetList.indexOf(tokenWidget);
    if (curPos > 0) {
        d->widgetList.removeAt(curPos);
        const int layoutPos = d->containerLayout->indexOf(tokenWidget);
        d->containerLayout->removeWidget(tokenWidget);
        d->widgetList.insert(curPos - 1, tokenWidget);
        d->containerLayout->insertWidget(layoutPos - 1, tokenWidget, 1);
    }
}

void IdSuggestionsEditWidget::moveDownToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget *>(widget);
    int curPos = d->widgetList.indexOf(tokenWidget);
    if (curPos < d->widgetList.size() - 1) {
        d->widgetList.removeAt(curPos);
        const int layoutPos = d->containerLayout->indexOf(tokenWidget);
        d->containerLayout->removeWidget(tokenWidget);
        d->widgetList.insert(curPos + 1, tokenWidget);
        d->containerLayout->insertWidget(layoutPos + 1, tokenWidget, 1);
    }
}

void IdSuggestionsEditWidget::removeToken(QWidget *widget)
{
    TokenWidget *tokenWidget = static_cast<TokenWidget *>(widget);
    d->widgetList.removeOne(tokenWidget);
    d->containerLayout->removeWidget(tokenWidget);
    tokenWidget->deleteLater();
    updatePreview();
}

void IdSuggestionsEditWidget::addToken(int cmd)
{
    if (cmd < 0) {
        d->add((IdSuggestionsEditWidgetPrivate::TokenType)(-cmd), true);
        d->area->ensureWidgetVisible(d->buttonAddTokenAtTop); // FIXME does not work as intended
    } else {
        d->add((IdSuggestionsEditWidgetPrivate::TokenType)cmd, false);
        d->area->ensureWidgetVisible(d->buttonAddTokenAtBottom); // FIXME does not work as intended
    }
    updatePreview();
}

IdSuggestionsEditDialog::IdSuggestionsEditDialog(QWidget *parent, Qt::WindowFlags flags)
        : QDialog(parent, flags)
{
    setWindowTitle(i18n("Edit Id Suggestion"));
}

IdSuggestionsEditDialog::~IdSuggestionsEditDialog()
{
    /// nothing
}

QString IdSuggestionsEditDialog::editSuggestion(const Entry *previewEntry, const QString &suggestion, QWidget *parent)
{
    QPointer<IdSuggestionsEditDialog> dlg = new IdSuggestionsEditDialog(parent);
    QBoxLayout *boxLayout = new QVBoxLayout(dlg);
    IdSuggestionsEditWidget *widget = new IdSuggestionsEditWidget(previewEntry, dlg);
    boxLayout->addWidget(widget);
    QDialogButtonBox *dbb = new QDialogButtonBox(dlg);
    dbb->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    boxLayout->addWidget(dbb);

    widget->setFormatString(suggestion);
    if (dlg->exec() == Accepted) {
        const QString formatString = widget->formatString();
        delete dlg;
        return formatString;
    }

    delete dlg;

    /// Return unmodified original suggestion
    return suggestion;
}

#include "settingsidsuggestionseditor.moc"
