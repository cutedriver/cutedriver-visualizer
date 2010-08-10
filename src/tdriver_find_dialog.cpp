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



#include "tdriver_main_window.h"

bool MainWindow::containsWords( QHash<QString, QString> itemData, QString text, bool caseSensitive, bool entireWords ) {

    bool result = false;

    Qt::CaseSensitivity caseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if ( entireWords ) {

        result = (
            itemData.value( "name" ).compare( text, caseSensitivity ) == 0 ||
            itemData.value( "type" ).compare( text, caseSensitivity ) == 0 ||
            itemData.value( "id"   ).compare( text, caseSensitivity ) == 0
        );

    } else {

        result = (
            itemData.value( "name" ).contains( text, caseSensitivity ) ||
            itemData.value( "type" ).contains( text, caseSensitivity ) ||
            itemData.value( "id"   ).contains( text, caseSensitivity )
        );

    }

    return result;

}

bool MainWindow::attributeContainsWords( int itemPtr, QString text, bool caseSensitive, bool entireWords )
{
    bool result = false;

    Qt::CaseSensitivity caseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QMapIterator<QString, QHash<QString, QString> > iterator( attributesMap.value( itemPtr ) );

    while ( iterator.hasNext() && !result ) {

        iterator.next();
        QString value = iterator.value().value( "value" );

        if ( entireWords ? ( value.compare( text, caseSensitivity ) == 0 ) : ( value.contains( text, caseSensitivity ) ) ) {
            result = true;
            break;
        }
    }

    return result;
}


void MainWindow::findNextTreeObject()
{
    // exit if no objects in tree
    QString findString = findDialogText->currentText();

    if ( objectTree->columnCount() < 1 || objectTree->invisibleRootItem()->childCount() < 1 ) {

        qDebug() << "findFromObjectTree: no objects in tree";
        QMessageBox::warning( 0, "Find", " No matches found with '" + findString + "' " );
        return;
    }

    bool backwards = findDialogBackwards->isChecked();
    bool matchCase = findDialogMatchCase->isChecked();
    bool entireWords = findDialogEntireWords->isChecked();
    bool searchWrapAround = findDialogWrapAround->isChecked();
    bool searchAttributes = findDialogAttributes->isChecked();


    // add item to combobox
    if ( findDialogText->findText( findString ) == -1 ) {
        findDialogText->addItem( findString );
    }

    QTreeWidgetItem *current = objectTree->currentItem();

    switch ( findDialogSubtreeOnly->checkState() ) {

    case Qt::Unchecked:
        if (findDialogSubtreeRoot != objectTree->topLevelItem(0)) {
            findDialogSubtreeRoot = objectTree->topLevelItem(0);
        }
        break;

    case Qt::PartiallyChecked:
        if (findDialogSubtreeRoot) {
            break;
        }
                // fall through

    case Qt::Checked:
        findDialogSubtreeOnly->setCheckState(Qt::PartiallyChecked);
        findDialogSubtreeRoot = current;
        break;
    }

    findFromSubTree(current, findString, backwards, matchCase, entireWords, searchWrapAround, searchAttributes);

}


QTreeWidgetItem *MainWindow::findDialogSubtreeNext(QTreeWidgetItem *current, QTreeWidgetItem *root, bool wrap)
{
    if (!current) return NULL; // invalid current item

    if (current->childCount() > 0) return current->child(0); // first child  is next

    // can't use QTreeWidget::itemBelow because it will not iterate into hidden subtrees

    while( current != root) {

        QTreeWidgetItem *par = current->parent();
        if (!par) break; // null parent -> probably child of objectTree->invisibleRootItem
        Q_ASSERT(par); // can't be at root of entire tree if here

        int ind = par->indexOfChild(current);
        Q_ASSERT(ind >= 0);
        if (ind+1 < par->childCount()) return par->child(ind+1); // next sibling is next

        current = par; // no next sibling, try next sibling of parent
    }

    if (!wrap) return NULL; // entire subtree done, no next
    else return current; // wrapped to subtree root
}


static inline QTreeWidgetItem *goToBottomChild(QTreeWidgetItem *node)
{
    forever {
        int count = node->childCount();
        if (count <= 0) return node;
        node = node->child(count-1);
    }
}


QTreeWidgetItem *MainWindow::findDialogSubtreePrev(QTreeWidgetItem *current, QTreeWidgetItem *root, bool wrap)
{
    if (!current) return NULL; // invalid current item

    Q_ASSERT(current != objectTree->invisibleRootItem());

    if (current == root || current == objectTree->topLevelItem(0)) {
        if (!wrap) return NULL; // at subtree root, no next
        else return goToBottomChild(current); // wrap to end of tree
        // not reached
    }

    // can't use QTreeWidget::itemAbove because it will not iterate into hidden subtrees

    QTreeWidgetItem *par = current->parent();
    Q_ASSERT(par && par != objectTree->invisibleRootItem());

    int ind = par->indexOfChild(current);
    Q_ASSERT(ind >= 0 && ind < par->childCount());
    if (ind == 0) return par; // no previous sibling, so parent is previous
    else return goToBottomChild(par->child(ind-1)); // last descendant of previous sibling is previous
}


bool MainWindow::compareTreeItem(QTreeWidgetItem *item, const QString &findString, bool matchCase, bool entireWords, bool searchAttributes)
{
    if (!item) return false;

    int itemIndex = (int)item;
    Q_ASSERT(objectTreeData.contains( itemIndex) );

    // check values in itemData
    QHash<QString, QString> itemData = objectTreeData.value( itemIndex );
    if ( containsWords( itemData, findString, matchCase, entireWords ) ) {
        return true;
    }

    // check attribute values if that option is checked
    if ( searchAttributes ) {
        if ( attributeContainsWords( itemIndex, findString, matchCase, entireWords ) ) {
            return true;
        }
    }

    return false;
}


void MainWindow::findFromSubTree(QTreeWidgetItem *current, const QString &findString, bool backwards, bool matchCase, bool entireWords, bool searchWrapAround, bool searchAttributes)
{
    Q_ASSERT(findDialogSubtreeRoot);
    QTreeWidgetItem *startItem = current;

    forever {

        current = backwards ?
                  findDialogSubtreePrev(current, findDialogSubtreeRoot, searchWrapAround) :
                  findDialogSubtreeNext(current, findDialogSubtreeRoot, searchWrapAround);

        if (compareTreeItem(current, findString, matchCase, entireWords, searchAttributes)) {
            // found
            Q_ASSERT( objectTreeMap.contains( (int)current ) && current == objectTreeMap.value( (int)current ));
            objectTree->setCurrentItem( current );
            return;
        }

        if (!current || current == startItem) {
            QMessageBox::warning( 0, "Find", " No matches found with '" + findDialogText->currentText() + "' " );
            return;
        }
    }
}


void MainWindow::findDialogTextChanged( const QString & text )
{
    findDialogFindButton->setEnabled( !text.isEmpty() );
}


void MainWindow::objectTreeCurrentChanged(QTreeWidgetItem*current)
{
    if (!findDialogSubtreeOnly || !current) return;

    if (findDialogSubtreeOnly->checkState() != Qt::Unchecked) {
        while(current) {
            if (current == findDialogSubtreeRoot) return; // ok!
            current = current->parent();
        }
        // current not in selected subtree, switch off subtree-only searching
        findDialogSubtreeOnly->setCheckState(Qt::Unchecked);
    }
    else findDialogSubtreeOnly = NULL;
}


void MainWindow::findDialogSubtreeChanged( int state)
{
    if (state != Qt::PartiallyChecked) {
        findDialogSubtreeRoot = NULL;
        // prevent user from switching state to PartiallyChecked
        findDialogSubtreeOnly->setTristate(false);
    }
}


void MainWindow::showFindDialog() {

    findDialogSubtreeRoot = NULL;
    if (findDialogSubtreeOnly->checkState() == Qt::PartiallyChecked)
        findDialogSubtreeOnly->setCheckState(Qt::Checked);
    findDialog->show();

    if ( findDialogPos != QPoint( -1, -1 ) ) { findDialog->move( findDialogPos ); }

    findDialogText->setFocus();

}

void MainWindow::closeFindDialog() {

    // store find dialog position
    findDialogPos = findDialog->pos();

    findDialog->close();

}

void MainWindow::createFindDialog() {

    findDialogSubtreeRoot = NULL;

    findDialog = new QDialog( this );
    findDialog->setObjectName( "main find" );
    findDialog->setWindowTitle( "Find" );

    findDialog->setFixedSize( 560, 155 );

    // reset find dialog position, stored before closing the dialog and restored when dialog opened
    findDialogPos = QPoint(-1, -1);

    // main layout
    QVBoxLayout *layout = new QVBoxLayout( findDialog );
    layout->setObjectName("main find");

    // group box with title
    QGroupBox *groupBox = new QGroupBox();
    groupBox->setObjectName("main find group");
    groupBox->setTitle( "Search for:" );

    // groupBox layout
    QGridLayout *groupBoxLayout = new QGridLayout( groupBox );
    groupBoxLayout->setObjectName("main find group");

    // search text combobox
    findDialogText = new QComboBox();
    findDialogText->setObjectName("main find text");
    findDialogText->setEditable( true );
    findDialogText->setDuplicatesEnabled( false );

    // buttons: find & close
    findDialogFindButton = new QPushButton( "&Find", this );
    findDialogFindButton->setObjectName("main find find");
    findDialogFindButton->setEnabled( false );
    findDialogFindButton->setDefault( true );
    findDialogFindButton->setAutoDefault( true );

    findDialogCloseButton = new QPushButton( "&Close", this );
    findDialogCloseButton->setObjectName("main find close");
    findDialogCloseButton->setDefault( false );
    findDialogCloseButton->setAutoDefault( false );

    findDialogMatchCase = new QCheckBox( "&Match case" );
    findDialogMatchCase->setObjectName("main find matchcase");
    findDialogBackwards = new QCheckBox( "Search &backwards" );
    findDialogBackwards->setObjectName("main find backwards");

    findDialogEntireWords = new QCheckBox( "Match &entire word only" );
    findDialogEntireWords->setObjectName("main find entirewords");

    findDialogWrapAround = new QCheckBox( "&Wrap around" );
    findDialogWrapAround->setObjectName("main find wrap");
    findDialogWrapAround->setCheckState( Qt::Checked );

    findDialogAttributes = new QCheckBox( "&Attribute values" );
    findDialogAttributes->setObjectName("main find attributevalues");
    findDialogAttributes->setCheckState( Qt::Checked );

    findDialogSubtreeOnly = new QCheckBox( "Current &subtree only" );
    findDialogSubtreeOnly->setObjectName("main find subtree");
    findDialogSubtreeOnly->setTristate(false);

    // populate widgets
    groupBoxLayout->addWidget( findDialogText, 0, 0, 1, 4 );

    groupBoxLayout->addWidget( findDialogMatchCase, 1, 0 );
    groupBoxLayout->addWidget( findDialogBackwards, 1, 1 );
    groupBoxLayout->addWidget( findDialogAttributes, 1, 2 );

    groupBoxLayout->addWidget( findDialogEntireWords, 2, 0 );
    groupBoxLayout->addWidget( findDialogWrapAround, 2, 1 );
    groupBoxLayout->addWidget( findDialogSubtreeOnly, 2, 2 );

    groupBoxLayout->addWidget( findDialogFindButton, 1, 3 );
    groupBoxLayout->addWidget( findDialogCloseButton, 2, 3 );

    layout->addWidget( groupBox );

    connect( findDialogSubtreeOnly, SIGNAL(stateChanged(int)),
             this, SLOT(findDialogSubtreeChanged(int)));
    connect( findDialogText, SIGNAL( editTextChanged( const QString & ) ),
             this, SLOT( findDialogTextChanged( const QString & ) ) );

    //Q_ASSERT(objectTree);    connect (objectTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),             this, SLOT(objectTreeCurrentChanged(QTreeWidgetItem*)));

    connect( findDialogFindButton, SIGNAL( pressed() ), this, SLOT( findNextTreeObject() ) );
    connect( findDialogCloseButton, SIGNAL( pressed() ), this, SLOT( closeFindDialog() ) );

}


