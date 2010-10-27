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


#include "tdriver_tabbededitor.h"

#include "tdriver_rubyhighlighter.h"
#include "tdriver_codetextedit.h"
#include "tdriver_runconsole.h"
#include "tdriver_debugconsole.h"
#include "tdriver_rubyinteract.h"
#include "tdriver_editor_common.h"
#include "tdriver_editbar.h"
#include "tdriver_combolineedit.h"


#include <tdriver_executedialog.h>
#include <tdriver_util.h>
#include <tdriver_debug_macros.h>

#include <QApplication>
#include <QSettings>
#include <QIcon>
#include <QAction>
#include <QFile>
#include <QUrl>

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTextCodec>
#include <QByteArray>
#include <QMenuBar>
#include <QMenu>
#include <QDockWidget>
#include <QSet>
#include <QFont>
#include <QPushButton>
#include <QPalette>
#include <QStackedWidget>
#include <QShortcut>
#include <QLineEdit>

static inline QString strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName();
}


static inline void setEditorDefaultEncoding(TDriverCodeTextEdit *editor) {
    if (editor) {
        editor->setFileCodec(QTextCodec::codecForName("UTF-8"));
        editor->setFileCodecUtfBom(false);
    }
}


TDriverTabbedEditor::TDriverTabbedEditor(QWidget *shortcutParent, QWidget *parent) :
    QTabWidget(parent),
    editorFont(),
    proceedRunPending(false),
    editBarP(new TDriverEditBar(this)),
    rubyHighlighter(new TDriverRubyHighlighter()),
    plainHighlighter(new TDriverHighlighter()),
    runConsoleContainer(NULL),
    runConsole(NULL),
    runConsoleVisible(false),
    debugConsoleContainer(NULL),
    debugConsole(NULL),
    debugConsoleVisible(false),
    irConsoleContainer(NULL),
    irConsole(NULL),
    irConsoleVisible(false)
{
    // is there an easier way to set background color of empty QTabWidget?
    QStackedWidget *stack = findChild<QStackedWidget*>();
    if (stack) {
        QPalette pal = stack->palette();
        pal.setColor(QPalette::Window, Qt::lightGray);
        stack->setAutoFillBackground(true);
        stack->setPalette(pal);
    }

    setAcceptDrops(true);
    createActions();
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(currentChangeAction(int)));
    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    setTabsClosable(true);

    connect(editBarP, SIGNAL(requestUnfocus()), this, SLOT(focusCurrent()));

    // create some non-global shortcuts
    {
        if (!shortcutParent) shortcutParent = this;
        QShortcut *shortcut;

        shortcut = new QShortcut(QKeySequence("Ctrl+Tab"), shortcutParent, 0, 0, Qt::WidgetWithChildrenShortcut );
        connect(shortcut, SIGNAL(activated()), this, SLOT(nextTab()));

        shortcut = new QShortcut(QKeySequence("Ctrl+Shift+Tab"), shortcutParent, 0, 0, Qt::WidgetWithChildrenShortcut );
        connect(shortcut, SIGNAL(activated()), this, SLOT(prevTab()));

        shortcut = new QShortcut(QKeySequence("Ctrl+F"), shortcutParent, 0, 0, Qt::WidgetWithChildrenShortcut );
        connect(shortcut, SIGNAL(activated()), editBarP->findTextField(), SLOT(setFocus()));
        connect(shortcut, SIGNAL(activated()), editBarP->findTextField()->lineEdit(), SLOT(selectAll()));

        shortcut = new QShortcut(QKeySequence("F3"), shortcutParent, 0, 0, Qt::WidgetWithChildrenShortcut );
        connect(shortcut, SIGNAL(activated()), editBarP, SLOT(findNext()));

        shortcut = new QShortcut(QKeySequence("Shift+F3"), shortcutParent, 0, 0, Qt::WidgetWithChildrenShortcut );
        connect(shortcut, SIGNAL(activated()), editBarP, SLOT(findPrev()));
    }
}


void TDriverTabbedEditor::disconnectTabSignals()
{
    // disconnect signals from actions connecting to editor widgets in tabs
    disconnect(undoAct, SIGNAL(triggered()), 0, 0);
    disconnect(redoAct, SIGNAL(triggered()), 0, 0);
    disconnect(cutAct, SIGNAL(triggered()), 0, 0);
    disconnect(copyAct, SIGNAL(triggered()), 0, 0);
    disconnect(pasteAct, SIGNAL(triggered()), 0, 0);
    disconnect(selectAllAct, SIGNAL(triggered()), 0, 0);

    disconnect(commentCodeAct, SIGNAL(triggered()), 0, 0);

    disconnect(toggleUsingTabulatorsModeAct, SIGNAL(toggled(bool)), 0, 0);
    disconnect(toggleRubyModeAct, SIGNAL(toggled(bool)), 0, 0);
    disconnect(toggleWrapModeAct, SIGNAL(toggled(bool)), 0, 0);

    disconnect(editBarP, SIGNAL(requestFind(QString,QTextDocument::FindFlags)), 0, 0);
    disconnect(editBarP, SIGNAL(requestFindIncremental(QString,QTextDocument::FindFlags)), 0, 0);
    disconnect(editBarP, SIGNAL(requestReplaceFind(QString,QString,QTextDocument::FindFlags)), 0, 0);
    disconnect(editBarP, SIGNAL(requestReplaceAll(QString,QString,QTextDocument::FindFlags)), 0, 0);

    // disable and disconnect setEnabled for actions which are enabled/disabled by active editor widget
    QAction *disablelist[] = { undoAct, redoAct, cutAct, copyAct, pasteAct, NULL };
    for (int ind=0 ; disablelist[ind] != NULL; ++ind) {
        disconnect(disablelist[ind], SLOT(setEnabled(bool)));
        disablelist[ind]->setEnabled(false);
    }

}

void TDriverTabbedEditor::connectTabSignals(TDriverCodeTextEdit *editor)
{
    connect(undoAct, SIGNAL(triggered()), editor, SLOT(undo()));
    connect(redoAct, SIGNAL(triggered()), editor, SLOT(redo()));
    connect(cutAct, SIGNAL(triggered()), editor, SLOT(cut()));
    connect(copyAct, SIGNAL(triggered()), editor, SLOT(copy()));
    connect(pasteAct, SIGNAL(triggered()), editor, SLOT(paste()));
    connect(selectAllAct, SIGNAL(triggered()), editor, SLOT(selectAll()));

    connect(commentCodeAct, SIGNAL(triggered()), editor, SLOT(commentCode()));

    connect(editor, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
    connect(editor, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));

    connect(editBarP, SIGNAL(requestFind(QString,QTextDocument::FindFlags)),
            editor, SLOT(doFind(QString,QTextDocument::FindFlags)));
    connect(editBarP, SIGNAL(requestFindIncremental(QString,QTextDocument::FindFlags)),
            editor, SLOT(doIncrementalFind(QString,QTextDocument::FindFlags)));
    connect(editBarP, SIGNAL(requestReplaceFind(QString,QString,QTextDocument::FindFlags)),
            editor, SLOT(doReplaceFind(QString,QString,QTextDocument::FindFlags)));
    connect(editBarP, SIGNAL(requestReplaceAll(QString,QString,QTextDocument::FindFlags)),
            editor, SLOT(doReplaceAll(QString,QString,QTextDocument::FindFlags)));

    bool cAvail = editor->textCursor().hasSelection();
    copyAct->setEnabled(cAvail);
    cutAct->setEnabled(cAvail);

    // TODO: make pasteAct enabled when a document is open and clipboard has compatible data
    pasteAct->setEnabled(true);

    connect(editor, SIGNAL(undoAvailable(bool)), undoAct, SLOT(setEnabled(bool)));
    undoAct->setEnabled(editor->document()->isUndoAvailable());

    connect(editor, SIGNAL(redoAvailable(bool)), redoAct, SLOT(setEnabled(bool)));
    redoAct->setEnabled(editor->document()->isRedoAvailable());

    toggleUsingTabulatorsModeAct->setChecked(editor->useTabulatorsMode());
    connect(toggleUsingTabulatorsModeAct, SIGNAL(toggled(bool)), editor, SLOT(setUsingTabulatorsMode(bool)));

    toggleRubyModeAct->setChecked(editor->rubyMode());
    connect(toggleRubyModeAct, SIGNAL(toggled(bool)), editor, SLOT(setRubyMode(bool)));

    toggleWrapModeAct->setChecked(editor->wrapMode());
    connect(toggleWrapModeAct, SIGNAL(toggled(bool)), editor, SLOT(setWrapMode(bool)));
}

void TDriverTabbedEditor::currentChangeAction(int index)
{
    Q_UNUSED(index);
    //qDebug() << FFL << "Tab changed to" << index;
    disconnectTabSignals();
    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
    if (editor) {
        connectTabSignals(editor);
        editor->madeCurrent();
        emit documentNameChanged(editor->fileName());
    }
    else {
        qWarning("Warning! Unknown widget type in tab, some editor signals not connected.");
    }
}


void TDriverTabbedEditor::focusCurrent()
{
    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
    if (editor) {
        editor->setFocus();
    }
}


void TDriverTabbedEditor::createActions()
{
    // Actions are added to this editor widget,
    // so they can be fetched with actions() method,
    // so they can be easily added to menus and toolbars

    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setObjectName("editor new");
    newAct->setShortcuts(QKeySequence::New);
    newAct->setToolTip(tr("Create a new file"));
    fileActs.append(newAct);
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    newFromTemplateAct = new QAction(QIcon(":/images/new.png"), tr("New from &template"), this);
    newFromTemplateAct->setObjectName("editor newfromtemplate");
    newFromTemplateAct->setToolTip(tr("Create a newFromTemplate file"));
    fileActs.append(newFromTemplateAct);
    connect(newFromTemplateAct, SIGNAL(triggered()), this, SLOT(newFromTemplate()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setObjectName("editor open");
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setToolTip(tr("Open an existing file"));
    fileActs.append(openAct);
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setObjectName("editor save");
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setToolTip(tr("Save the document to disk"));
    fileActs.append(saveAct);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(saveCurrent()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setObjectName("editor saveas");
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setToolTip(tr("Save the document under a new name"));
    fileActs.append(saveAsAct);
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveCurrentAs()));

    saveAsTemplateAct = new QAction(tr("Save as T&emplate..."), this);
    saveAsTemplateAct->setObjectName("editor saveas");
    saveAsTemplateAct->setShortcuts(QKeySequence::SaveAs);
    saveAsTemplateAct->setToolTip(tr("Save the document under a new name"));
    fileActs.append(saveAsTemplateAct);
    connect(saveAsTemplateAct, SIGNAL(triggered()), this, SLOT(saveCurrentAsTemplate()));

    saveAllAct = new QAction(tr("Save A&ll"), this);
    saveAllAct->setObjectName("editor saveall");
    saveAllAct->setShortcut(QKeySequence("Ctrl+Shift+S"));
    saveAllAct->setToolTip(tr("Save the document under a new name"));
    fileActs.append(saveAllAct);
    connect(saveAllAct, SIGNAL(triggered()), this, SLOT(saveAll()));

    closeAct = new QAction(tr("C&lose file"), this);
    closeAct->setObjectName("editor close");
    closeAct->setShortcuts(QKeySequence::Close);
    closeAct->setToolTip(tr("Close the current file"));
    fileActs.append(closeAct);
    connect(closeAct, SIGNAL(triggered()), this, SLOT(closeCurrent()));

    for (int i = recentFileActs.size(); i < MaxRecentFiles; ++i) {
        recentFileActs.append(new QAction(this));
        recentFileActs.last()->setVisible(false);
        connect(recentFileActs.last(), SIGNAL(triggered()), this, SLOT(openRecentFile()));
    }

    // a tdriver_codetextedit action
    undoAct = new QAction(QIcon(":/images/undo.png"), tr("&Undo"), this);
    undoAct->setObjectName("editor undo");
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setToolTip(tr("Undo last edit"));
    editActs.append(undoAct);

    // a tdriver_codetextedit action
    redoAct = new QAction(QIcon(":/images/redo.png"), tr("&Redo"), this);
    redoAct->setObjectName("editor redo");
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setToolTip(tr("Redo last undoed edit"));
    editActs.append(redoAct);

    // a tdriver_codetextedit action
    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setObjectName("editor cut");
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setToolTip(tr("Cut the current selection's contents to the clipboard"));
    editActs.append(cutAct);

    // a tdriver_codetextedit action
    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setObjectName("editor copy");
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setToolTip(tr("Copy the current selection's contents to the clipboard"));
    editActs.append(copyAct);

    // a tdriver_codetextedit action
    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setObjectName("editor paste");
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setToolTip(tr("Paste the clipboard's contents into the current selection"));
    pasteAct->setEnabled(false);
    editActs.append(pasteAct);

    // a tdriver_codetextedit action
    selectAllAct = new QAction(QIcon(":/images/selectall.png"), tr("&Select All"), this);
    selectAllAct->setObjectName("editor selectall");
    selectAllAct->setShortcuts(QKeySequence::SelectAll);
    selectAllAct->setToolTip(tr("Select all text of current file"));
    editActs.append(selectAllAct);

    // a tdriver_codetextedit action
    commentCodeAct = new QAction(tr("&Comment/Uncomment region/line"), this);
    commentCodeAct->setObjectName("editor commentcode");
    commentCodeAct->setShortcut(QKeySequence(tr("Ctrl+/")));
    commentCodeAct->setToolTip(tr("Comments or uncomments selected code or current line"));
    codeActs.append(commentCodeAct);


    // a tdriver_codetextedit action
    toggleUsingTabulatorsModeAct = new QAction(tr("&Use TAB characters"), this);
    toggleUsingTabulatorsModeAct->setObjectName("editor toggle usetabs");
    toggleUsingTabulatorsModeAct->setToolTip(tr("Toggle converting tabs to spaces and using spaces as indentation"));
    toggleUsingTabulatorsModeAct->setCheckable(true);
    toggleUsingTabulatorsModeAct->setEnabled(true);
    optActs.append(toggleUsingTabulatorsModeAct);

    // a tdriver_codetextedit action
    toggleRubyModeAct = new QAction(tr("&Ruby mode"), this);
    toggleRubyModeAct->setObjectName("editor toggle rubymode");
    toggleRubyModeAct->setToolTip(tr("Enable Ruby specific features"));
    toggleRubyModeAct->setCheckable(true);
    toggleRubyModeAct->setEnabled(true);
    optActs.append(toggleRubyModeAct);

    // a tdriver_codetextedit action
    toggleWrapModeAct = new QAction(tr("&Wrap mode"), this);
    toggleWrapModeAct->setObjectName("editor toggle wrapmode");
    toggleWrapModeAct->setToolTip(tr("Enable Wrap specific features"));
    toggleWrapModeAct->setCheckable(true);
    toggleWrapModeAct->setEnabled(true);
    optActs.append(toggleWrapModeAct);

    {
        QAction *tmp = new QAction(this);
        tmp->setSeparator(true);
        optActs.append(tmp);
    }

    toggleRunDockAct = new QAction(tr("Run Console visible"), this);
    toggleRunDockAct->setObjectName("editor toggle scriptdock");
    toggleRunDockAct->setToolTip(tr("Visibility of Script Run Console Dock"));
    toggleRunDockAct->setCheckable(true);
    toggleRunDockAct->setEnabled(true);
    optActs.append(toggleRunDockAct);

    toggleDebugDockAct = new QAction(tr("Debug Console visible"), this);
    toggleDebugDockAct->setObjectName("editor toggle debugdock");
    toggleDebugDockAct->setToolTip(tr("Visibility of Debugger Console Dock"));
    toggleDebugDockAct->setCheckable(true);
    toggleDebugDockAct->setEnabled(true);
    optActs.append(toggleDebugDockAct);

    toggleIrDockAct = new QAction(tr("RubyInteract Console visible"), this);
    toggleIrDockAct->setObjectName("editor toggle irdock");
    toggleIrDockAct->setToolTip(tr("Visibility of Ruby Interaction Console Dock"));
    toggleIrDockAct->setCheckable(true);
    toggleIrDockAct->setEnabled(true);
    optActs.append(toggleIrDockAct);

    runAct = new QAction(QIcon(":/images/run.png"), tr("&Run with ruby"), this);
    runAct->setObjectName("editor run ruby");
    runAct->setShortcut(QKeySequence(Qt::Key_F9));
    runAct->setToolTip(tr("Save all and run current file with ruby, close debugger"));
    runActs.append(runAct);
    connect(runAct, SIGNAL(triggered()), this, SLOT(run()));

#if TDRIVER_RUNCONSOLE_DEBUG1_ENABLED
    debug1Act = new QAction(QIcon(":/images/debug1.png"), tr("&Run with rdebug"), this);
    debug1Act->setObjectName("editor run rdebug");
    debug1Act->setToolTip(tr("Save all and run current file with rdebug, show debugger, no editor interaction"));
    runActs.append(debug1Act);
    connect(debug1Act, SIGNAL(triggered()), this, SLOT(debug1()));
    //debug1Act->setEnabled(false);
    //debug1Act->setVisible(false);
#endif

    debug2Act = new QAction(QIcon(":/images/debug2.png"), tr("&Run with integrated debugger"), this);
    debug2Act->setObjectName("editor run integrated");
    debug2Act->setShortcut(QKeySequence(Qt::Key_F5)); // see debugConsole continueAct
    debug2Act->setToolTip(tr("Save all and run current file with editor-integrated debugger"));
    runActs.append(debug2Act);
    connect(debug2Act, SIGNAL(triggered()), this, SLOT(debug2()));
    debug2Act->setEnabled(true);

    syntaxCheckAct = new QAction(tr("&Check syntax"), this);
    syntaxCheckAct->setObjectName("editor run syntaxcheck");
    syntaxCheckAct->setToolTip(tr("Save all and run current file through 'ruby -cw'"));
    runActs.append(syntaxCheckAct);
    connect(syntaxCheckAct, SIGNAL(triggered()), this, SLOT(syntaxCheck()));
    syntaxCheckAct->setEnabled(true);

    // make actions() to return a flat list of all actions
    addActions(fileActs);
    addActions(editActs);
    addActions(codeActs);
    addActions(optActs);
    addActions(runActs);
}


static inline void makeDockVisible(QObject *parent)
{
    QDockWidget *dock = NULL;

    while (parent && !dock) {
        dock = qobject_cast<QDockWidget*>(parent);
        parent = parent->parent();
    }
    if (dock) {
        dock->show();
    }
}


bool TDriverTabbedEditor::open(void)
{
    makeDockVisible(parent());

    QString dir;
    TDriverCodeTextEdit *w = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
    if (w && !w->fileName().isEmpty()) {
        dir = MEC::absoluteFilePath(w->fileName());
        //qDebug() << FFL << "GOT FROM CURRENT TAB" << dirName;
    }
    if (dir.isEmpty()) {
        dir = MEC::settings->value("editor/defaultdir").toString(); // empty default ok
        //qDebug() << FFL << "GOT FROM CONFIG" << dirName;
    }
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open..."), dir);
    return loadFile(fileName);
}


bool TDriverTabbedEditor::saveCurrent(void)
{
    return saveTab(currentIndex());
}

bool TDriverTabbedEditor::saveTab(int index)
{
    // TODO: if sender is tab context menu, currentWidget isn't correct tab!
    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(index));

    if (!editor) return true; // consider file unsaveable and situation therefore "ok"

    if (editor->fileName().isEmpty()) {
        qDebug() << FFL << "no filename";
        return saveTabAs(index, tr("Save unnamed file as UTF-8 w/o BOM..."));
    }
    else {
        qDebug() << FFL << editor->fileName();
        return saveFile(editor->fileName(), index, false);
    }
}


bool TDriverTabbedEditor::saveCurrentAs(void)
{
    return saveTabAs(currentIndex(), tr("Save with new name as UTF-8 w/o BOM..."));
}

bool TDriverTabbedEditor::saveCurrentAsTemplate(void)
{
    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
    if (!editor) return false;

    // remember original name in case saving is canceled
    QString origName(editor->fileName());

    // create templte file name
    QString dir(TDriverUtil::tdriverHelperFilePath("templates/"));
    QString newName = MEC::pathReplaced(origName, dir);
    if (!newName.endsWith(".template")) newName.append(".template");
    editor->setFileName(newName);

    if (saveTabAs(currentIndex(),
                  tr("Save as Template..."),
                  tr("Templates (*.template);;Ruby Templates (*.rb.template)"))) {
        return true;
    }
    else {
        editor->setFileName(origName);
        return false;
    }
}

bool TDriverTabbedEditor::saveTabAs(int index, const QString &caption, const QString &filter)
{
    setCurrentIndex(index);
    const TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(index));
    if (!editor) return false;

    QString dirName;
    if (editor && !editor->fileName().isEmpty()) {
        dirName = MEC::absoluteFilePath(editor->fileName());
    }
    else {
        dirName = MEC::settings->value("editor/defaultdir").toString(); // empty default ok
    }

    QString fileName = QFileDialog::getSaveFileName(this, caption, dirName, filter);
    if (fileName.isEmpty()) {
        qDebug("%s: canceled", __FUNCTION__);
        return false;
    }
    else {
        qDebug("%s: got filename '%s'", __FUNCTION__, qPrintable(fileName));
        return saveFile(fileName, index, true); // ignore return code
    }
}

bool TDriverTabbedEditor::saveAll(void)
{
    for(int index=0 ; index < count(); ++index) {
        const TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(index));
        if (!editor) continue; // ignore tab that is not TDriverCodeTextEdit
        if (!editor->document()->isModified()) continue; // ignore tab that is not modified
        if (!saveTab(index)) {
            // make failed tab current, since user probably wants to do something with it
            setCurrentIndex(index);
            return false;
        }
    }
    return true;
}


bool TDriverTabbedEditor::closeCurrent(void)
{
    return closeTab(currentIndex());
}


void TDriverTabbedEditor::nextTab()
{
    int index = currentIndex();
    if (index >= 0) {
        if (++index >= count()) index = 0;
        setCurrentIndex(index);
        currentWidget()->setFocus(Qt::OtherFocusReason);
    }
}


void TDriverTabbedEditor::prevTab()
{
    int index = currentIndex();
    if (index >= 0) {
        if (--index < 0 ) index = count() - 1;
        setCurrentIndex(index);
        currentWidget()->setFocus(Qt::OtherFocusReason);
    }
}


bool TDriverTabbedEditor::closeTab(int index)
{
    TDriverCodeTextEdit *w = qobject_cast<TDriverCodeTextEdit*>(widget(index));
    if (w) {
        if (w->document()->isModified()) {
            QMessageBox msgBox;
            msgBox.setText(tr("File modified"));
            msgBox.setInformativeText(tr("File has unsaved modifications."));
            msgBox.setStandardButtons(QMessageBox::Save |QMessageBox::Discard | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Save);
            int selection = msgBox.exec();
            // w will be set to NULL in switch if close shouldn't happen
            switch(selection) {

            case QMessageBox::Discard:
                break;

            case QMessageBox::Save:
                if (saveCurrent()) break;
                // if save fails, fall through to Cancel

            case QMessageBox::Cancel:
                w=NULL;
                break;
            }
        }
        if (w) {
            qDebug() << FFL << "removing index" << index;
            Q_ASSERT(widget(index) == w);
            removeTab(index);
            qDebug() << FFL << "deleting" << w;
            delete w;
            return true;
        }
    }
    return false;
}

bool TDriverTabbedEditor::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        qDebug("Opening recent file '%s'", qPrintable(action->data().toString()));
        return loadFile(action->data().toString());
    }
    else {
        return false;
    }
}

static inline void setEditorFontAndTabWidth(TDriverCodeTextEdit *editor, const QFont &font)
{
    editor->setFont(font);
    int tabPixels = editor->fontMetrics().width(' '); // line number
    editor->setTabStopWidth(MEC::settings->value("editor/tabspaces", 8).toInt() * tabPixels);
}


void TDriverTabbedEditor::newFile(QString fileName)
{
    makeDockVisible(parent());

    TDriverCodeTextEdit *newEdit = new TDriverCodeTextEdit(this);
    newEdit->setTranslationDatabase(paramMap, MEC::settings);
    setEditorFontAndTabWidth(newEdit, editorFont);
    newEdit->setFileName(fileName);
    //setEditorDefaultEncoding(newEdit);

    connect(newEdit->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(documentModification(bool)));
    connect(newEdit, SIGNAL(modesChanged()),
            this, SLOT(editorModeChange()));

    connect(newEdit, SIGNAL(addedBreakpoint(MEC::Breakpoint)),
            this, SIGNAL(addedBreakpoint(MEC::Breakpoint)));
    connect(newEdit, SIGNAL(removedBreakpoint(int)),
            this, SIGNAL(removedBreakpoint(int)));

    connect(newEdit, SIGNAL(requestInteractiveCompletion(QByteArray)),
            irConsole, SLOT(queryCompletions(QByteArray)));
    connect(irConsole, SIGNAL(completionResult(QObject*,QByteArray,QStringList)),
            newEdit, SLOT(popupInteractiveCompletion(QObject*,QByteArray,QStringList)));
    connect(irConsole, SIGNAL(completionError(QObject*,QByteArray,QStringList)),
            newEdit, SLOT(errorInteractiveCompletion(QObject*,QByteArray,QStringList)));

    connect(newEdit, SIGNAL(requestInteractiveEvaluation(QByteArray)),
            irConsole, SLOT(evalStatement(QByteArray)));
    connect(newEdit, SIGNAL(requestInteractiveEvaluation(QByteArray)),
            this, SLOT(showIrConsole()));
    // add when needed: connect(irConsole, SIGNAL(evaluationError(QObject*,QByteArray,QStringList)), newEdit, SLOT(errorInteractiveEvaluation(QObject*,QByteArray,QStringList)));

    if (runConsole) {
        connect(runConsole, SIGNAL(runningState(bool)), newEdit, SLOT(setRunning(bool)));
        newEdit->setRunning(runConsole->isRunning());
    }
    else qWarning() << FFL << "new TDriverCodeTextEdit created before runConsole was set!";

    int index = addTab(newEdit, QString());
    newEdit->setHighlighter(plainHighlighter);
    updateTab(index);
    setCurrentIndex(index);
    currentWidget()->setFocus();
    // Post-condition, checking for both type and identity.
    // If this is changed, check every place where newFile is called.
    Q_ASSERT(newEdit == qobject_cast<TDriverCodeTextEdit*>(currentWidget()));
}


bool TDriverTabbedEditor::newFromTemplate(void)
{
    QString dir(TDriverUtil::tdriverHelperFilePath("templates/"));
    QString fileName(QFileDialog::getOpenFileName(this, tr("Select Template..."), dir));
    return loadFile(fileName, true);
}


bool TDriverTabbedEditor::run()
{
    // TODO: support for different file types to be run differently
    // TODO: add feature to set what to run, instead of current tab

    TDriverCodeTextEdit *editor = prepareExtAction();
    if (!editor) return false;

    emit requestRun(editor->fileName(), TDriverRunConsole::RunRequest);
    setRunConsoleVisible(true);
    setDebugConsoleVisible(false);
    return true;
}

#if TDRIVER_RUNCONSOLE_DEBUG1_ENABLED
bool TDriverTabbedEditor::debug1()
{
    // TODO: same as run()
    TDriverCodeTextEdit *editor = prepareExtAction();
    if (!editor) return false;

    emit requestRun(editor->fileName(), TDriverRunConsole::Debug1Request);
    setRunConsoleVisible(true);
    setDebugConsoleVisible(false);
    return true;
}
#endif


bool TDriverTabbedEditor::debug2()
{
    // TODO: same as run()
    TDriverCodeTextEdit *editor = prepareExtAction();
    if (!editor) return false;

    emit requestRun(editor->fileName(), TDriverRunConsole::Debug2Request);
    setRunConsoleVisible(true);
    setDebugConsoleVisible(true);
    return true;
}


bool TDriverTabbedEditor::syntaxCheck()
{
    TDriverCodeTextEdit *editor = prepareExtAction();
    if (!editor) return false;

    QStringList args;
    args << "-cw" << editor->fileName();
    if (!execDialog.isNull() && execDialog.data()->autoClose()) execDialog->close();

    execDialog = new TDriverExecuteDialog::TDriverExecuteDialog("ruby", args, this);
    execDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(execDialog, SIGNAL(anchorClicked(QUrl)), this, SLOT(gotoLine(QUrl)));
    execDialog->show();

    return false;
}


TDriverCodeTextEdit *TDriverTabbedEditor::prepareExtAction()
{
    // TODO: if sender is tab context menu, currentWidget isn't correct tab!
    // TODO: call saveAll() instead of save()

    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());

    if (editor && !queryUnsavedFate(BeforeRun)) return NULL;

    if (editor && editor->fileName().isEmpty()) {
        QMessageBox::information(this, tr("No file"), tr("This action needs a file"));
        qDebug() << FFL << "no file name";
        editor=NULL;
    }

    return editor;
}


void TDriverTabbedEditor::connectConsoles(TDriverRunConsole *rConsole, QWidget *rContainer,
                                          TDriverDebugConsole *dConsole, QWidget *dContainer,
                                          TDriverRubyInteract *iConsole, QWidget *iContainer)
{
    runConsoleContainer = rContainer;
    runConsole = rConsole;
    debugConsoleContainer = dContainer;
    debugConsole = dConsole;

    irConsole = iConsole;
    irConsoleContainer = iContainer;

    connect(rConsole, SIGNAL(needDebugConsole(bool)), this, SLOT(setDebugConsoleVisible(bool)));
    connect(rConsole, SIGNAL(needDebugConsole(bool)), dConsole, SLOT(clear()));

    connect(this, SIGNAL(requestRun(QString,TDriverRunConsole::RunRequestType)),
            this, SLOT(runFilePrep(QString,TDriverRunConsole::RunRequestType)));

    connect(rConsole, SIGNAL(requestRemoteDebug(QString,quint16,quint16, TDriverRunConsole*)),
            dConsole, SLOT(connectTo(QString,quint16,quint16, TDriverRunConsole*)));

    connect(rConsole, SIGNAL(runSignal(bool)), runAct, SLOT(setDisabled(bool)));
#if TDRIVER_RUNCONSOLE_DEBUG1_ENABLED
    connect(rConsole, SIGNAL(runSignal(bool)), debug1Act, SLOT(setDisabled(bool)));
#endif
    connect(rConsole, SIGNAL(runSignal(bool)), debug2Act, SLOT(setDisabled(bool)));

    connect(rConsole, SIGNAL(runningState(bool)), dConsole, SLOT(setRunning(bool)));
    connect(rConsole, SIGNAL(runSignal(bool)), this, SLOT(resetRunningLines()));
    //connect(dConsole, SIGNAL(runStarting(QString,int)), this, SLOT(resetRunningLines())); // hide running line marker while script is executing
    connect(dConsole, SIGNAL(breakpoint(MEC::Breakpoint)), this, SLOT(addBreakpoint(MEC::Breakpoint)));
    connect(dConsole, SIGNAL(breakpoints(QList<MEC::Breakpoint>)), this, SLOT(addBreakpointList(QList<MEC::Breakpoint>)));
    connect(dConsole, SIGNAL(delegateContinueAction()), debug2Act, SLOT(trigger()));

    connect(this, SIGNAL(dataSyncDone()), dConsole, SLOT(allSynced()));
    connect(dConsole, SIGNAL(requestDataSync()), this, SLOT(syncTabsData()));
    connect(dConsole, SIGNAL(gotRemotePrompt(QString,int)), this, SLOT(setRunningLine(QString,int)));

    connect(this, SIGNAL(addedBreakpoint(MEC::Breakpoint)), dConsole, SLOT(addBreakpoint(MEC::Breakpoint)));
    connect(this, SIGNAL(removedBreakpoint(int)), dConsole, SLOT(removeBreakpoint(int)));

    connect(toggleRunDockAct, SIGNAL(toggled(bool)), runConsoleContainer, SLOT(setVisible(bool)));
    connect(toggleDebugDockAct, SIGNAL(toggled(bool)), debugConsoleContainer, SLOT(setVisible(bool)));
    connect(toggleIrDockAct, SIGNAL(toggled(bool)), irConsoleContainer, SLOT(setVisible(bool)));

    runConsoleContainer->setVisible(runConsoleVisible);
    debugConsoleContainer->setVisible(debugConsoleVisible);
    irConsoleContainer->setVisible(irConsoleVisible);

}


void TDriverTabbedEditor::documentModification(bool)
{
    // TODO: find out which tab sent the signal, and do updateTab on correct tab
    // this one will fail to update correctly if non-active tab becomes modified
    // note: first check if any features which can do this have been implemented :-)
    updateTab();
}


void TDriverTabbedEditor::editorModeChange()
{
    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(sender());
    if (editor) {
        //qDebug() << FFL << editor->fileName() << "rubyMode" << editor->rubyMode() << "useTabulatorsMode" << editor->useTabulatorsMode();
        editor->setHighlighter(editor->rubyMode() ? rubyHighlighter : plainHighlighter);

        if (currentWidget() == editor) {
            toggleUsingTabulatorsModeAct->setChecked(editor->useTabulatorsMode());
            toggleRubyModeAct->setChecked(editor->rubyMode());
            toggleWrapModeAct->setChecked(editor->wrapMode());
            editor->madeCurrent();
        }
    }
}


void TDriverTabbedEditor::setRunConsoleVisible(bool visibility)
{
    // TODO: possibly make it so that if runConsole is not visible, neither is debugConsole
    runConsoleVisible = visibility;
    if (runConsoleContainer) {
        runConsoleContainer->setVisible(isVisible() && visibility);
    }
}

void TDriverTabbedEditor::setDebugConsoleVisible(bool visibility)
{
    // TODO: possibly make it so that if runConsole is not visible, neither is debugConsole
    debugConsoleVisible = visibility;
    if (debugConsoleContainer) {
        debugConsoleContainer->setVisible(visibility && isVisible());
    }
}

void TDriverTabbedEditor::setIRConsoleVisible(bool visibility)
{
    // TODO: possibly make it so that if runConsole is not visible, neither is debugConsole
    irConsoleVisible = visibility;
    if (irConsoleContainer) {
        irConsoleContainer->setVisible(visibility && isVisible());
    }
}

void TDriverTabbedEditor::showIrConsole()
{
    setIRConsoleVisible(true);
}


bool TDriverTabbedEditor::loadFile(QString fileName, bool fromTemplate)
{
    qDebug() << FFL << fileName;
    bool ret = false;
    QFile file(fileName);

    if (fileName.isEmpty()) {
        // ignore request to open empty file
    }

    else if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("TDriver Editor"),
                             tr("Can't read file '%1':\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
    }

    else {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());

        if (editor && editor->document()->isEmpty() && editor->fileName().isEmpty()) {
            // just reset modified flag of existing tab (empty, unnamed document)
            editor->document()->setModified(false);
        }
        else {
            // create new tab for document to be loaded
            newFile();
            editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
            Q_ASSERT(editor);
        }

        QString stringData;
        QByteArray byteData(file.readAll());
        bool haveBom = false;
        bool encodingTestedOk;
        QTextCodec *codec = QTextCodec::codecForUtfText(byteData, NULL);
        if (codec == NULL) {
            qDebug() << FCFL << "Unicode codec autodetection failed, trying UTF-8";
            codec = QTextCodec::codecForName("UTF-8");
            stringData = codec->toUnicode(byteData);
            encodingTestedOk = (codec->fromUnicode(stringData) == byteData);
            if (!encodingTestedOk) {
                qDebug() << FCFL << "Encoding back to UTF-8 produced different result, trying codecForLocale";
                codec = QTextCodec::codecForLocale();
                stringData = codec->toUnicode(byteData);
                encodingTestedOk = (codec->fromUnicode(stringData) == byteData);
            }
        }
        else {
            stringData = codec->toUnicode(byteData);
            haveBom = true;
            QByteArray testBuf;
            {
                QTextStream testStream(&testBuf);
                testStream.setCodec(codec);
                testStream.setGenerateByteOrderMark(haveBom);
                testStream << stringData;
            }
            encodingTestedOk = (testBuf == byteData);
        }
        editor->setFileCodec(codec);
        editor->setFileCodecUtfBom(haveBom);
        editor->setPlainText(stringData);
        editor->setFileName(fileName, fromTemplate);
        qDebug() << FCFL
                 << "editor set to file" << editor->fileName()
                 << "codec" << editor->fileCodec()->name();

        if (!fromTemplate) {
            emit documentNameChanged(editor->fileName());
            recentFileUpdate(fileName);
        }

        updateTab();

        //statusBar()->showMessage(tr("File loaded"), 2000);
        QApplication::restoreOverrideCursor();
        if (!encodingTestedOk) {
            QMessageBox::warning(this, tr("TDriver Editor"),
                                 tr("Decoded file '%1' using codec '%2'%3,\n"
                                    "but encoding it when saving will NOT produce identical file!.")
                                 .arg(fileName)
                                 .arg(QString::fromLocal8Bit(editor->fileCodec()->name()))
                                 .arg(haveBom ? tr(" with UTF BOM") : tr(" without UTF BOM")));
        }
        ret = true;
    }

    return ret;
}


bool TDriverTabbedEditor::saveFile(QString fileName, int index, bool resetEncoding)
{
    if (index < 0) index = currentIndex();

    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(index));

    if (!editor) return false;

    if (resetEncoding) setEditorDefaultEncoding(editor);

    if (editor->fileName() != fileName) {
        // name is changed and document marked modified even if saving will fail
        editor->setFileName(fileName);
        if (editor == currentWidget()) emit documentNameChanged(editor->fileName());

        editor->document()->setModified(true);
        updateTab(index);

    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Recent Files"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        updateTab(index);

        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    {
        QTextStream outStream(&file);
        if (editor->fileCodec()) outStream.setCodec(editor->fileCodec());
        outStream.setGenerateByteOrderMark(editor->fileCodecUtfBom());
        outStream << editor->toPlainText();
    }
    file.close();
    editor->document()->setModified(false);
    QApplication::restoreOverrideCursor();

    updateTab(index);

    recentFileUpdate(fileName);

    //statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}


void TDriverTabbedEditor::recentFileUpdate(QString fileName)
{
    QStringList files = MEC::settings->value("editor/recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles) {
        files.removeLast();
    }

    QString dirName = MEC::absoluteFilePath(fileName);
    if (!dirName.isEmpty())    {
        MEC::settings->setValue("editor/defaultdir", dirName);
        //qDebug() << FFL << "STORED editor/defaultdir" << dirName;
    }

    MEC::settings->remove("editor/testValue");
    MEC::settings->setValue("editor/recentFileList", files);
    updateRecentFileActions();
}


void TDriverTabbedEditor::updateRecentFileActions()
{
    QStringList files = MEC::settings->value("editor/recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(files[i]); //arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setObjectName(QString("main recent %1").arg(i+1));
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int i = numRecentFiles; i < MaxRecentFiles; ++i) {
        recentFileActs[i]->setVisible(false);
    }
}


void TDriverTabbedEditor::updateTab(int index)
{
    QString label;
    QString suffix;

    if (index < 0) index = currentIndex();

    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(index));
    if (editor) {
        label = strippedName(editor->fileName());
        editor->setObjectName("editor file "+ label);
        setTabToolTip(index, editor->fileName());
        if (editor->document()->isModified()) suffix = " *";
    }

    if (label.isEmpty()) {
        label = "untitled " + QString::number(editor->getNoNameId());
        setTabToolTip(index, tr("Unnamed new file"));
    }

    label.append(suffix);
    if (editor->fileCodec()) {
        QString codecName(QString::fromAscii(editor->fileCodec()->name()));
        const char *bomSuffix = "";

        if (codecName.contains("utf", Qt::CaseInsensitive)) {
            bomSuffix = (editor->fileCodecUtfBom()) ? " BOM" : " w/o BOM";
        }

        label.append(tr(" (%1%2)").arg(codecName).arg(bomSuffix));
    }
    label.replace(QString("&"), QString("&&")); // single & defines keyboard shortcut
    setTabText(index, label);
}


void TDriverTabbedEditor::showEvent(QShowEvent *ev)
{
    QTabWidget::showEvent(ev);
    if (runConsoleVisible) runConsoleContainer->show();
    if (debugConsoleVisible) debugConsoleContainer->show();
    if (irConsoleVisible) irConsoleContainer->show();
}


void TDriverTabbedEditor::hideEvent(QHideEvent *ev)
{
    QTabWidget::hideEvent(ev);
    if (runConsoleVisible) runConsoleContainer->hide();
    if (debugConsoleVisible) debugConsoleContainer->hide();
    if (irConsoleVisible) irConsoleContainer->hide();
}


void TDriverTabbedEditor::dragEnterEvent(QDragEnterEvent *ev)
{
    //    qDebug() << FFL << "dropAction" << ev->dropAction();
    //    qDebug() << FFL << "formats" << ev->mimeData()->formats();
    //    qDebug() << FFL << "action" << ev->dropAction();
    //    qDebug() << FFL << "text" << ev->mimeData()->text();
    //    qDebug() << FFL << "html" << ev->mimeData()->html();
    //    qDebug() << FFL << "urls" << ev->mimeData()->urls();
    if (ev->dropAction() == Qt::CopyAction && ev->mimeData()->hasUrls()) {
        ev->acceptProposedAction();
        //qDebug() << FFL << "ACCEPT:" << ev->mimeData()->urls();
    }
}


void TDriverTabbedEditor::dropEvent(QDropEvent *ev)
{
    if (ev->dropAction() == Qt::CopyAction && ev->mimeData()->hasUrls()) {
        foreach(QUrl url, ev->mimeData()->urls()) {
            QString fileName = url.toLocalFile();
            if (fileName.isEmpty()) {
                qDebug() << FFL << "failed to convert to local file from url" << url;
                continue;
            }
            loadFile(fileName);
        }
    }
    else ev->ignore();
}


bool TDriverTabbedEditor::queryUnsavedFate(TDriverTabbedEditor::ActionContext context)
{
    QStringList modifiedNames;
    QString unsavedCurrentName;
    for(int index=0; index < count(); ++index) {
        const TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(index));
        if (!editor) continue; // ignore tab that is not TDriverCodeTextEdit
        if (!editor->document()->isModified()) continue; // ignore tab that is not modified

        QString name(editor->fileName().isEmpty()
                     ? "untitled "+QString::number(editor->getNoNameId())
                     : editor->fileName());
        modifiedNames << name;
        if (currentIndex() == index) {
            unsavedCurrentName = strippedName(name);
        }
    }

    if(modifiedNames.size()==0) return true; // no unsaved files, free to proceed

    QString title(tr(", unsaved files:"));

    QMessageBox msgBox(QMessageBox::Question,
                       "" /*title*/,
                       modifiedNames.join("\n"),
                       QMessageBox::NoButton);

    switch (context) {

    case BeforeExit:
        msgBox.addButton(QMessageBox::SaveAll);
        msgBox.addButton(QMessageBox::Discard)->setText(tr("Discard all"));
        title.prepend(tr("EXIT"));
        break;

    case BeforeRun:
        if (!unsavedCurrentName.isEmpty()) {
            msgBox.addButton(QMessageBox::Save)->setText(tr("Save current"));
        }
        if (unsavedCurrentName.isEmpty() || modifiedNames.size() > 1) {
            msgBox.addButton(QMessageBox::SaveAll);
        }
        msgBox.addButton(QMessageBox::Ignore)->setText(tr("Don't save"));
        title.prepend(tr("RUN ")+unsavedCurrentName);
        break;
    }

    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setWindowTitle(title);

    //msgBox.setDefaultButton(QMessageBox::SaveAll);

    int selection = msgBox.exec();

    switch(selection) {
    case QMessageBox::Discard:
    case QMessageBox::Ignore:
        return true; // proceed without saving

    case QMessageBox::Save:
        return saveCurrent();

    case QMessageBox::SaveAll:
        return saveAll(); // proceed if saveAll was successful

    case QMessageBox::Cancel:
    default:
        return false; // don't proceed
    }
}

bool TDriverTabbedEditor::mainCloseEvent(QCloseEvent *)
{
    return queryUnsavedFate(BeforeExit);
}


bool TDriverTabbedEditor::currentHasWritableCursor()
{
    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
    return !(!editor ||
             editor->isReadOnly() ||
             editor->textCursor().isNull());
}


bool TDriverTabbedEditor::smartInsert(QString text, bool prependParent, bool prependDot)
{
    static const QRegExp NoAppEx("[]})a-zA-Z!?.:#]$");
    static const QString NoDotAfter(".:#+-*/%$&|={[(");

    TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
    if (!editor || editor->isReadOnly() || text.isEmpty()) {
        return false;
    }

    QTextCursor tc = editor->textCursor();

    if (tc.hasSelection()) {
        // delete selected text and clear selection before doing any context-aware stuff
        tc.deleteChar();
    }

    tc.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor);
    QString wordBefore(tc.selectedText());
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::MoveAnchor);

    QChar charBefore;
    bool dotPrepended = false;
    if ( !wordBefore.isEmpty()) charBefore = wordBefore.at(wordBefore.size()-1);

    if ( prependDot && !NoDotAfter.contains(charBefore)) {
        text.prepend('.');
        dotPrepended = true;
    }
    if ( prependParent && (charBefore.isNull() || charBefore.isSpace() || !wordBefore.contains(NoAppEx))) {
        if (!dotPrepended) {
            text.prepend('.');
            dotPrepended = true;
        }
        text.prepend(appVariable());
    }

    editor->textCursor().insertText(text);
    editor->setFocus();
    return true;
}


void TDriverTabbedEditor::setParamMap(const QMap<QString, QString> &map)
{
    paramMap = map;
    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (editor) editor->setTranslationDatabase(map, MEC::settings);
    }
}


QString TDriverTabbedEditor::sutVariable() const
{
    if (editBarP) return editBarP->sutVariable();
    else return QString();
}


QString TDriverTabbedEditor::appVariable() const
{
    if (editBarP) return editBarP->appVariable();
    else return QString();
}


QMenuBar *TDriverTabbedEditor::createEditorMenuBar(QWidget *parent)
{
    QMenuBar *bar = new QMenuBar(parent);
    bar->setObjectName("editor");

    QMenu *fileMenu = bar->addMenu(tr("Fi&le"));
    bar->actions().last()->setObjectName("editor file");
    fileMenu->setObjectName("editor file");
    fileMenu->addActions(fileActions());
    fileMenu->addSeparator();
    fileMenu->addActions(recentFileActions());
    updateRecentFileActions();

    QMenu *editMenu = bar->addMenu(tr("&Edit"));
    bar->actions().last()->setObjectName("editor edit");
    editMenu->setObjectName("editor edit");
    editMenu->addActions(editActions());
    editMenu->addSeparator();
    editMenu->addActions(codeActions());

    QMenu *optMenu = bar->addMenu(tr("Toggles"));
    bar->actions().last()->setObjectName("editor opt");
    optMenu->setObjectName("editor opt");
    optMenu->addActions(optionActions());

    QMenu *runMenu = bar->addMenu(tr("&Run"));
    bar->actions().last()->setObjectName("editor run");
    runMenu->setObjectName("editor run");
    runMenu->addActions(runActions());

    return bar;
}


void TDriverTabbedEditor::setEditorFont(QFont font)
{
    editorFont = font;
    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (editor) setEditorFontAndTabWidth(editor, editorFont);
    }
}


void TDriverTabbedEditor::runFilePrep(QString fileName, TDriverRunConsole::RunRequestType type)
{
    proceedRunFilename = fileName;
    proceedRunType = type;
    proceedRunPending = true;
    emit requestRunPreparations(fileName);
}


bool TDriverTabbedEditor::proceedRun()
{
    if (!proceedRunPending) return false;

    proceedRunPending = false;
    return runConsole->runFile(proceedRunFilename, proceedRunType);
}


void TDriverTabbedEditor::addBreakpoint(struct MEC::Breakpoint bp)
{
    int setCount = 0;
    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (!editor) {
            //qDebug() << FFL << "skipping non-TDriverCodeTextEdit index"<<ind;
            continue;
        }

        if (editor->fileName() == bp.file) {
            qDebug() << FFL << "adding rdebug breakpoint: " << MEC::dumpBreakpoint(&bp);
            editor->rdebugBreakpoint(bp);
            ++setCount;
            //return; // same file may be open multiple times!
        }
    }
    if (setCount==0) {
        qDebug() << FFL << "rdebug reported a breakpoint to unopened file: " << MEC::dumpBreakpoint(&bp);
    }
}


void TDriverTabbedEditor::addBreakpointList(QList<struct MEC::Breakpoint> bpList)
{
    // TODO: optimization: do this differently to avoid O(breakpoints * tabs)

    resetBreakpoints(); // bpList is assumed to have all active breakpoints
    MEC::Breakpoint bp;
    foreach(bp, bpList) {
        addBreakpoint(bp);
    }
}


void TDriverTabbedEditor::gotoLine(const QString fileName, int lineNum)
{
    if (lineNum > 0 && !fileName.isEmpty()) {

        QList<TDriverCodeTextEdit*> editors = setupLineJump(fileName, lineNum);

        for (int ind = 0; ind < editors.count(); ++ind) {
            editors.at(ind)->gotoLine(lineNum);
            setCurrentWidget(editors.at(ind));
        }

        if (!editors.isEmpty()) {
            setCurrentWidget(editors.first());
            currentWidget()->activateWindow();
            currentWidget()->setFocus();
        }
    }
}

void TDriverTabbedEditor::gotoLine(const QUrl &fileLineSpec)
{
    QString fileName = fileLineSpec.toString().section(':', 0, -2);
    bool ok;
    int lineNum = fileLineSpec.toString().section(':', -1).toInt(&ok);

    if (ok) gotoLine(fileName, lineNum);
}

void TDriverTabbedEditor::setRunningLine(QString fileName, int lineNum)
{
    QList<TDriverCodeTextEdit*> editors = setupLineJump(fileName, lineNum);

    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (editor) {
            if(editors.contains(editor)) {
                // set running lines in all found tabs
                editor->setRunningLine(lineNum);
            }
            else {
                // clear running lines in other tabs
                editor->setRunningLine(0);
            }
        }

        if (!editors.isEmpty()) setCurrentWidget(editors.first());
    }
}


QList<TDriverCodeTextEdit*> TDriverTabbedEditor::setupLineJump(const QString &fileName, int lineNum)
{
    QList<TDriverCodeTextEdit*> ret;

    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (!editor) {
            //qDebug() << FFL << "skipping non-TDriverCodeTextEdit index"<<ind;
            continue;
        }

        if (editor->fileName() == fileName) {
            ret.append(editor);
        }
    }

    if (ret.isEmpty()) {
        // not set yet, ie. file not found in tabs, try to open!
        qDebug() << FFL << fileName <<"not open, opening";
        if (loadFile(fileName)) {
            TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(currentWidget());
            if (editor) {
                qDebug() << FFL << "made new tab for" << editor->fileName();
                ret.append(editor);
            }
            else qWarning("loadFile '%s' didn't create TDriverCodeTextEdit", qPrintable(fileName));
        }
        else {
            qDebug() << FFL << "loading file failed:" << fileName;
        }
    }

    return ret;
}


void TDriverTabbedEditor::resetRunningLines()
{
    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (editor) {
            editor->setRunningLine(0);
        }
    }
}


void TDriverTabbedEditor::syncTabsData()
{
    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (editor) {
            editor->dataSyncRequest();
        }
    }
    emit dataSyncDone();
}


void TDriverTabbedEditor::resetBreakpoints()
{
    for (int ind = 0; ind < count() ; ++ind) {
        TDriverCodeTextEdit *editor = qobject_cast<TDriverCodeTextEdit*>(widget(ind));
        if (editor) {
            editor->rdebugBreakpointReset();;
        }
    }
}
