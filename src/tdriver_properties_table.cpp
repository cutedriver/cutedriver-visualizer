/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (tdriversupport@nokia.com)
**
** This file is part of Testability Driver.
**
** If you have questions regarding the use of this file, please contact
** Nokia at tdriversupport@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/


#include "tdriver_main_window.h"
#include <tdriver_tabbededitor.h>

#include <tdriver_debug_macros.h>

void MainWindow::tabWidgetChanged( int currentTableWidget ) {

    Q_UNUSED( currentTableWidget );
    doPropertiesTableUpdate();

}

void MainWindow::clearPropertiesTableContents() {

#if !DISABLE_API_TAB_PENDING_REMOVAL
    // clear api table contents
    apiTable->clearContents();
    apiTable->setRowCount( 0 );
#endif
    // clear methods table contents
    methodsTable->clearContents();
    methodsTable->setRowCount( 0 );

    // clear signals table contents
    signalsTable->clearContents();
    signalsTable->setRowCount( 0 );

    // clear properties table contents
    propertiesTable->clearContents();
    propertiesTable->setRowCount( 0 );

    propertyTabLastTimeUpdated.clear();

}


void MainWindow::doPropertiesTableUpdate()
{
    if ( objectTree->currentItem() != NULL ) {
        // retrieve pointer of current item selected in object tree
        TestObjectKey currentItemPtr = ptr2TestObjectKey(objectTree->currentItem());

        // retrieve current table index
        int currentTab = tabWidget->currentIndex();

        if ( currentTab == 0 && propertyTabLastTimeUpdated.value( "attributes" ) != currentItemPtr ) {
            updateAttributesTableContent();
        }
        else if ( currentTab == 1 && propertyTabLastTimeUpdated.value( "methods" ) != currentItemPtr ) {
            updateMethodsTableContent();
        }
        else if ( currentTab == 2 && propertyTabLastTimeUpdated.value( "signals" ) != currentItemPtr ) {
            sendUpdateSignalsTableContent();
        }
        else if ( currentTab == 3  && propertyTabLastTimeUpdated.value( "api_methods" ) != currentItemPtr ) {
            sendUpdateApiTableContent();
        }
    }
}


void MainWindow::sendUpdateApiTableContent()
{
    TestObjectKey currentItemPtr = ptr2TestObjectKey( objectTree->currentItem() );

#if !DISABLE_API_TAB_PENDING_REMOVAL
    // clear methods table contents
    apiTable->clearContents();
    apiTable->setRowCount( 0 );
    // update table only if item selected in object tree
    if ( objectTree->currentItem() != NULL && apiFixtureEnabled ) {

        QString objectType = objectTreeData.value( currentItemPtr ).type;

        if ( !objectType.isEmpty() ) {

            if ( !apiFixtureChecked ) {
                qDebug() << "requesting API fixture check";
                sendTDriverCommand(commandCheckApiFixture,
                                   QStringList() << activeDevice << "check_fixture",
                                   "checking API fixture");
                // response handler for commandCheckApiFixture will call this method again
                return;
            }

            // retrieve methods using fixture if not already found from api methods cache
            if ( !apiMethodsMap.contains( objectType ) ) {
                qDebug() << "requesting apiMethods for " << objectType;
                sendTDriverCommand(commandClassMethods,
                                   QStringList() << activeDevice << "fixture" << objectType,
                                   "class methods for " + objectType,
                                   objectType);
                // response handler for commandClassMethods will call this method again
                return;

            } else {
                qDebug() << "apiMethods for " << objectType << " found";
            }

            QMap<QString, QHash<QString, QString> > methodsMap = apiMethodsMap.value( objectType );

            QMap<QString, QHash<QString, QString> >::iterator methodsIterator;
            for ( methodsIterator = methodsMap.begin(); methodsIterator != methodsMap.end(); ++methodsIterator ) {

                // retrieve current item count
                int rowNumber = apiTable->rowCount();

                // insert new line to table
                apiTable->insertRow( rowNumber );

                QString methodName      = methodsIterator.key();
                QString returnValueType = methodsIterator.value().value( "returnValueType" );
                QString arguments       = methodsIterator.value().value( "arguments" );

                // add return value type
                QTableWidgetItem *methodReturnValue = new QTableWidgetItem( returnValueType );
                methodReturnValue->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
                methodReturnValue->setFont( *defaultFont );
                if ( returnValueType.isEmpty() || returnValueType == "void" ) {
                    methodReturnValue->setBackground( QBrush( Qt::lightGray ) );
                }
                apiTable->setItem( rowNumber, 0, methodReturnValue );

                // add method name
                QTableWidgetItem *methodNameItem = new QTableWidgetItem( methodName );
                methodNameItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
                methodNameItem->setFont( *defaultFont );
                apiTable->setItem( rowNumber, 1, methodNameItem );

                // add method arguments
                QTableWidgetItem *methodArguments = new QTableWidgetItem( arguments );
                methodArguments->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
                methodArguments->setFont( *defaultFont );
                if ( arguments.isEmpty() ) { methodArguments->setBackground( QBrush( Qt::lightGray ) ); }
                apiTable->setItem( rowNumber, 2, methodArguments );

                // qDebug() << methodsIterator.key() << ": " << methodsIterator.value() << endl;

            }

        }


    }
#else
    Q_ASSERT(!apiFixtureEnabled);
    Q_ASSERT(apiFixtureChecked);
#endif

    propertyTabLastTimeUpdated.insert( "api_methods", currentItemPtr );

}

void MainWindow::updateMethodsTableContent() {

    // qDebug() << "updateMethodsTableContent";

    TestObjectKey currentItemPtr = ptr2TestObjectKey( objectTree->currentItem() );

    Behaviour behaviour;

    // clear methods table contents
    methodsTable->clearContents();
    methodsTable->setRowCount( 0 );

    // update table only if item selected in object tree
    if ( objectTree->currentItem() != NULL ) {

        // retrieve current item object type
        QString currentItemObjectType = objectTree->currentItem()->data( 0, Qt::DisplayRole ).toString();

        QStringList objectTypes;
        //objectTypes << "*" << currentItemObjectType;
        objectTypes << currentItemObjectType;

        // retrieve methods ( behaving-object/test-object[@value = "*"] and behaving-object/test-object[@value = objectType] )
        for ( int objectTypeIndex = 0; objectTypeIndex < objectTypes.size(); objectTypeIndex++ ) {

            if ( behavioursMap.contains( objectTypes.at( objectTypeIndex ) ) ) {

                behaviour = behavioursMap.value( objectTypes.at( objectTypeIndex ) );

                QStringList methodsList = behaviour.getMethodsList();

                for ( int methodIndex = 0; methodIndex < methodsList.size(); methodIndex++ ) {

                    // retrieve method hash
                    QMap<QString, QString> method = behaviour.getMethod( methodsList.at( methodIndex ) );

                    // retrieve current item count
                    int rowNumber = methodsTable->rowCount();

                    // insert new line to table
                    methodsTable->insertRow( rowNumber );

                    // add method name
                    QTableWidgetItem *methodName = new QTableWidgetItem( methodsList.at( methodIndex ) );
                    methodName->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
                    methodName->setToolTip( method.value( "description" ) );
                    methodName->setFont( *defaultFont );
                    methodsTable->setItem( rowNumber, 0, methodName );

                    // add method example
                    QTableWidgetItem *methodExample = new QTableWidgetItem( method.value( "example" ) );
                    methodExample->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
                    methodExample->setToolTip( method.value( "description" ) );
                    methodExample->setFont( *defaultFont );
                    methodsTable->setItem( rowNumber, 1, methodExample );

                }

            }

        }

        // sort methods table
        methodsTable->sortItems( 0 );

    }

    // store pointer of current item to table, so methods table won't be updated unless item is changed on object tree
    propertyTabLastTimeUpdated.insert( "methods", currentItemPtr );
}


bool MainWindow::sendUpdateSignalsTableContent()
{
    // qDebug() << "updateSignalsTableContent";

    TestObjectKey currentItemPtr = ptr2TestObjectKey( objectTree->currentItem() );

    // store pointer of current item to table, so signals table won't be updated unless item is changed on object tree
    propertyTabLastTimeUpdated.insert( "signals", currentItemPtr );

    // clear signals table contents
    signalsTable->clearContents();
    signalsTable->setRowCount( 0 );

    // update table only if item selected in object tree
    if ( currentItemPtr != 0 ) {

        // retrieve current item object type
        QString objectType = objectTree->currentItem()->data( 0, Qt::DisplayRole ).toString();
        QString objectId   = objectTreeData.value(currentItemPtr).id;

        // Retrieve the signals from the device
        if (objectType != "sut" && objectType != "application" && objectType != "QAction") {

            // list_signals
            if (activeDevice.contains("qt")){
                if (!apiSignalsMap.contains(objectType)) {

                    QStringList cmd(QStringList()
                                    << activeDevice << "list_signals" << currentApplication.name << objectId << objectType);
                    return sendTDriverCommand(commandSignalList, cmd, "signal list", objectType);
                }
            }
        }
    }
    return false;
}


void MainWindow::updateAttributesTableContent()
{
    // disconnect itemChanged signal listening, reconnect after table is updated
    disconnect(propertiesTable, SIGNAL(itemChanged(QTableWidgetItem*)),
               this, SLOT(changePropertiesTableValue(QTableWidgetItem*)) );

    // retrieve pointer of currently selected objectTree item
    TestObjectKey currentItemPtr = ptr2TestObjectKey( objectTree->currentItem() );

    // clear properties table contents
    propertiesTable->clearContents();
    propertiesTable->setRowCount( 0 );

    if ( attributesMap.contains( currentItemPtr ) && objectTree->currentItem() != NULL ) {

        // set number of attributes in table
        propertiesTable->setRowCount( attributesMap.value( currentItemPtr ).size() );

        // retrieve current objects attributes (using pointer of QTreeWidgetItem)
        QMapIterator<QString, AttributeInfo > iterator( attributesMap.value( currentItemPtr ) );

        int index = 0;
        while ( iterator.hasNext() ) {
            iterator.next();

            QString attributeName      = iterator.value().name;
            QString attributeValue     = iterator.value().value;
            QString attributeType      = iterator.value().type;

            // Attribute name
            QTableWidgetItem *attributeNameItem = new QTableWidgetItem( attributeName );
            attributeNameItem->setFont( *defaultFont );
            attributeNameItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            propertiesTable->setItem( index, 0, attributeNameItem );

            // Attribute value
            QTableWidgetItem *attributeValueItem = new QTableWidgetItem( attributeValue );
            attributeValueItem->setFont( *defaultFont );

            // Attribute type - non writable attributes must not be accessible

            if ( !attributeType.contains( "writable", Qt::CaseInsensitive ) ) {

                // make item selectabe & enabled for copying, but not editable
                //Qt::ItemFlags newFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
                attributeValueItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

                // paint item gray
                attributeValueItem->setBackground( QBrush( Qt::lightGray ) );

            }

            propertiesTable->setItem( index, 1, attributeValueItem );
            index++;
        }

        connect(propertiesTable, SIGNAL(itemChanged(QTableWidgetItem*)),
                SLOT(changePropertiesTableValue(QTableWidgetItem*)) );
        propertiesTable->update();
    }
    propertyTabLastTimeUpdated.insert( "attributes", currentItemPtr );
}


void MainWindow::connectTabWidgetSignals()
{
    connect(tabWidget, SIGNAL(currentChanged(int)),
            SLOT(tabWidgetChanged(int)));

    connect(methodsTable, SIGNAL(itemPressed(QTableWidgetItem*)),
            SLOT(methodItemPressed(QTableWidgetItem*)) );

    connect(propertiesTable, SIGNAL(itemPressed(QTableWidgetItem*)),
            SLOT(propertiesItemPressed(QTableWidgetItem*)) );

#if !DISABLE_API_TAB_PENDING_REMOVAL
    connect(apiTable, SIGNAL(itemPressed(QTableWidgetItem*)),
            SLOT(apiItemPressed(QTableWidgetItem*)) );
#endif
}


void MainWindow::changePropertiesTableValue( QTableWidgetItem *item )
{
    TestObjectKey currentItemPtr = ptr2TestObjectKey( objectTree->currentItem() );
    const TreeItemInfo &treeItemData = objectTreeData.value( currentItemPtr );

    // this feature is not supported in with env != qt
    if (treeItemData.env.toLower() == "qt") {

        QTableWidgetItem *currentItem = propertiesTable->item( propertiesTable->row( item ), 0 );
        QString attributeName = currentItem->text();
        QString objRubyId = treeItemData.type;

        objRubyId.append('(');
        if ( treeItemData.name != "application" ) {
            objRubyId.append(":name=>" + TDriverUtil::rubySingleQuote(treeItemData.name) +",");
        }
        objRubyId.append(":id=>"+TDriverUtil::rubySingleQuote(treeItemData.id));
        objRubyId.append(')');

        QString targetDataType = attributesMap.value(currentItemPtr).value(attributeName).dataType;

        if (targetDataType.size() == 0) {
            QMessageBox::warning(this,
                                 tr("Incompatible Attribute"),
                                 tr("No data type found for attribute ") + attributeName );

        } else {
            QStringList cmd(QStringList()
                            << activeDevice << "set_attribute"
                            << objRubyId << targetDataType << attributeName << item->text());

            if (sendTDriverCommand(commandSetAttribute, cmd, tr("set attribute")) ) {
                propertiesDock->setDisabled(true);
                statusbar(tr("Attribute change request sent..."));
            }
            else {
                QMessageBox::critical(this,
                                      tr("Attribute Change Error"),
                                      tr("Failed to send attribute change request!"));
            }
        }
    }
    else {
        qDebug() << FCFL << "tried to change non-qt property, fail";
        QMessageBox::warning(this,
                             tr("Not Supported by Current Device"),
                             tr("Current device / SUT does not support changing attributes"));
    }
}


void MainWindow::methodItemPressed( QTableWidgetItem * item ) {

    if (QApplication::mouseButtons() == Qt::RightButton) {

        ContextMenuSelection action = showCopyAppendContextMenu();
        if (action > cancelAction) {

            QString text(item->text());
            bool fullPath = isPathAction(action);

            if (fullPath) {
                QTreeWidgetItem * treeItem = objectTree->currentItem();
                TestObjectKey sutItemPtr = ptr2TestObjectKey(objectTree->topLevelItem(0));
                do {
                    text = TDriverUtil::smartJoin(
                                treeObjectRubyId(ptr2TestObjectKey(treeItem), sutItemPtr), '.', text);
                } while (ptr2TestObjectKey(treeItem) != sutItemPtr && (treeItem = treeItem->parent()));
            }

            switch (action) {

            case appendAction:
            case copyAction:
            case appendPathAction:
            case copyPathAction:
                updateClipboardText(text,  (action==appendAction) );
                break;

            case insertAction:
            case insertPathAction:
                emit insertToCodeEditor(text + '\n', false, !fullPath);
                break;

            default:
                qWarning("Bad context menu action in %s:%s", __FILE__, __FUNCTION__);
            }

        }
    }
}


// Handle (right) clicks on properties table: display context menu
void MainWindow::propertiesItemPressed ( QTableWidgetItem * item )
{
    if (QApplication::mouseButtons() == Qt::RightButton) {

        ContextMenuSelection action = showCopyAppendContextMenu();
        //QString selectedCommand = item->text();
        // QClipboard *clipboard = QApplication::clipboard();

        // only react to click if one of the menu choices was clicked
        if ( action > cancelAction ) {
            bool fullPath = isPathAction(action);
            QTreeWidgetItem * treeItem = objectTree->currentItem();
            QString objectType = treeItem->data( 0, Qt::DisplayRole ).toString();

            QList<QTableWidgetItem *> selectedItems;
            selectedItems = item->tableWidget()->selectedItems();

            // combine object type and selected attribute rows into a TDriver ruby test object selection script
            QString objRubyId = objectType + "(";
            for ( int i = 0; i < selectedItems.size(); i++ ) {
                if (i > 0) {
                    objRubyId += ", ";
                }

                objRubyId += ":"
                        + propertiesTable->item( selectedItems[ i ]->row(), 0 )->text()
                        + " => "
                        + TDriverUtil::rubySingleQuote(propertiesTable->item( selectedItems[ i ]->row(), 1 )->text());
                qDebug() << FCFL << objRubyId;
            }

            objRubyId += ")";

            if (fullPath) {
                TestObjectKey sutItemPtr = ptr2TestObjectKey(objectTree->topLevelItem(0));
                while (ptr2TestObjectKey(treeItem) != sutItemPtr && (treeItem = treeItem->parent())) {
                    objRubyId = TDriverUtil::smartJoin(
                                treeObjectRubyId(ptr2TestObjectKey(treeItem), sutItemPtr), '.', objRubyId);
                }
            }

            switch (action) {

            case appendAction:
            case copyAction:
            case appendPathAction:
            case copyPathAction:
                updateClipboardText( objRubyId, action == appendAction );
                break;

            case insertAction:
            case insertPathAction:
                emit insertToCodeEditor(objRubyId, !fullPath, !fullPath);
                break;

            default:
                qDebug() << FCFL << "Unhandled context menu action" << action;
            }
        }
    }
}


void MainWindow::apiItemPressed( QTableWidgetItem * item )
{

    Q_UNUSED( item );

    if (QApplication::mouseButtons() == Qt::RightButton)
    {

        QMenu contextMenu(this);
        QAction *item1 = new QAction(tr("Sorry, this feature is not yet implemented"), this);
        item1->setObjectName("main api item1");
        contextMenu.addAction(item1);
        contextMenu.exec(QCursor::pos() );

    }

}
