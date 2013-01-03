/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (testabilitydriver@nokia.com)
**
** This file is part of Testability Driver.
**
** If you have questions regarding the use of this file, please contact
** Nokia at testabilitydriver@nokia.com .
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/


#include "tdriver_codetextedit.h"

#include <QPainter>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QCompleter>
#include <QSyntaxHighlighter>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QColor>
#include <QBrush>
#include <QMessageBox>
#include <QFileInfo>
#include <QSet>
#include <QMenu>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QFileSystemWatcher>
#include <QPushButton>

#include <tdriver_util.h>

#ifndef TDRIVER_NO_SQL
#include <tdriver_translationdb.h>
#endif

#include "tdriver_editor_common.h"
#include <tdriver_debug_macros.h>

#define ALWAYS_USE_RUBY_SYMBOLS 1


#define CURSEL(cur) #cur << (cur).selectionStart() << (cur).position() << (cur).selectionEnd()

const struct TDriverCodeTextEdit::ConfigStruct TDriverCodeTextEdit::Config;

TDriverCodeTextEdit::ConfigStruct::ConfigStruct(void) {
    handledCancelKeys
            << Qt::Key_Escape;
    unhandledCancelKeys
            << Qt::Key_Right << Qt::Key_Left
            << Qt::Key_Delete << Qt::Key_Insert;
    complAcceptKeys
            << Qt::Key_Enter << Qt::Key_Return;
    //    "scratchbuffer":
    // << Qt::Key_Up << Qt::Key_Down
    // << Qt::Key_PageDown << Qt::Key_PageUp
    // << Qt::Key_Tab << Qt::Key_Space;
}



TDriverCodeTextEdit::TDriverCodeTextEdit(QWidget *parent) :
    QPlainTextEdit(parent),
    noNameId(++noNameIdCounter),
    sideArea(new SideArea(this)),
    highlighter(NULL),
    needSyntaxRehighlight(false),
    completer(new QCompleter(this)),
    complPopupShowingInfo(false),
    phraseModel(NULL),
    stackHighlightStart(-1),
    translationDBconfigured(false),
    watcher(NULL),
    fcodec(NULL),
    fcodecUtfBom(false),
    lastFindWrapped(false),
    lastBlock(-1),
    lastBlockCount(document()->blockCount()),
    isRunning(false),
    isUsingTabulatorsMode(true),
    isRubyMode(false),
    isWrapMode(true),
    runningLine(0),
    //completionPopup(new QMenu(tr("Choose translation"), this)),
    completionType(NO_COMPLETION),
    popupFirstInsert(false),
    popupWasTriggered(false),
    contextEval(new QAction(this)),
    ignoreCursorPosChanges(false),
    needRehighlightAfterCursorPosChange(false)
{
    // fix selection color when not focused
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Inactive, QPalette::Highlight,
                     pal.color(QPalette::Active, QPalette::Highlight).lighter(120));
        setPalette(pal);
    }
    // read ruby phrases from completions definition file
    QStringList phrases;
    MEC::DefinitionFileType type = MEC::getDefinitionFile(
                TDriverUtil::tdriverHelperFilePath("completions/completions_ruby_phrases.txt"), phrases);
    if (type != MEC::InvalidDefinitionFile) {
        //qDebug() << FCFL << "phrases" << phrases;
        phraseModel = new QStandardItemModel(this);
        foreach (const QString &phrase, phrases) {
            QStandardItem *stdItem = new QStandardItem();
            stdItem->setData(phrase, Qt::UserRole+1);
            stdItem->setData(MEC::textShortened(phrase.simplified(), 30, 20), Qt::EditRole);
            phraseModel->appendRow(stdItem);
        }
    }

    completer->setWidget(this);
    completer->setWrapAround(true);
    completer->setModelSorting(QCompleter::UnsortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->popup()->setAlternatingRowColors(true);
    connect(completer, SIGNAL(activated(const QModelIndex &)), this, SLOT(completerActivated(const QModelIndex &)));

    cursorLineColor = QColor(Qt::yellow).lighter(180);
    pairMatchColor = QColor(Qt::red);
    pairNoMatchBgColor = QColor(Qt::yellow);
    completionStackedColor = QColor(Qt::cyan);
    completionBaseColor = QColor(Qt::darkMagenta);
    completionSelectColor = QColor(Qt::magenta);
    runningLineColor = QColor(Qt::green).lighter(180);
    breakpointLineColor = QColor(Qt::red).lighter(160);

    setWrapMode(isWrapMode);

    connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(documentBlockCountChange(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateSideArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChange()));

    contextEval->setShortcut(tr("Ctrl+Alt+I"));
    connect(contextEval, SIGNAL(triggered()), this, SLOT(doInteractiveEval()));

    updateSideAreaWidth();
    updateHighlights();
}


int TDriverCodeTextEdit::noNameIdCounter = 0;
int TDriverCodeTextEdit::getNoNameIdCounter() {
    return noNameIdCounter;
}


void TDriverCodeTextEdit::setUsingTabulatorsMode(bool enabled)
{
    if (enabled != isUsingTabulatorsMode) {
        isUsingTabulatorsMode = enabled;
        emit modesChanged();
    }
}


void TDriverCodeTextEdit::setRubyMode(bool enabled)
{
    if (enabled != isRubyMode) {
        isRubyMode = enabled;
        //        if (enabled)
        //            isUsingTabulatorsMode = false;
        emit modesChanged();
    }
}


void TDriverCodeTextEdit::setWrapMode(bool enabled)
{
    setWordWrapMode(enabled ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    if (enabled != isWrapMode) {
        isWrapMode = enabled;
        //        if (enabled)
        //            isUsingTabulatorsMode = false;
        emit modesChanged();
    }
}


int TDriverCodeTextEdit::sideAreaWidth()
{
    // first count digits needed for last line number
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // then get pixel size for that
    int space = 2; // border
    space += fontMetrics().width(QLatin1Char('0')) * digits; // line number

    space += fontMetrics().height(); // breakpoint and running line symbols

    return space;
}


void TDriverCodeTextEdit::updateSideAreaWidth()
{
    setViewportMargins(sideAreaWidth(), 0, 0, 0);
}


void TDriverCodeTextEdit::updateSideArea(const QRect &rect, int dy)
{
    if (dy) {
        sideArea->scroll(0, dy);
    }
    else {
        sideArea->update(0, rect.y(), sideArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateSideAreaWidth();
    }
}


void TDriverCodeTextEdit::cancelCompletion(bool adjustTextCursor)
{
    completionType = NO_COMPLETION;
    complPopupShowingInfo = false;
    if (completer->popup()->isVisible())
        completer->popup()->hide();
    if (adjustTextCursor && !complCur.isNull()) {
        complCur.clearSelection();
        setTextCursor(complCur);
    }

    lastBaseText.clear();
    baseStart = -1;
    if (!complCur.isNull()) complCur = QTextCursor();
    stackHighlightStart = -1;
    complCharStack.clear();
    complStartStack.clear();
}


bool TDriverCodeTextEdit::completerActivated()
{
    if (complPopupShowingInfo || completer->popup()->isHidden()) {
        cancelCompletion();
        return false; // can't activate in info mode
    }

    //qDebug() << FCFL << completer->popup()->currentIndex() << completer->currentIndex();
    if (completer->popup()->currentIndex().row() < 0) {
        if (completer->completionCount() == 1) {
            // no completion selected in popup, but only one valid completion: use it
            completerActivated(completer->currentIndex());
            return true;
        }
        else {
            // no completion selected in popup, with more than one valid completion
#if 0
            // cancel
            cancelCompletion();
            return false; // can't activate in info mode
#else
            // ignore
#endif
        }
    }
    else {
        // a completion selected in popup: use it
        completerActivated(completer->popup()->currentIndex());
    }
    return true;
}


void TDriverCodeTextEdit::completerActivated(const QModelIndex &index)
{
    if (complPopupShowingInfo) {
        if (completionType == NO_COMPLETION)
            cancelCompletion();
        return; // can't activate in info mode
    }

    if (isReadOnly() || complCur.isNull() || index.row() < 0) {
        qDebug() << FCFL << "warning! called with index " << index << " when readonly" << isReadOnly() << ", complCur.isNull" << complCur.isNull() << ", index.row" << index.row();
        cancelCompletion();
        return;
    }

    //qDebug() << FCFL << "EditRole" << index.data(Qt::EditRole) << "UserRole1" << index.data(Qt::UserRole+1);
    QString text(index.data(Qt::UserRole+1).toString());

    switch (completionType) {

    case TRANSLATION_COMPLETION:
        insertAtComplCursor(text);
        cancelCompletion();
        break;

    case BASIC_COMPLETION:
        insertAtComplCursor(text);
        setTextCursor(complCur);
        complCur.setPosition(baseStart, QTextCursor::KeepAnchor);
        splitToComplCursors(complCur);
        lastBaseText.clear();

        popupCompleterInfo(tr("CTRL+I to continue completion."));
        updateHighlights();
        break;

    default:
        qDebug() << FCFL << "warning! called with bad completionType" << completionType    << ", index=" << index;
        cancelCompletion();
    }
}


// TODO: these should be configurable and at least not static variables
static int indentSizeForTabMode = 8;
static int indentSizeForSpaceMode = 2;


static inline bool adjustSpaceIndent(int &spaceIndent, QChar ch)
{
    if (ch.isSpace()) {
        if (ch == '\t') {
            spaceIndent = (spaceIndent - (spaceIndent % indentSizeForTabMode)) + indentSizeForTabMode;
        }
        else {
            ++spaceIndent;
        }
        return true;
    }
    else return false;
}


static inline void countIndentation(const QString &line, bool isTabMode, int &indentLevel, int &indentChars)
{
    int indentSize = (isTabMode) ? indentSizeForTabMode : indentSizeForSpaceMode;
    // counts indentation level (eg 1 if there are only spaces and their count is indentSize)
    int spaceIndent = 0;
    int skip = 0;

    static QVector<QChar> specials(
                QVector<QChar>() << QChar::ParagraphSeparator << QChar::LineSeparator );
    while (skip < line.length() && specials.contains(line.at(skip))) {
        ++skip;
    }

    int pos = skip;

    while (pos < line.length()) {
        QChar ch = line.at(pos);

        if (!adjustSpaceIndent(spaceIndent, ch)) {
            break;
        }
        ++pos;
    }

    indentChars = pos-skip;
    indentLevel = (spaceIndent+indentSize-1)/indentSize; // round indentLevel up
}


static inline QChar countSpaceIndentation(const QString &line, int &spaceIndent)
{
    QChar ch; // creates null QChar
    spaceIndent = 0;

    for (int pos = 0; pos < line.size(); ++pos) {
        ch = line.at(pos);
        if (!adjustSpaceIndent(spaceIndent, ch))
            break;
    }

    return ch;
}


static inline int spaceIndentToPos(const QString &line, int spaceIndent)
{
    int pos = 0;
    int spcInd = 0;

    while (spaceIndent > spcInd && pos < line.size()) {
        QChar ch = line.at(pos);
        if (!adjustSpaceIndent(spcInd, ch))
            break;
        ++pos;
    }

    return pos;
}


static inline int countIndentChars(const QString &line)
{
    int ii;
    for(ii=0 ; ii < line.size(); ++ii) {
        if(!line.at(ii).isSpace()) break;
        QChar::Category cat = line[ii].category();
        if( cat == QChar::Separator_Line || cat == QChar::Separator_Paragraph ) break;
    }
    return ii;
}


void TDriverCodeTextEdit::doAutoIndent(QKeyEvent *event)
{
    if (isReadOnly()) return;
    if (event->key() != Qt::Key_Return && event->key() != Qt::Key_Enter) return; // not newline
    QTextBlock bl = textCursor().block().previous();
    if (!bl.isValid()) return; // no valid previous block
    insertAtTextCursor(bl.text().left(countIndentChars(bl.text())));
}


static inline QString stripQuotes(const QString &str)
{
    int chopLeft = 0;
    int chopRight = 0;
    if (!str.isEmpty()) {
        QChar first = str[0];
        if (first == '\'' || first == '"') {
            chopLeft = 1;
            if (str[str.size()-1] == first) {
                chopRight = 1;
            }
        }
    }

    return str.mid(chopLeft, str.size()-chopRight-chopLeft);
}


static inline void completerCompleteRect(QCompleter *completer, QRect rect)
{
    rect.setWidth( completer->popup()->sizeHintForColumn(0) + completer->popup()->verticalScrollBar()->sizeHint().width());
    completer->complete(rect);
}


static inline void completerResize(QCompleter *completer)
{
    //rect.setHeight(completer->popup()->sizeHintForRow(0) +  completer->popup()->horizontalScrollBar()->sizeHint().height());
    int limitedCount = qMin(5, completer->completionCount());
    int fontHeight = completer->popup()->fontMetrics().height();

    completer->popup()->resize(QSize(
                                   completer->popup()->sizeHintForColumn(0) +  completer->popup()->verticalScrollBar()->sizeHint().width(),
                                   limitedCount * fontHeight + completer->popup()->horizontalScrollBar()->sizeHint().height()));
}


bool TDriverCodeTextEdit::getTranslationParameter(const QString &paramKey, const QString &settingKey, const QMap<QString, QString> &tdriverParamMap, const QMap<QString, QString> &sutParamMap, QString &paramValue)
{
    bool ret = false;

    if (!paramKey.isEmpty() && sutParamMap.contains(paramKey)) {
        paramValue = stripQuotes(sutParamMap.value(paramKey));
        ret = true;
    }
    else if (!paramKey.isEmpty() && tdriverParamMap.contains(paramKey)) {
        paramValue = stripQuotes(tdriverParamMap.value(paramKey));
        ret = true;
    }
    else {
        if (!settingKey.isEmpty()) {
            QSettings settings;
            if (settings.contains(settingKey)) {
                paramValue = settings.value(settingKey).toString();
                ret = true;
            }
        }
    }

    return ret;
}


bool TDriverCodeTextEdit::setTranslationDatabase(const QMap<QString, QString> &tdriverParamMap, const QMap<QString, QString> &sutParamMap)
{
#ifdef TDRIVER_NO_SQL
    return false;
#else
    // tdriver_parameters.xml parameter names
    static const char *PK_HOST = "localisation_server_ip";
    static const char *PK_NAME = "localisation_server_database_name";
    static const char *PK_TABLE = "localisation_server_database_tablename";
    static const char *PK_USER = "localisation_server_username";
    static const char *PK_PASSWORD = "localisation_server_password";
    static const char *PK_LANGUAGE = "language";

    // QSettings configuration file parameter names
    static const char *SK_HOST = "translationdb/host";
    static const char *SK_NAME = "translationdb/name";
    static const char *SK_TABLE = "translationdb/table";
    static const char *SK_USER = "translationdb/user";
    static const char *SK_PASSWORD = "translationdb/password";
    static const char *SK_LANGUAGES = "translationdb/languages";

    translationDBerrors.clear();

    if (!getTranslationParameter(PK_HOST, SK_HOST, tdriverParamMap, sutParamMap, translationDBHost))
        translationDBerrors << tr("database server not defined");

    if (!getTranslationParameter(PK_USER, SK_USER, tdriverParamMap, sutParamMap, translationDBUser))
        translationDBerrors << tr("username not defined");

    if (!getTranslationParameter(PK_PASSWORD, SK_PASSWORD, tdriverParamMap, sutParamMap, translationDBPassword))
        translationDBerrors << tr("password not defined");

    if (!getTranslationParameter(PK_NAME, SK_NAME, tdriverParamMap, sutParamMap, translationDBName))
        translationDBerrors << tr("database name not defined");

    if (!getTranslationParameter(PK_TABLE, SK_TABLE, tdriverParamMap, sutParamMap, translationDBTable))
        translationDBerrors << tr("database table not defined");

    QString langStr;
    if (getTranslationParameter(PK_LANGUAGE, QString(), tdriverParamMap, sutParamMap, langStr)) {
        translationDBLanguages.clear();
        if (!langStr.isEmpty()) {
            translationDBLanguages << langStr;
        }
    }
    else {
        QSettings settings;
        if (settings.contains(SK_LANGUAGES)) {
            translationDBLanguages = settings.value(SK_LANGUAGES).toStringList();
        }
    }

    if (translationDBLanguages.isEmpty()) {
        translationDBerrors << "no languages defined";
    }

    translationDBconfigured = translationDBerrors.isEmpty();
    if (!translationDBconfigured) {
        qWarning() << "Localisation database configuration errors:" << translationDBerrors;
    }

    return translationDBconfigured;
#endif
}


void TDriverCodeTextEdit::startTranslationCompletion(QKeyEvent* /*event*/)
{
#ifdef TDRIVER_NO_SQL
    return;
#else

    cancelCompletion();
    if (!translationDBconfigured) {
        QMessageBox::warning(
                    qobject_cast<QWidget*>(parent()),
                    tr("Database not configured"),
                    tr("Database configuration is missing or incomplete:\n* ") + translationDBerrors.join("\n* "));
        return;
    }
    if (isReadOnly()) return;

    complCur = textCursor();
    if (!complCur.hasSelection() && !MEC::autoSelectQuoted(complCur)) {
        complCur = QTextCursor();
        return;
    }

    completionType = TRANSLATION_COMPLETION;

    QList<QStringList> translationStrings; // List of StringLists like [lname, translation, language]

    // item 0 in list is the original text
    translationStrings << ( QStringList() <<  complCur.selectedText() << complCur.selectedText() << tr("original"));

    QString withoutQuotes(stripQuotes(complCur.selectedText()));
    //qDebug() << FFL << "quotestripped" << withoutQuotes;

    bool dbOk;
    TDriverTranslationDb *translator = new TDriverTranslationDb();
    popupCompleterInfo("Connecting to database...");

    qDebug() << FCFL << "Connecting to database";
    if (translator->connectDB(translationDBHost, translationDBName, translationDBTable,
                              translationDBUser, translationDBPassword)) {
        QSet<QString> langSet(QSet<QString>::fromList(translationDBLanguages));
        QSet<QString> validLangSet(langSet);
        validLangSet.intersect(QSet<QString>::fromList(translator->getAvailableLanguages()));
        //qDebug() << FCFL << translator->availableLanguages;
        langSet.subtract(validLangSet);
        bool showListDialog = false;
        if (!langSet.isEmpty()) {
            static bool alreadyWarnedInvalidLangs = false;
            if (!alreadyWarnedInvalidLangs) {
                QMessageBox::warning( qobject_cast<QWidget*>(parent()),
                                     "Translation Lookup Configuraton Problem",
                                     "Invalid languages in configuration:\n\n" + QStringList(langSet.toList()).join(", "));
                alreadyWarnedInvalidLangs = true;
                showListDialog = true;
            }
        }

        QStringList langs;
        if (validLangSet.isEmpty()) {
            static bool alreadyWarnedNoLangs = false;
            if(!alreadyWarnedNoLangs) {
                QMessageBox::warning( qobject_cast<QWidget*>(parent()),
                                     "Translation Lookup Configuraton Problem",
                                     "No valid languages configured!");
                alreadyWarnedNoLangs = true;
                showListDialog = true;
            }
        }
        else {
            langs = validLangSet.toList();
        }

        if(showListDialog) {
            QMessageBox::information(qobject_cast<QWidget*>(parent()),
                                     "Translation Lookup Configuraton Help",
                                     "Languages supported by current localization database:\n\n" + translator->availableLanguages.join(", "));
        }

        if (!langs.isEmpty()) {
            translator->setLanguageFilter(langs);

            qDebug() << FCFL << "Getting translations";
            translationStrings << translator->getTranslationsLike(withoutQuotes);
            dbOk = true;
        }
        else {
            qWarning("Translation database language list problem!");
            dbOk = false;
        }
    }
    else {
        qWarning("Translation database connection error!");
        dbOk = false;
    }
    delete translator; translator = NULL;


    if (!dbOk) {
        completionType = NO_COMPLETION;
        popupCompleterInfo("Database connection failed!");
    }

    else if (dbOk && translationStrings.size() == 1) {
        completionType = NO_COMPLETION;
        popupCompleterInfo("No matching logical string IDs found!");
    }

    else {
        //qDebug() << FFL << translationStrings;

        QStandardItemModel *model = new QStandardItemModel(completer);
        for(int ii = 0; ii<translationStrings.size(); ++ii)    {
            QString infoText(
                        MEC::textShortened(translationStrings[ii][0], 40, 0) + " \""
                        + MEC::textShortened(translationStrings[ii][1], 20, 10) +"\"");
            QString completionText(translationStrings[ii][0]);
            if (translationStrings[ii].size()>2) {
                infoText.append( " (" + translationStrings[ii][2] + ")"); // add lang
            }
            if (ii>0 &&
                    (ALWAYS_USE_RUBY_SYMBOLS ||rubyMode()) &&
                    (completionText.length() > 0 && completionText.at(0).isLetter())) {
                completionText.prepend(":"); // create Ruby symbol by adding colon
            }
            QStandardItem *stdItem = new QStandardItem();
            stdItem->setData(completionText, Qt::UserRole+1);
            stdItem->setData(infoText, Qt::EditRole);
            model->appendRow(stdItem);
        }
        //MEC::dumpStdModel(model);
        //model->setSortRole(Qt::EditRole);
        completer->setModel(model);
        complPopupShowingInfo = false;
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        setTextCursor(complCur);
        completer->setCompletionPrefix(QString());

        completerCompleteRect(completer, cursorRect(complCur));
    }
#endif
}

bool TDriverCodeTextEdit::doTranslationCompletion(QKeyEvent *event)
{
    Q_ASSERT(completionType == TRANSLATION_COMPLETION);
    Q_ASSERT(!complCur.isNull());

    if (isReadOnly()) {
        cancelCompletion();
        return true;
    }

    bool handled = false;
    if (!doGenericCompletion(event, handled)) {
        // doGenericCompletion didn't handle the key
        // so abort the translation completion
        cancelCompletion();
    }
    return handled;
}


void TDriverCodeTextEdit::popupCompleterInfo(const QString &text)
{
    QStandardItemModel *model = new QStandardItemModel(completer);
    QStandardItem *stdItem = new QStandardItem();
    stdItem->setData(text, Qt::EditRole);
    model->appendRow(stdItem);

    completer->setCompletionPrefix("");
    complPopupShowingInfo = true;
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setModel(model);

    completerCompleteRect(completer, cursorRect(complCur));
}


void TDriverCodeTextEdit::popupBasicCompleter(QAbstractItemModel *model)
{
    //qDebug() << FCFL;
    Q_ASSERT(completionType = BASIC_COMPLETION);
    Q_ASSERT(!complCur.isNull());
    QString selText(complCur.selectedText());

    if (complCur.position() != complCur.selectionEnd()) {
        // make sure completion cursor is at the end of selection
        Q_ASSERT( complCur.position() == complCur.selectionStart());
        complCur.clearSelection();
        complCur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, selText.size());
    }

    QTextCursor tmpCur(complCur);
    tmpCur.clearSelection();
    setTextCursor(tmpCur);

    completer->setCompletionPrefix(selText);
    complPopupShowingInfo = false;
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setModel(model);

    completerCompleteRect(completer, cursorRect(complCur));
}


// returns base selection, ie. the part between baseStart and complCur.selectionStart
QString TDriverCodeTextEdit::splitToComplCursors(QTextCursor selection)
{
    Q_ASSERT(!selection.isNull());

    // init baseStart
    baseStart = selection.selectionStart();

    QString selText(selection.selectedText());
    //qDebug() << FCFL << CURSEL(complCur) << CURSEL(selection);

    static const QRegExp splitEx("[^0-9@A-Z_a-z!?]"); // note: ruby-specific...
    int baseLen = selText.lastIndexOf(splitEx)+1;
    int complLen = selText.size() - baseLen;

    // complCur: exclude base text, select compl text, position at selection end
    complCur = selection;
    complCur.setPosition(baseStart+baseLen, QTextCursor::MoveAnchor);
    complCur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, complLen);

    //qDebug() << FCFL << CURSEL(complCur) << CURSEL(selection);
    Q_ASSERT(baseStart <= complCur.selectionStart());
    Q_ASSERT(complCur.selectionEnd() == selection.selectionEnd());

    return selText.left(baseLen);
}


QTextCursor TDriverCodeTextEdit::joinComplCursors()
{
    QTextCursor cur;

    if (!complCur.isNull()) {
        cur = complCur;
        cur.setPosition(baseStart >= 0 ? baseStart : complCur.selectionStart());
        cur.setPosition(complCur.selectionEnd(), QTextCursor::KeepAnchor);
    }

    return cur;
}

bool TDriverCodeTextEdit::autoInteractiveCompletion()
{
    QTextCursor cur(textCursor());
    bool result = false;

    if (MEC::autoSelectExpression(cur)) {
        result = true; // going to return with selection in textCursor
    }

    else if (phraseModel) {

        if (!cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor) ||
                cur.selectedText().at(0).isSpace()) {
            // cursor is at beginning of document or after a whitespace (incl. line break),
            // show phrase completion popup immediately
            cur = textCursor();
            complCur = cur;
            cur.clearSelection();
            completionType = BASIC_COMPLETION;
            popupBasicCompleter(phraseModel);
        }
    }

    setTextCursor(cur);
    return result;
}


bool TDriverCodeTextEdit::doInteractiveCompletion(QKeyEvent *event)
{
    Q_ASSERT(completionType == BASIC_COMPLETION);
    Q_ASSERT(!complCur.isNull());

    if (isReadOnly()) {
        cancelCompletion();
        return true;
    }

    QString evText = event->text();
    bool handled = false;
    bool genCompletionHandled = doGenericCompletion(event, handled);

    if (!genCompletionHandled && !evText.isEmpty()) {
        // event not recognized by doGenericCompletion, and text is not empty

        bool doInsert = true;
        enum SpecialAction { Nothing, Restart, StackPush, StackPop } special = Nothing;
        QString selText(complCur.selectedText());
        char evChar = evText.at(0).toLatin1();

        if (event->key() == Qt::Key_Backspace) {
            if (selText.isEmpty()) {
                doInsert = false;
                complCur.deletePreviousChar();

                if (baseStart >= 0 && baseStart < complCur.position()) special = Restart;
                else if (!complCharStack.isEmpty()) special = StackPop;
                else {
                    // nothing within completion area to delete, cancel completion and return handled (deleted above)
                    cancelCompletion();
                    return false;
                }
            }
            else {
                evText.clear();
                selText.chop(1);
            }
        }
        else if (evChar == '.' || (evChar == ':' && selText.endsWith(':'))) {
            //qDebug() << FCFL << evChar;
            special = Restart;
        }

        else if (!complCharStack.isEmpty() && evChar == complCharStack.last() && !MEC::isAfterEscape(complCur)) {
            // must check for nesting end before start, or nestings with same start and end char won't work
            special = StackPop;
        }
        else if ((evChar == '(' || evChar == '{' || evChar == '[' || evChar == '|' || evChar == '"' || evChar == '\'')  && !MEC::isAfterEscape(complCur)) {
            special = StackPush;
        }
        else if (evChar < ' ') {
            // don't insert control chars
            doInsert = false;

            //if ((event->modifiers() & Qt::ControlModifier) != 0 && event->key() == Qt::Key_I) {
            if (evChar == '\t' ) {
                special = Restart;
            }
        }

        if (doInsert) {
            // replace completion selection with new text, and re-select
            // StackPop will do it's own insertion
            int selStart = complCur.selectionStart();
            insertAtComplCursor(selText + evText);
            setTextCursor(complCur);
            complCur.setPosition(selStart);
            complCur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, selText.size() + evText.size());
            updateHighlights();
        }

        if (special == StackPush) {
            QTextCursor stackCur(joinComplCursors());

            if (complStartStack.isEmpty()) {
                // initialize highlight start
                stackHighlightStart = stackCur.selectionStart();
            }

            complCharStack.append(MEC::getPair(evChar));
            complStartStack.append(stackCur.selectionStart());

            complCur.setPosition(complCur.selectionEnd());
            baseStart = -1;
            special = Restart;
        }
        else if (special == StackPop) {
            int pos = complCur.position();
            complCharStack.removeLast();
            complCur.setPosition(complStartStack.takeLast());
            complCur.setPosition(pos, QTextCursor::KeepAnchor);
            baseStart = -1;
            special = Restart;
        }

        if (special==Restart) {
            Q_ASSERT(!complCur.isNull()); // non-NULL means "restart"
            tryInteractiveCompletion(event);
        }
        else if (!complPopupShowingInfo) {
            completer->setCompletionPrefix(selText);
            completerResize(completer);
            QTextCursor tmpCur(complCur);
            tmpCur.clearSelection();
            setTextCursor(tmpCur);
        }

        handled = true;
    }

    return handled;
}


void TDriverCodeTextEdit::tryInteractiveCompletion(QKeyEvent *)
{
    if (isReadOnly()) {
        cancelCompletion();
        return;
    }

    if (completionType != BASIC_COMPLETION) {
        // can/initialize if not already doing basic completion
        cancelCompletion();
    }

    if (!complCur.isNull()) {
        // complCur not null indicates restart with current cursors:
        // take selection from base- and complCurs, don't touch cursor stack
        int oldComplEnd = complCur.selectionEnd();
        int start = (baseStart >= 0) ? baseStart : complCur.selectionStart();

        complCur.setPosition(start);
        complCur.setPosition(oldComplEnd, QTextCursor::KeepAnchor);
        setTextCursor(complCur);
        complCur = QTextCursor();
        baseStart = -1;
    }
    else {
        cancelCompletion();
        if (!textCursor().hasSelection() && !autoInteractiveCompletion()) {
            // there was no selection,
            // and autoInteractiveCompletion didn't select text suitable for interactive completion.
            // note: it may have popped up other completion popup, like phrases
            return;
        }
    }

    QTextCursor tmpCur(textCursor());

    completionType = BASIC_COMPLETION;
    QString newBaseText = splitToComplCursors(tmpCur);
    tmpCur.setPosition(tmpCur.selectionEnd(), QTextCursor::MoveAnchor);
    setTextCursor(tmpCur);

    if (newBaseText.isEmpty() && complCur.selectedText().isEmpty()) {
        popupCompleterInfo(tr("Type text to start completion..."));
        lastBaseText.clear();
    }
    else if (lastBaseText == newBaseText) {
        popupCompleterInfo(tr("Searching completions for:\n") + lastBaseText);
        emit requestInteractiveCompletion(MEC::replaceUnicodeSeparators(lastBaseText).toLocal8Bit());
    }
    else {
        popupCompleterInfo(tr("CTRL+I to complete:") + newBaseText);
        lastBaseText = newBaseText;
    }

}


void TDriverCodeTextEdit::popupInteractiveCompletion(QObject *client, QByteArray statement, QStringList completions)
{
    Q_UNUSED(statement);
    if (client != this) return; // not for us

    //qDebug() << FCFL << "completionType" << completionType;

    if (!completer->popup()->isVisible() || completionType != BASIC_COMPLETION || complCur.isNull()) return; // obsolete signal received

    QStandardItemModel *model = new QStandardItemModel(completer);

    for(int ii = 0; ii<completions.size(); ++ii) {
        QString completionText(completions[ii].trimmed());
        if (completionText.isEmpty()) continue;

        QStandardItem *stdItem = new QStandardItem();
        stdItem->setData(completionText, Qt::UserRole+1);
        stdItem->setData(completionText, Qt::EditRole);
        model->appendRow(stdItem);
    }

    if (model->rowCount() > 0)
        popupBasicCompleter(model);
    else
        cancelCompletion();
}


void TDriverCodeTextEdit::errorInteractiveCompletion(QObject *client, QByteArray statement, QStringList completions)
{
    Q_UNUSED(completions);
    QString stmStr(QString::fromLocal8Bit(statement));
    if (client != this) return; // not for us
    //qDebug() << FCFL << "got" << statement << "/" << completions;
    if (completionType != BASIC_COMPLETION) return; // obsolete

    //qDebug() << FCFL << "base" << lastBaseText << "vs statement" << statement;
    if (lastBaseText != stmStr) return;

    qDebug() << FCFL << "canceling completion";
    cancelCompletion();
}


bool TDriverCodeTextEdit::doGenericCompletion(QKeyEvent *event, bool &handled)
{
    if (Config.unhandledCancelKeys.contains(event->key())) {
        cancelCompletion();
        return true;
    }

    if (Config.handledCancelKeys.contains(event->key())) {
        cancelCompletion();
        handled = true;
        return true;
    }

    if (Config.complAcceptKeys.contains(event->key())) {
        // if no choice is active in popup, but handledAcceptKey is pressed, and there's exactly one choice, accept it
        handled = completerActivated();
        return true;
    }

    // didn't do anything, return false
    return false;
}


void TDriverCodeTextEdit::keyPressEvent(QKeyEvent *event)
{
    QTextCursor oldComplCur(complCur);

    bool handled = false;

    bool textEmpty = event->text().isEmpty();

    if (!textEmpty && !handled && (event->modifiers() == Qt::ControlModifier+Qt::AltModifier) && event->key() == Qt::Key_I) {
        doInteractiveEval();
        handled = true;
    }

    if (!textEmpty && completionType == NO_COMPLETION && event->modifiers() == Qt::ControlModifier) {
        // test hotkeys for starting different intellisense / completion modes
        if (event->key() == Qt::Key_L) {
            startTranslationCompletion(event);
            handled = true;
        }

        else if (event->key() == Qt::Key_I) {
            Q_ASSERT(complCur.isNull()); // this not being null indicates restart, shouldn't happen here
            tryInteractiveCompletion(event);
            handled = true;
        }
    }


    if (!textEmpty && !handled && completionType != NO_COMPLETION) {
        switch (completionType) {
        case BASIC_COMPLETION:
            handled = doInteractiveCompletion(event);
            break;
        case TRANSLATION_COMPLETION:
            handled = doTranslationCompletion(event);
            break;
        default:
            // nothing
            break;
        }
    }

    if (!textEmpty && !handled) {
        handled = doTabHandling(event);
    }

    if (!handled) {
        //qDebug() << FCFL << "calling parent" << event->modifiers() << event->text();
        QPlainTextEdit::keyPressEvent(event);
        if (!textEmpty) doAutoIndent(event);
    }

    if (!oldComplCur.isCopyOf(complCur)) {
        updateHighlights();
    }
}


void TDriverCodeTextEdit::doInteractiveEval()
{
    QTextCursor cur(textCursor());

    if (!cur.hasSelection()) {
        QTextCursor cur2 = joinComplCursors();
        if (cur2.hasSelection()) {
            cur = cur2;
        }
        else {
            cur.select(QTextCursor::BlockUnderCursor);
            if (!cur.hasSelection()) {
                return;
            }
        }
    }

    QString selText(cur.selectedText());
    //qDebug() << FCFL << "emit requestInteractiveEvaluation" << cur.selectedText();
    if (!selText.trimmed().isEmpty()) {
        emit requestInteractiveEvaluation(MEC::replaceUnicodeSeparators(selText).toLocal8Bit());
    }
}


void TDriverCodeTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    QTextCursor cur(textCursor());

    if (!cur.hasSelection())
        cur.select(QTextCursor::BlockUnderCursor);

    QString selText(cur.selectedText());

    if (!selText.trimmed().isEmpty()) {
        MEC::replaceUnicodeSeparators(selText);
        contextEval->setText(tr("Evaluate: '%1'").arg(MEC::textShortened(selText,    40, 20)));
        contextEval->setToolTip(selText);
        //contextEval->setStatusTip(cur.selectedText());

        menu->addAction(contextEval);
    }
    menu->exec(event->globalPos());
    delete menu;
}


void TDriverCodeTextEdit::cursorPositionChange()
{
    if (ignoreCursorPosChanges) return;

    QTextCursor cur(textCursor());
    bool hlUpdate = false;

    //qDebug() << FCFL << cur.position() << completionType;
    if (completionType != NO_COMPLETION) {
        Q_ASSERT(!complCur.isNull());


        if (cur.position() < complCur.selectionStart() || cur.position() > complCur.selectionEnd() ) {
            //qDebug() << FCFL << "cancelCompletion";
            cancelCompletion();
            hlUpdate = true;
        }
        else {
            cur.setPosition(complCur.selectionEnd());
            cur.clearSelection();
            setTextCursor(cur);
        }
    }
    else {
        int block = cur.block().blockNumber();
        if (block != lastBlock) {
            hlUpdate = true;
            lastBlock = block;
        }
    }

    if (needRehighlightAfterCursorPosChange) {
        hlUpdate = true;
        needRehighlightAfterCursorPosChange = false;
    }

    if (!hlUpdate) hlUpdate = testBlockDelimiterHighlight();

    if (hlUpdate) updateHighlights();
}

void TDriverCodeTextEdit::madeCurrent()
{
    //qDebug() << FCFL << fileName();
    // if this has focus or is visible, start using highlighter
    if (highlighter) {
        highlighter->setDocument(document());
        //qDebug() << FCFL << "doing rehighlight";
        highlighter->rehighlight();
    }
}


bool TDriverCodeTextEdit::doFind(QString findText, QTextDocument::FindFlags options)
{
    QTextCursor cur(textCursor());
    bool ret = doFind(findText, cur, options);
    setTextCursor(cur);
    return ret;
}


static inline void copyTextCursorSelection(QTextCursor &dest, const QTextCursor &src) {
    // assumes src and dest have same document()
    dest.setPosition(src.anchor());
    dest.setPosition(src.position(), QTextCursor::KeepAnchor);
}


bool TDriverCodeTextEdit::doFind(QString findText, QTextCursor &cur, QTextDocument::FindFlags options)
{
    if (cur.document() != document() || findText.isEmpty()) return false;

    QTextCursor foundCur = document()->find(findText, cur, options);

    if (foundCur.isNull()) {
        // wrap search
        lastFindWrapped = true;
        bool back = (options & QTextDocument::FindBackward);
        foundCur = cur;
        foundCur.movePosition(back ? QTextCursor::End : QTextCursor::Start);

        foundCur = document()->find(findText, foundCur, options);
        if (foundCur.isNull()) {
            cur.setPosition(back ? cur.selectionStart() : cur.selectionEnd());
            return false;
        }
    }
    else {
        lastFindWrapped = false;
    }
    copyTextCursorSelection(cur, foundCur);
    return true;
}


bool TDriverCodeTextEdit::doReplaceFind(QString findText, QString replaceText, QTextCursor &cur, QTextDocument::FindFlags options)
{
    if (cur.document() != document() || isReadOnly() || findText.isEmpty()) return false;

    if (cur.hasSelection()) {
        Qt::CaseSensitivity caseSens = (options & QTextDocument::FindCaseSensitively)
                ? Qt::CaseSensitive
                : Qt::CaseInsensitive;
        if (0 == QString::compare(cur.selectedText(), findText, caseSens)) {
            int pos = cur.selectionStart();
            cur.insertText(replaceText);
            if ((options & QTextDocument::FindBackward)) cur.setPosition(pos);
        }
    }
    return doFind(findText, cur, options);
}


bool TDriverCodeTextEdit::doIncrementalFind(QString findText, QTextDocument::FindFlags options)
{
    {
        QTextCursor cur(textCursor());
        if (cur.hasSelection() && cur.position() != cur.selectionStart()) {
            cur.setPosition(cur.selectionStart());
            setTextCursor(cur);
        }
    }
    return doFind(findText, options);
}


bool TDriverCodeTextEdit::doReplaceFind(QString findText, QString replaceText, QTextDocument::FindFlags options)
{
    QTextCursor cur(textCursor());
    bool ret = doReplaceFind(findText, replaceText, cur, options);
    setTextCursor(cur);
    return ret;
}


void TDriverCodeTextEdit:: doReplaceAll(QString findText, QString replaceText, QTextDocument::FindFlags options)
{
    if (isReadOnly() || findText.isEmpty()) return;

    QTextCursor cur(textCursor());

    // wraparound logic requires forward direction in replace:
    options &= ~QTextDocument::FindBackward;
    if (cur.hasSelection()) cur.setPosition(cur.selectionStart());

    if (doFind(findText, cur, options)) {
        // first occurence to be replaced is now selected
        // startMarker position must be set after replacing it, or replace will move it!
        QTextCursor startMarker(cur);
        int startPos = cur.selectionStart();

        bool wrapped = false;
        lastFindWrapped = false; // reset the internal wrap indicator flag

        cur.beginEditBlock();
        while (doReplaceFind(findText, replaceText, cur, options)) {
            if (lastFindWrapped) {
                if (wrapped) break; // safeguard, this probably shouldn't happen ever
                else wrapped = true;
            }
            if (startPos >= 0) {
                startMarker.setPosition(startPos);
                startPos = -1;
            }
            if (wrapped && startMarker.position() <= cur.position()) break;
        }
        cur.endEditBlock();
    }

    setTextCursor(cur);
}


void TDriverCodeTextEdit::commentCode()
{
    QTextCursor cur = textCursor();
    if (!cur.hasSelection()) {
        cur.select(QTextCursor::BlockUnderCursor);
    }

    bool doCommenting = false;
    int commentSpaceIndent = -1;

    QString text = cur.selectedText();
    MEC::replaceUnicodeSeparators(text);
    QStringList lines = text.split("\n", QString::KeepEmptyParts);
    foreach (const QString &line, lines) {
        int spcInd = 0;
        bool blankLine = MEC::isBlankLine(line);
        if (!doCommenting && !blankLine) {
            QChar firstRealChar = countSpaceIndentation(line, spcInd);
            if (firstRealChar != '#') {
                // line does not start with comment and is not blank line
                // -> do commenting instead of uncommenting
                doCommenting = true;
            }
        }

        if ((!blankLine && spcInd < commentSpaceIndent) || commentSpaceIndent < 0 ) {
            // keep track of position where column of comment marks is to be placed
            commentSpaceIndent=spcInd;
        }
    }

    for(int ii=0; ii < lines.size(); ++ii) {
        QString &line = lines[ii];
        if(doCommenting) {
            if (!line.isEmpty()) { // don't comment completely empty lines
                int pos = spaceIndentToPos(line, commentSpaceIndent);
                if (line.length()) {
                    line.insert(pos, '#');
                }
            }
        }
        else {
            int dummy;
            int whitespaces;
            countIndentation(line, isUsingTabulatorsMode, dummy, whitespaces);
            if (whitespaces < line.length() && line.at(whitespaces) == '#') {
                // remove first char after whitespaces
                line.remove(whitespaces, 1);
            }
        }
    }

    cur.insertText(lines.join("\n"));
}


void TDriverCodeTextEdit::setRunning(bool state)
{
    //qDebug() << FCFL << fileName() << state;
    isRunning = state;
    updateHighlights();
}


void TDriverCodeTextEdit::setHighlighter(TDriverHighlighter *h)
{
    //qDebug() << FCFL << (void*)highlighter << "->" << (void*)h;
    if (highlighter != h) {
        if (highlighter && highlighter->document() == document()) {
            // remove existing highlighter
            highlighter->setDocument(NULL);
        }
        highlighter = h;
    }
}


void TDriverCodeTextEdit::setFileName(QString fn, bool onlySetModes)
{
    if (fn.isEmpty()) {
        static int uniqnoname = 0;
        uniqnoname++;
        setObjectName("editor edit noname"+QString::number(uniqnoname));
        disableWatcher();
    }
    else {
        if (!onlySetModes) {
            disableWatcher();
            fname = MEC::fileWithPath(fn);
            enableWatcher();
            setObjectName("editor edit " + fn);
        }
        if (!isRubyMode && (fn.endsWith(".rb") || fn.endsWith(".rb.template"))) {
            setRubyMode(true);
            setUsingTabulatorsMode(false);
            setWrapMode(false);
            emit modesChanged();
        }
    }
    if (watcher) qDebug() << FCFL << fname << "watching changes:" << watcher->files() << watcher->directories();
    else qDebug() << FCFL << "not watching for changes";
}


void TDriverCodeTextEdit::focusInEvent(QFocusEvent *event)
{
    if (highlighter) {
        highlighter->setDocument(document());
    }
    QPlainTextEdit::focusInEvent(event);
}


void TDriverCodeTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    sideArea->setGeometry(QRect(cr.left(), cr.top(), sideAreaWidth(), cr.height()));
}


void TDriverCodeTextEdit::updateHighlights()
{
    update(sideArea->rect());

    if (needSyntaxRehighlight) doSyntaxHighlight();

    QList<QTextEdit::ExtraSelection> extraSelections;

    highlightCursorLine(extraSelections);

    if (isRunning) {
        highlightBreakpointLines(extraSelections);
        highlightRunningLine(extraSelections);
    }

    highlightCompletionCursors(extraSelections);

    highlightBlockDelimiters(extraSelections);

    setExtraSelections(extraSelections);
}


void TDriverCodeTextEdit::doSyntaxHighlight()
{
    if (highlighter) {
        highlighter->rehighlight();
        // TODO: if completion selection becomes sluggish, do highlight only to visible blocks,
        // see sideAreaPaintEvent for example
        needSyntaxRehighlight = false;
    }
}


bool TDriverCodeTextEdit::testBlockDelimiterHighlight()
{
    QTextCursor cur(textCursor());
    bool ok = MEC::blockDelimiterOrWordUnderCursor(cur);
    if(!ok) return false;

    QString delimStr = cur.selectedText();

    if (delimStr.size() == 1) {
        static const QString delimCharStr(")(}{][");
        return delimCharStr.contains(delimStr.at(0));
    }
#if 0
    // totally incomplete code
    else return (delimStr == "end");
    static const QStringList delimWords(
                QStringList()
                << "end" << "do"
                << "begin" << "rescue"
                << "if" << "then" << "else" << "unless"
                << "while" << "until");
    return delimWords.contains(delimStr);
#else
    // keyword pair matching not implemented
    return false;
#endif
}

void TDriverCodeTextEdit::highlightBlockDelimiters(QList<QTextEdit::ExtraSelection> &extraSelections)
{
    QTextCursor cur(textCursor());
    if(MEC::blockDelimiterOrWordUnderCursor(cur)) {

        QString delimStr = cur.selectedText();
        Q_ASSERT(!delimStr.isEmpty());

        if (delimStr.size() == 1) {
            static const QString delimCharStr("([{}])");
            QChar delimCh = delimStr.at(0).toLatin1();

            if (delimCharStr.contains(delimCh)) {
                QTextEdit::ExtraSelection selection;
                selection.cursor = cur;
                selection.format.setForeground(pairMatchColor);

                if (MEC::findNestedPair(delimCh.toLatin1(), cur)) {
                    // highlight both members of brace char pair with same color
                    QTextEdit::ExtraSelection selection2;
                    selection2.format.setForeground(pairMatchColor);
                    selection2.cursor = cur;
                    extraSelections.append(selection2);
                }
                else {
                    // highlight pairless brace char background to indicate possible error
                    selection.format.setBackground(pairNoMatchBgColor);
                }
                extraSelections.append(selection);

                // Setting flag below is needed, because inserted characters will copy char format
                // from previous character, resulting in pair matching color extending to newly
                // typed text until next highlight is done.
                needRehighlightAfterCursorPosChange = true;
            }
        }
#if 0
        else if (delimStr == "end") {
            QTextEdit::ExtraSelection selection;
            selection.cursor = cur;
            selection.format.setForeground(pairMatchColor);
            extraSelections.append(selection);
        }
#else
        // keyword pair matching not implemented
#endif
    }
}


void TDriverCodeTextEdit::highlightCursorLine(QList<QTextEdit::ExtraSelection> &extraSelections)
{
    if (isReadOnly()) return;

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(cursorLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
}


void TDriverCodeTextEdit::highlightCompletionCursors(QList<QTextEdit::ExtraSelection> &extraSelections)
{
    if (isReadOnly()) return;

    if (stackHighlightStart >= 0) {
        int pos = (baseStart >= 0) ? baseStart : complCur.selectionStart();
        Q_ASSERT(pos >= stackHighlightStart);

        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(completionStackedColor);
        selection.cursor = textCursor();
        selection.cursor.setPosition(stackHighlightStart);
        selection.cursor.setPosition(pos, QTextCursor::KeepAnchor);
        extraSelections.append(selection);
    }
    if (baseStart >= 0 && baseStart < complCur.selectionStart())    {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(completionBaseColor);
        selection.cursor = complCur;
        selection.cursor.setPosition(baseStart, QTextCursor::MoveAnchor);
        selection.cursor.setPosition(complCur.selectionStart(), QTextCursor::KeepAnchor);
        extraSelections.append(selection);
    }
    if (complCur.hasSelection())    {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(completionSelectColor);
        selection.cursor = complCur;
        extraSelections.append(selection);
    }
}


void TDriverCodeTextEdit::highlightRunningLine(QList<QTextEdit::ExtraSelection> &extraSelections)
{
    if (runningLine <= 0 || runningLine > document()->blockCount()) return;

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(runningLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = QTextCursor(document());
    selection.cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, runningLine-1);
    extraSelections.append(selection);
}


void TDriverCodeTextEdit::highlightBreakpointLines(QList<QTextEdit::ExtraSelection> &extraSelections)
{
    if (bpList.size() <= 0) return;

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(breakpointLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = QTextCursor(document());

    int lineNum = 0;
    for(int ind=0; ind < bpList.size(); ++ind) {
        if (lineNum == bpList[ind].line) continue; // don't add duplicate entries
        if (bpList[ind].num <= 0) continue; // don't highlight lines that aren't reported by rdebug
        lineNum = bpList[ind].line;
        if (lineNum <= 0 || lineNum >= document()->blockCount()) continue; // invalid line number

        //qDebug() << FFL "highlighting line" << lineNum;
        selection.cursor.movePosition(QTextCursor::Start);
        selection.cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNum-1);
        extraSelections.append(selection);
    }
}


bool TDriverCodeTextEdit::doTabHandling(QKeyEvent *event)
{
    static QSet<int> handledKeys(
                QSet<int>() << Qt::Key_Tab << Qt::Key_Backtab << Qt::Key_Backspace << Qt::Key_Delete);

    bool handled;
    // let compiler warn us if handled isn't initialized for all code paths

    QTextCursor tc(textCursor());

    if (!handledKeys.contains(event->key()) || (event->modifiers()|Qt::ShiftModifier) != Qt::ShiftModifier) {
        // not interested in this key or modifier other than shift is used
        handled = false;
    }

    else if ((isUsingTabulatorsMode || tc.hasSelection())
             && (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)) {
        // backspace and delete aren't handled specially when using tabulators or having a selection
        handled = false;
    }

    else if (tc.hasSelection() || event->key() == Qt::Key_Backtab) {
        // adjust indentation of entire block (or single line with backtab)
        if (!tc.hasSelection()) tc.select(QTextCursor::LineUnderCursor);
        indentSelection(tc, ( event->key()==Qt::Key_Tab));
        handled = true;
    }

    else if (isUsingTabulatorsMode) {
        // above did not match, no special handling in tabulator mode
        handled = false;
    }

    else if ( isReadOnly()) {
        // do nothing in readonly mode
        handled = true;
    }

    else {
        // adjust indentation
        int column = tc.columnNumber();
        tc.select(QTextCursor::BlockUnderCursor);
        int indLevel, indChars;
        countIndentation(tc.selectedText(), isUsingTabulatorsMode, indLevel, indChars);

        // increase or decrease indentation depending on key
        indLevel += ((event->key()==Qt::Key_Tab) ? +1 : -1);

        // check if indentation adjustment is valid
        if ( column <= indChars && indLevel >= 0 &&
                !(event->key()==Qt::Key_Backspace && column <=0 ) &&
                !(event->key()==Qt::Key_Delete && column >= indChars))
        {
            reindentSelectionStart(tc, indLevel, indChars);

            // set cursor column
            tc = textCursor();
            tc.movePosition(QTextCursor::StartOfLine);
            int indentSize = (isUsingTabulatorsMode) ? indentSizeForTabMode : indentSizeForSpaceMode;
            if (event->key()==Qt::Key_Backspace)
                column = (indentSize<column) ? column-indentSize : 0;
            else if (column > indentSize*indLevel || event->key()==Qt::Key_Tab || event->key()==Qt::Key_Backtab)
                column = indentSize*indLevel;
            // else restore original column
            tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
            setTextCursor(tc);
            handled = true;
        }
        else if (event->key()==Qt::Key_Backtab) {
            // avoid insertion of real tab by shift-tab
            handled = true;
        }
        else {
            handled = false;
        }
    }
    return handled;
}


void TDriverCodeTextEdit::indentSelection(QTextCursor tc, bool increaseIndentation)
{
    int lastBlock;
    {
        QTextCursor tempCur(tc);
        tempCur.setPosition(tc.selectionEnd());
        if (tempCur.isNull()) return;
        if (tempCur.position() == tempCur.block().position()) {
            // cursor is at the beginning of a QTextBlock, don't include the block
            tempCur.movePosition(QTextCursor::PreviousCharacter);
            // if above fails, it means selection is empty first line, proceed with indentation
        }
        lastBlock = tempCur.blockNumber();
    }
    tc.beginEditBlock();
    tc.setPosition(tc.selectionStart());

    do {
        int indLevel, indChars;
        tc.select(QTextCursor::BlockUnderCursor);
        countIndentation(tc.selectedText(), isUsingTabulatorsMode, indLevel, indChars);

        indLevel += ((increaseIndentation) ? 1 : -1);
        reindentSelectionStart(tc, indLevel, indChars);

    } while (tc.movePosition(QTextCursor::NextBlock) && (tc.blockNumber() <= lastBlock) );
    tc.endEditBlock();
}


void TDriverCodeTextEdit::reindentSelectionStart(QTextCursor tc, int indLevel, int indChars)
{
    // select current indentation (if any) and replace with correct number of spaces
    tc.movePosition(QTextCursor::StartOfBlock);
    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, indChars);

    ignoreCursorPosChanges = true;
    if (isUsingTabulatorsMode) {
        tc.insertText(QString(indLevel, '\t'));
    }
    else {
        tc.insertText(QString(indentSizeForSpaceMode * indLevel, ' '));
    }
    ignoreCursorPosChanges = false;
}


// helper function for sideAreaPaintEvent
bool TDriverCodeTextEdit::forwardSearch(int targetLine, int &ind)
{
    forever {
        if (ind >= bpList.size()) return false; // nothing more to be found in nums
        int lineNum = bpList[ind].line;
        if (targetLine < lineNum) return false; // target not in nums
        if (targetLine == lineNum) return true; //target found
        ++ind;
    }
}


bool TDriverCodeTextEdit::binarySearch(int targetLine, int &ind)
{
    int start = 0;
    int end = bpList.size();
    int indLine = -1;
    ind = 0;

    while (start < end) {
        ind = (start+end)/2;
        indLine = bpList[ind].line;
        if (targetLine == indLine) return true; // found!

        if (targetLine < indLine) end = ind;
        else start = ind+1;
    }

    // not found, make sure ind is the index where target would be inserted
    if (ind < bpList.size() && indLine < targetLine) ++ind;

    return false;
}


void TDriverCodeTextEdit::clearBreakpoints()
{
    for(int ind=0; ind < bpList.size(); ++ind) {
        qDebug() << FFL << "removing: " << MEC::dumpBreakpoint(&bpList[ind]);
        if (bpList[ind].num > 0) {
            //qDebug() << FFL << "emit removedBreakpoint for list index" << ind;
            emit removedBreakpoint(bpList[ind].num);
        }
    }

    bpList.clear();
    updateHighlights();
}


void TDriverCodeTextEdit::rdebugBreakpointReset()
{
    int setCapacity = bpList.size();

    for(int ind=0; ind < bpList.size(); ++ind) {
        bpList[ind].num = 0;
        const int thisLine = bpList[ind].line;
        // remove duplicates
        while (ind+1 < bpList.size() && bpList[ind+1].line == thisLine) {
            bpList.removeAt(ind+1);
        }
    }

    rdebugBpSet.clear();
    rdebugBpSet.reserve(setCapacity);

    updateHighlights();
}


void TDriverCodeTextEdit::rdebugBreakpoint(MEC::Breakpoint bp)
{
    qDebug() << FFL << MEC::dumpBreakpoint(&bp);

    if (bp.line < 1 || bp.line > document()->blockCount()) return;

    Q_ASSERT(fileName() == bp.file);

    if (rdebugBpSet.contains(bp.num)) {
        qDebug() << FFL << bp.num << "already added, ignoring" << bp.line;
        return;
    }

    int ind;
    if (binarySearch(bp.line, ind)) {
        // seek index of first breakpoint in line
        while (ind > 0 && bpList[ind-1].line == bp.line) --ind;

        if (bpList[ind].num == 0) {
            qDebug() << FFL << "setting rdebug breakpoint num";
            bpList[ind].num = bp.num;
            // assert that this is only breakpoint for line
            Q_ASSERT(ind+1 == bpList.size() || bpList[ind+1].line > bp.line);
        }
        else {
            // seek exact match or insertion point
            while (ind < bpList.size() && bpList[ind].num < bp.num) ++ind;

            // if non-existing breakpoint, insert as new
            if (bpList[ind].num != bp.num) {
                // TODO: handle enabled status of new breakpoint
                qDebug() << FFL << "adding new breakpoint";
                bpList.insert(ind, bp);
                rdebugBpSet.insert(bp.num);
            }
            else {
                qDebug() << FFL << "breakpoint already ok";
                // TODO: handle enable/disable of existing breakpoint
            }
        }
    }

    updateHighlights();
}

void TDriverCodeTextEdit::removeBreakpointLine(int lineNum)
{
    int ind;
    if (binarySearch(lineNum, ind)) {
        // seek index of first breakpoint in line
        while (ind > 0 && bpList[ind-1].line == lineNum) --ind;

        do {
            qDebug() << FFL << "removing: " << MEC::dumpBreakpoint(&bpList[ind]);
            if (bpList[ind].num > 0) {
                //qDebug() << FFL << "emit removedBreakpoint for list index" << ind;
                emit removedBreakpoint(bpList[ind].num);
                rdebugBpSet.remove(bpList[ind].num);
            }
            bpList.removeAt(ind);
        } while (ind < bpList.size() && bpList[ind].line == lineNum);
    }

    updateHighlights();
}


void TDriverCodeTextEdit::rdebugDelBreakpoint(int rdebugInd)
{
    if (!rdebugBpSet.contains(rdebugInd)) {
        qDebug() << FFL << rdebugInd << "not known, ignoring remove request";
        return;
    }

    int ind=0;
    int count = 0;
    while (ind < bpList.size()) {
        if (bpList[ind].num == rdebugInd) {
            ++count;
            qDebug() << FFL << "removing: " << MEC::dumpBreakpoint(&bpList[ind]);
            rdebugBpSet.remove(rdebugInd);
            bpList.removeAt(ind);
        }
        else ++ind;
    }

    Q_ASSERT(count == 1); // rdebugBpSet use must make sure this holds!

    updateHighlights();
    if (count > 1) qDebug() << FCFL << "Removed multiple breakpoints with same rdebug breakpoint index" << rdebugInd;
}


void TDriverCodeTextEdit::uiToggleBreakpointLine(int lineNum)
{
    int ind;
    if (binarySearch(lineNum, ind)) {
        // found
#if 0
        if (isRunning) {
            QMessageBox::warning(this, tr("TDriver Editor"),
                                 tr("Cannot remove breakpoints while script is running..."));
            return;
        }
#endif
        removeBreakpointLine(lineNum);
    }
    else {
        if (lineNum == document()->blockCount()) return; // last line not allowed
        qDebug() << FFL << "Inserting new breakpoint line" << lineNum << "(at ind" << ind << "/ size" << bpList.size() << ")";
        MEC::Breakpoint bp(0, true, fileName(), lineNum);
        bpList.insert(ind, bp);
        emit addedBreakpoint(bp);

        Q_ASSERT(ind == 0 || bpList[ind-1].line < lineNum);
        Q_ASSERT(ind+1 == bpList.size() || bpList[ind+1].line > lineNum);
    }

    updateHighlights();
}


void TDriverCodeTextEdit::dataSyncRequest()
{
    rdebugBreakpointReset();
    for(int ind=0; ind < bpList.size(); ++ind) {
        emit addedBreakpoint(bpList[ind]);
    }
}


void TDriverCodeTextEdit::gotoLine(int lineNum)
{
    if (lineNum > 0) {
        QTextCursor tc = textCursor();
        tc.movePosition(QTextCursor::Start);
        tc.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNum-1);
        setTextCursor(tc);
        centerCursor();
    }
}


void TDriverCodeTextEdit::setRunningLine(int lineNum)
{
    qDebug() << FFL << fileName() << lineNum << "running:" << isRunning;

    gotoLine(lineNum);

    if (lineNum == runningLine) return; // no change, avoid updates below

    runningLine = (lineNum > 0 && lineNum <= document()->blockCount()) ? lineNum : 0;
    updateHighlights();
    // TODO: change cursor position to running line? or just scroll document to running line without changing cursor position?
}


void TDriverCodeTextEdit::sideAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(sideArea);
    static QColor sideAreaFillColor(QColor(Qt::lightGray).lighter(130));
    painter.fillRect(event->rect(), sideAreaFillColor);
    painter.setPen(Qt::black);

    QTextBlock block = firstVisibleBlock();
    int blockNum = block.blockNumber();
    int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bot = top + (int) blockBoundingRect(block).height();

    int bpInd = 0;
    while (block.isValid() && top <= event->rect().bottom()) {

        if (block.isVisible() && bot >= event->rect().top()) {
            // line number
            QString num = QString::number(blockNum + 1);
            painter.setPen(Qt::darkGray);
            painter.setBrush(Qt::NoBrush);
            painter.drawText(0, top, sideArea->width(),
                             fontMetrics().height(), Qt::AlignRight, num);

            // breakpoint marker
            if (forwardSearch(blockNum+1, bpInd)) {
                static QBrush breakPointBrushes[2] = {
                    QBrush(Qt::red, Qt::Dense6Pattern), // disabled
                    QBrush(Qt::red, Qt::SolidPattern) }; // enabled

                painter.setBrush(breakPointBrushes[bpList[bpInd].enabled]);
                painter.setPen(Qt::darkRed);
                painter.drawEllipse(0, top, fontMetrics().height(), fontMetrics().height()-1);
            }

            // runningLine marker
            if (blockNum+1 == runningLine) {
                static QBrush runningLineMarkerBrush(Qt::green, Qt::SolidPattern);
                painter.setPen(Qt::darkGreen);
                painter.setBrush(runningLineMarkerBrush);
                static int halfSpanDegrees = 45;
                painter.drawPie(fontMetrics().height()/3, top-1,
                                fontMetrics().height()+4, fontMetrics().height()-1+2,
                                16*(180-halfSpanDegrees), 16*(2*halfSpanDegrees));
            }
        }

        block = block.next();
        top = bot;
        bot = top + (int) blockBoundingRect(block).height();
        ++blockNum;
    }
}


void TDriverCodeTextEdit::sideAreaMouseReleaseEvent(QMouseEvent *event)
{
    //qDebug() << FFL << "COORDS" << event->pos();
    QTextBlock block = firstVisibleBlock();
    int y = event->y();
    int top = 0;
    int bot = 0;

    while (block.isValid() && block.isVisible()) {
        //qDebug() << FFL << block.blockNumber();
        top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
        bot = top + (int) blockBoundingRect(block).height();

        if (y >= top && y < bot) break;
        block = block.next();
    }

    if (!(block.isValid() && block.isVisible())) {
        //qDebug() << FFL << "No match: out of blocks";
        return;
    }

    int lineNum = block.blockNumber()+1;
    //qDebug() << FFL << "MATCH" << lineNum;
    uiToggleBreakpointLine(lineNum);
}


void TDriverCodeTextEdit::documentBlockCountChange(int newCount)
{
    int adjust = newCount - lastBlockCount;
    lastBlockCount = newCount;

    int lineNum = textCursor().block().blockNumber()+1;
    //qDebug() << FFL << "got linenum" << lineNum << "adjust" << adjust;

    /* adjust breakpoints */
    for (int ind=bpList.size()-1 ; ind >= 0 && bpList[ind].line >= lineNum; --ind) {
        qDebug() << FFL << "adjusting breakpoint line number" << bpList[ind].line << "by" << adjust;
        bpList[ind].line += adjust;
    }

    updateSideAreaWidth();
}


void TDriverCodeTextEdit::fileChanged(const QString &path)
{
    qDebug() << FCFL << path;
    QString modifiedText = ( document()->isModified() )
        ? tr("Warning: The document in editor is marked as modified.")
        : tr("The document in editor does not have unsaved changed,\nbut it has been marked as modified now.");

    document()->setModified(true);

    QString text =                          tr("This file has changed on disk:")
            +QString("\n\n%1\n\n%2\n\n").arg(path, modifiedText)
            +tr("You should do one of these actions to resolve the situation:\n\n")
            +tr("- Save: overwrite the changed file on disk.\n")
            +tr("- Save as...: save version in editor with new name.\n")
            +tr("- Close: lose changes in the version in the editor.\n")
            +tr("- Revert: load the version on disk to editor.\n");

    QMessageBox::warning(this, tr("Open file has changed on disk"), text);
}

void TDriverCodeTextEdit::enableWatcher()
{
    if (watcher) {
        delete watcher;
        watcher = NULL;
    }

    if (!fname.isEmpty()) {
        watcher = new QFileSystemWatcher((QStringList() << fname), this);
        connect(watcher, SIGNAL(fileChanged(QString)), SLOT(fileChanged(QString)));
        qDebug() << FCFL << fname;
    }
    else {
        qDebug() << FCFL << "not enabled" << fname;
    }
}

void TDriverCodeTextEdit::disableWatcher()
{
    if (watcher) {
        qDebug() << FCFL << "with" << watcher->files() << watcher->directories();
        delete watcher;
        watcher = NULL;
    }
    else {
        qDebug() << FCFL << "already disabled";
    }
}


