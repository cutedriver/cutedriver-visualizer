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

#include <tdriver_util.h>
#include <tdriver_translationdb.h>
#include "tdriver_editor_common.h"

#define ALWAYS_USE_RUBY_SYMBOLS 1

#define SK_HOST "translationdb/host"
#define SK_NAME "translationdb/name"
#define SK_TABLE "translationdb/table"
#define SK_USER "translationdb/user"
#define SK_PASSWORD "translationdb/password"
#define SK_LANGUAGES "translationdb/languages"


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

static inline QStringList defaultLanguageList() { return QStringList() << "English-GB"; }
// removed: << "English-GB/English-US" << "Finnish"; }

static inline void verifySettings()
{
    if (! (MEC::settings->contains(SK_HOST) &&
           MEC::settings->contains(SK_NAME) &&
           MEC::settings->contains(SK_TABLE) &&
           MEC::settings->contains(SK_USER) &&
           MEC::settings->contains(SK_PASSWORD))) {
        qWarning() << "TDriverCodeTextEdit creating default translation database settings";
        MEC::settings->setValue(SK_HOST, "trtdriver1.nmp.nokia.com");
        MEC::settings->setValue(SK_NAME, "tdriver_locale");
        MEC::settings->setValue(SK_TABLE, "TB10_1_week16_loc");
        MEC::settings->setValue(SK_USER,"locale");
        MEC::settings->setValue(SK_PASSWORD, "password");
    }

    QVariant langvar = MEC::settings->value(SK_LANGUAGES);
    if(langvar.isNull() || !langvar.isValid() || !langvar.canConvert(QVariant::StringList)) {
        // settings contain something not understood, create default stringlist
        MEC::settings->setValue(SK_LANGUAGES,  defaultLanguageList());
    }
}


TDriverCodeTextEdit::TDriverCodeTextEdit(QWidget *parent) :
        QPlainTextEdit(parent),
        noNameId(++noNameIdCounter),
        sideArea(new SideArea(this)),
        highlighter(NULL),
        needReHighlight(false),
        completer(new QCompleter(this)),
        complPopupShowingInfo(false),
        phraseModel(NULL),
        stackHighlightStart(-1),
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
        ignoreCursorPosChanges(false)
{
    verifySettings();

    {
        QStringList phrases;
        MEC::DefinitionFileType type = MEC::getDefinitionFile(
                TDriverUtil::tdriverHelperFilePath("completions/completions_ruby_phrases.txt"), phrases);
        if (type != MEC::InvalidDefinitionFile) {
            //qDebug() << FCFL << "phrases" << phrases;
            phraseModel = new QStandardItemModel(this);
            foreach (QString phrase, phrases) {
                QStandardItem *stdItem = new QStandardItem();
                stdItem->setData(phrase, Qt::UserRole+1);
                stdItem->setData(MEC::textShortened(phrase.simplified(), 30, 20), Qt::EditRole);
                phraseModel->appendRow(stdItem);
            }
        }
    }

    completer->setWidget(this);
    completer->setWrapAround(true);
    completer->setModelSorting(QCompleter::UnsortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->popup()->setAlternatingRowColors(true);
    connect(completer, SIGNAL(activated(const QModelIndex &)), this, SLOT(completerActivated(const QModelIndex &)));


    //setAcceptDrops(true);
    cursorLineColor = QColor(Qt::yellow).lighter(180);
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


// TODO: these should be configurable and not static  variables
static int tabSize = 8;
static int indentSize = 2;

static inline void countIndentation(const QString &line, int &indentLevel, int &indentChars)
{
    int lineind = 0;
    int pos;

    for (pos = 0; pos < line.length(); ++pos) {
        char ch = line.at(pos).toAscii();

        if (ch == '\t') lineind = ((lineind + tabSize) % tabSize) * tabSize;
        else if (ch == ' ')    lineind += 1;
        else break;
    }

    indentChars = pos;
    indentLevel = (lineind+1)/indentSize;

    // qDebug() << FFL << indentLevel << indentChars;
}


static inline int countIndentChars(const QString &str)
{
    int i;
    for(i=0 ; i < str.size(); ++i) {
        if(!str[i].isSpace()) break;
        QChar::Category cat = str[i].category();
        if( cat == QChar::Separator_Line || cat == QChar::Separator_Paragraph ) break;
    }
    return i;
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


void TDriverCodeTextEdit::startTranslationCompletion(QKeyEvent */*event*/)
{
    cancelCompletion();
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
    if (translator->connect(
            MEC::settings->value(SK_HOST).toString(),
            MEC::settings->value(SK_NAME).toString(),
            MEC::settings->value(SK_TABLE).toString(),
            MEC::settings->value(SK_USER).toString(),
            MEC::settings->value(SK_PASSWORD).toString()
            )) {

        QSet<QString> langSet(QSet<QString>::fromList(MEC::settings->value(SK_LANGUAGES).toStringList()));
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
                langs = defaultLanguageList();
                QMessageBox::warning( qobject_cast<QWidget*>(parent()),
                                      "Translation Lookup Configuraton Problem",
                                      "No valid languages configured! Trying to use default list:\n\n" + langs.join(", "));
                alreadyWarnedNoLangs = true;
                showListDialog = true;
            }
        }
        else {
            langs = validLangSet.toList();
        }

        if(showListDialog) {
            langs = defaultLanguageList();
            QMessageBox::information(qobject_cast<QWidget*>(parent()),
                                     "Translation Lookup Configuraton Help",
                                     "Languages supported by current localization database:\n\n" + translator->availableLanguages.join(", "));
        }

        translator->setLanguageFilter(langs);

        qDebug() << FCFL << "Getting translations";
        translationStrings << translator->getTranslationsLike(withoutQuotes);
        dbOk = true;
    }
    else {
        qWarning() << FCFL << "Database connection error!";
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
        emit requestInteractiveCompletion(MEC::replaceUnicodeSeparators(lastBaseText));
    }
    else {
        popupCompleterInfo(tr("CTRL+I to complete:") + newBaseText);
        lastBaseText = newBaseText;
    }

}


void TDriverCodeTextEdit::popupInteractiveCompletion(QObject *client, QString statement, QStringList completions)
{

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


void TDriverCodeTextEdit::errorInteractiveCompletion(QObject *client, QString statement, QStringList completions)
{
    if (client != this) return; // not for us
    //qDebug() << FCFL << "got" << statement << "/" << completions;
    if (completionType != BASIC_COMPLETION) return; // obsolete

    //qDebug() << FCFL << "base" << lastBaseText << "vs statement" << statement;
    if (lastBaseText != statement) return;

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

    if (!textEmpty && !handled && !isUsingTabulatorsMode && !textCursor().hasSelection() &&
        (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) )  {

        if ( isReadOnly()) {
            // just ignore
            handled = true;
        }
        else {
            // perform special indentation handling
            QTextCursor tc(textCursor());
            int pos = tc.columnNumber();
            tc.select(QTextCursor::BlockUnderCursor);
            enum { doNothing, doTab, doBS, doDel } action = doNothing;

            int indLevel, indChars;
            countIndentation(tc.selectedText(), indLevel, indChars);

            // TODO: shift-tab is not same as del, add separate case for shift-tab (and maybe ctrl-shift-tab)

            if (event->key() == Qt::Key_Tab && !(event->modifiers() & Qt::ShiftModifier)) {
                action = doTab;
            }
            else if (event->key() == Qt::Key_Backspace ) {
                action = doBS;
            }
            else if (event->key() == Qt::Key_Delete
                     || (event->key() == Qt::Key_Tab && (event->modifiers() & Qt::ShiftModifier))) {
                action = doDel;
            }

            // qDebug() << FFL << action << (event->modifiers() & Qt::ShiftModifier);
            // qDebug() << FFL << pos << indChars << indLevel;

            if ((action==doTab && pos<=indChars) ||
                (action==doBS && indLevel>0 && pos>0 && pos<=indChars)  ||
                (action==doDel && indLevel>0 && pos<indChars)) {

                // select current indentation (if any)
                tc.movePosition(QTextCursor::StartOfLine);
                tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, indChars);
                // adujst indentation level
                indLevel += (action == doTab) ? +1 : -1;
                setTextCursor(tc);
                // replace selection with correct number of spaces
                insertAtTextCursor(QString(indentSize * indLevel, ' '));
                handled = true;
            }
        }
    } // endif  of tab handling

    if (!handled) {
        //qDebug() << FCFL << "not handled" << event->modifiers() << event->text();
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
        emit requestInteractiveEvaluation(MEC::replaceUnicodeSeparators(selText));
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
    if (ignoreCursorPosChanges) {
        //qDebug() << FCFL << "ignored";
        return;
    }

    //qDebug() << FCFL << textCursor().position() << completionType;
    if (completionType != NO_COMPLETION) {
        Q_ASSERT(!complCur.isNull());

        QTextCursor cur(textCursor());

        if (cur.position() < complCur.selectionStart() || cur.position() > complCur.selectionEnd() ) {
            qDebug() << FCFL << "cancelCompletion";
            cancelCompletion();
            updateHighlights();
        }
        else {
            cur.setPosition(complCur.selectionEnd());
            cur.clearSelection();
            setTextCursor(cur);
        }
    }
    else {
        int block = textCursor().block().blockNumber();
        if (block != lastBlock) {
            updateHighlights();
            lastBlock = block;
        }
    }
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
    if (!find(findText, options)) {
        // wrap search
        QTextCursor cur(textCursor());
        bool back = (options & QTextDocument::FindBackward);
        moveCursor(back ? QTextCursor::End : QTextCursor::Start);
        if (!find(findText, options)) {
            // not found even after warp
            setTextCursor(cur);
            // TODO: statusbar: not found
            return false;
        }
        // else TODO: statusbar: search wrapped
    }
    return true;
}


bool TDriverCodeTextEdit::doReplaceFind(QString findText, QString replaceText, QTextDocument::FindFlags options)
{
    if (!isReadOnly() &&
        textCursor().hasSelection() &&
        0 == QString::compare(textCursor().selectedText(),
                              findText,
                              (options & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
        insertAtTextCursor(replaceText);
    }
    return doFind(findText, options);
}


void TDriverCodeTextEdit:: doReplaceAll(QString findText, QString replaceText, QTextDocument::FindFlags options)
{
    if (isReadOnly()) return;

    while(doReplaceFind(findText, replaceText, options));
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
    }
    else {
        if (!onlySetModes) {
            fname = MEC::absoluteFilePath(fn);
            setObjectName("editor edit " + fn);
        }
        if (!isRubyMode && (fn.endsWith(".rb") || fn.endsWith(".rb.template"))) {
            setRubyMode(true);
            setUsingTabulatorsMode(false);
            setWrapMode(false);
            emit modesChanged();
        }
    }
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

    if (needReHighlight) doSyntaxHighlight();

    QList<QTextEdit::ExtraSelection> extraSelections;

    highlightCursorLine(extraSelections);

    if (isRunning) {
        highlightBreakpointLines(extraSelections);
        highlightRunningLine(extraSelections);
    }

    highlightCompletionCursors(extraSelections);

    setExtraSelections(extraSelections);
}


void TDriverCodeTextEdit::doSyntaxHighlight()
{
    if (highlighter) {
        highlighter->rehighlight();
        // TODO: if completion selection becomes sluggish, do highlight only to visible blocks,
        // see sideAreaPaintEvent for example
        needReHighlight = false;
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
    if (count > 1) qWarning() << FFL << "Removed multiple breakpoints with same rdebug breakpoint index" << rdebugInd;
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
        MEC::Breakpoint bp = { num:0, enabled:true, file:fileName(), line:lineNum };
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


void TDriverCodeTextEdit::setRunningLine(int lineNum)
{
    qDebug() << FFL << fileName() << lineNum << "running:" << isRunning;

    if (lineNum > 0) {
        QTextCursor tc = textCursor();
        tc.movePosition(QTextCursor::Start);
        tc.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNum-1);
        setTextCursor(tc);
        centerCursor();
    }

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

