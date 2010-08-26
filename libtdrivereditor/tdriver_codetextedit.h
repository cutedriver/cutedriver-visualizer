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


#ifndef CODETEXTEDIT_H
#define CODETEXTEDIT_H

#include "tdriver_editor_common.h"
#include "tdriver_highlighter.h"

#include <QPlainTextEdit>

#include <QColor>
#include <QList>
#include <QSet>
#include <QTextCursor>
#include <QStringList>
#include <QTextDocument>
#include <QPair>

class QAbstractItemModel;
class QModelIndex;
class QCompleter;
class QFont;
// contains line numbers, breakpoints, etc.
class SideArea;
//class QMenu;
class TDriverCompletionMenu;

class TDriverCodeTextEdit : public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
    Q_PROPERTY(int noNameId READ getNoNameId)
    Q_PROPERTY(int noNameIdCounter READ getNoNameIdCounter)

    friend class TDriverCompletionMenu;

    static const struct ConfigStruct {
        QSet<int> handledCancelKeys; // keys that are supposed to reject choice and close popup, and mark event handled
        QSet<int> unhandledCancelKeys; // keys that are supposed to reject choice and close popup, but not mark event handled
        QSet<int> complAcceptKeys; // keys that are supposed to accept choice and close popup without being inserted

        ConfigStruct(void);
    } Config;

public:
    explicit TDriverCodeTextEdit(QWidget *parent = 0);

    void sideAreaPaintEvent(QPaintEvent *);
    int sideAreaWidth();
    void sideAreaMouseReleaseEvent(QMouseEvent *event);

    void setHighlighter(TDriverHighlighter *);

    const QString &fileName() const { return fname; }
    void setFileName(QString fn, bool onlySetModes=false); // returns true if mode changes happend

    bool useTabulatorsMode() const { return isUsingTabulatorsMode; }
    bool rubyMode() const { return isRubyMode; }
    bool wrapMode() const { return isWrapMode; }
    int getNoNameId() const { return noNameId; }
    static int getNoNameIdCounter();

signals:
    void modesChanged();
    void addedBreakpoint(struct MEC::Breakpoint);
    void removedBreakpoint(int rdebugInd);
    void requestInteractiveCompletion(QByteArray statement);
    void requestInteractiveEvaluation(QByteArray statement);

public slots:
    void setUsingTabulatorsMode(bool enabled);
    void setRubyMode(bool enabled);
    void setWrapMode(bool enabled);
    void madeCurrent();

    bool doFind(QString findText, QTextDocument::FindFlags options = 0);
    bool doReplaceFind(QString findText, QString replaceText, QTextDocument::FindFlags options = 0);
    void doReplaceAll(QString findText, QString replaceText, QTextDocument::FindFlags options = 0);

    void commentCode();

    void setRunning(bool state);
    void focusInEvent(QFocusEvent *);
    void doAutoIndent(QKeyEvent *); // helper for keyPressEvent

    bool setTranslationDatabase(const QMap<QString,QString> &params, const QSettings *settings);
    void startTranslationCompletion(QKeyEvent *);
    bool doTranslationCompletion(QKeyEvent *);

    QString splitToComplCursors(QTextCursor selection);
    QTextCursor joinComplCursors();
    bool autoInteractiveCompletion();
    void tryInteractiveCompletion(QKeyEvent *);
    bool doInteractiveCompletion(QKeyEvent *);
    void popupInteractiveCompletion(QObject *client, QByteArray statement=QByteArray(), QStringList completions=QStringList());
    void errorInteractiveCompletion(QObject *client, QByteArray statement, QStringList completions);

    //void errorInteractiveEvaluation() { implement when needed }

    bool doGenericCompletion(QKeyEvent *event, bool &handled);

    void keyPressEvent(QKeyEvent *);

    void contextMenuEvent(QContextMenuEvent *);

    void cursorPositionChange();

    // Valid line numbers are 1 .. document.blockCount
    void dataSyncRequest();
    void setRunningLine(int lineNum);
    void clearBreakpoints();
    void rdebugBreakpointReset();
    void rdebugBreakpoint(struct MEC::Breakpoint bp);
    void removeBreakpointLine(int lineNum);
    void rdebugDelBreakpoint(int rdebugInd);
    void uiToggleBreakpointLine(int lineNum);

    void updateHighlights();


protected:
    void resizeEvent(QResizeEvent *event);

private:
    static int noNameIdCounter;
    const int noNameId;
    QWidget *sideArea;
    TDriverHighlighter *highlighter;
    bool needReHighlight;
    QCompleter *completer;
    bool complPopupShowingInfo;
    QStandardItemModel *phraseModel;
    int baseStart; // position before complCur.selectionStart, indicating the base text for which completions are searched
    QString lastBaseText;
    QTextCursor complCur;
    int stackHighlightStart;
    QList<int> complStartStack;
    QList<char> complCharStack;

    QString translationDBHost;
    QString translationDBName;
    QString translationDBTable;
    QString translationDBUser;
    QString translationDBPassword;
    QStringList translationDBLanguages;
    bool translationDBconfigured;
    QStringList translationDBerrors;

    QString fname;
    int lastBlock; // used for avoiding unnecessary calls to updateHighlights
    int lastBlockCount;
    bool isRunning;

    bool isUsingTabulatorsMode;
    bool isRubyMode;
    bool isWrapMode;

    // Note: valid line numbers are 1 .. document.blockCount
    int runningLine;
    QSet<int> rdebugBpSet;
    QList <struct MEC::Breakpoint> bpList;
    QColor cursorLineColor;
    QColor completionStackedColor;
    QColor completionBaseColor;
    QColor completionSelectColor;
    QColor runningLineColor;
    QColor breakpointLineColor;

    //QMenu *completionPopup;
    enum { NO_COMPLETION, TRANSLATION_COMPLETION, BASIC_COMPLETION } completionType;
    int popupOrigLen;
    bool popupFirstInsert;
    bool popupWasTriggered;

    QAction *contextEval;
    bool ignoreCursorPosChanges;

private slots:
    void doInteractiveEval();

    void popupCompleterInfo(const QString &text);
    void popupBasicCompleter(QAbstractItemModel *model);

    void documentBlockCountChange(int newCount);
    void updateSideAreaWidth();

    // highlight slots are usually not connect, but called by handleCursorPositionChange
    void doSyntaxHighlight();
    void highlightCursorLine(QList<QTextEdit::ExtraSelection> &extraSelections);
    void highlightCompletionCursors(QList<QTextEdit::ExtraSelection> &extraSelections);
    void highlightRunningLine(QList<QTextEdit::ExtraSelection> &extraSelections);
    void highlightBreakpointLines(QList<QTextEdit::ExtraSelection> &extraSelections);

    void updateSideArea(const QRect &, int);
    void cancelCompletion(bool adjustTextCursor = false);
    bool completerActivated();
    void completerActivated(const QModelIndex &);

private:
    bool forwardSearch(int targetLine, int &ind);
    bool binarySearch(int targetLine, int &ind);
    void insertAtComplCursor(QString text) {
        ignoreCursorPosChanges = true;
        complCur.insertText(text);
        ignoreCursorPosChanges = false;
    }
    void insertAtTextCursor(QString text) {
        ignoreCursorPosChanges = true;
        textCursor().insertText(text);
        ignoreCursorPosChanges = false;
    }

};

class SideArea : public QWidget
{
public:
    SideArea(TDriverCodeTextEdit *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const {
        return QSize(codeEditor->sideAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) {
        codeEditor->sideAreaPaintEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) {
        codeEditor->sideAreaMouseReleaseEvent(event);
    }

private:
    TDriverCodeTextEdit *codeEditor;
};

#endif // CODETEXTEDIT_H
