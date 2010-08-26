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
    updatePropetriesTable();

}

void MainWindow::clearPropertiesTableContents() {

    // clear api table contents
    apiTable->clearContents();
    apiTable->setRowCount( 0 );

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

void MainWindow::updatePropetriesTable()
{
    // qDebug() << "updatePropetriesTable()";

    if ( objectTree->currentItem() != NULL ) {
        // retrieve pointer of current item selected in object tree
        int currentItemPtr = (int)(objectTree->currentItem());

        // retrieve current table index
        int currentTab = tabWidget->currentIndex();

        if ( currentTab == 0 && propertyTabLastTimeUpdated.value( "attributes" ) != currentItemPtr ) {
            updateAttributesTableContent();

        } else if ( currentTab == 1 && propertyTabLastTimeUpdated.value( "methods" ) != currentItemPtr ) {
            updateMethodsTableContent();

        } else if ( currentTab == 2 && propertyTabLastTimeUpdated.value( "signals" ) != currentItemPtr ) {
            updateSignalsTableContent();

        } else if ( currentTab == 3  && propertyTabLastTimeUpdated.value( "api_methods" ) != currentItemPtr ) {
            updateApiTableContent();
        }
    }
}


bool MainWindow::checkApiFixture()
{
    return execute_command( commandCheckApiFixture, activeDevice.value( "name" ) + " check_fixture");
}


void MainWindow::getClassMethods( QString objectType )
{
    if ( execute_command( commandClassMethods,
                          activeDevice.value( "name" ) + " fixture " + objectType,
                          objectType ) ) {
        parseApiMethodsXml( outputPath + "/visualizer_class_methods_" + activeDevice.value( "name" ) + ".xml" );
    }
}


void MainWindow::getClassSignals(QString objectType, QString objectId)
{
    // list_signals
    if (activeDevice.value( "name" ).contains("qt")){
        if (!apiSignalsMap.contains(objectType)) {
            if ( execute_command( commandSignalList,
                                  activeDevice.value( "name" ) + " list_signals " + currentApplication.value( "name" ) + " " + objectId + " " + objectType )) {
                apiSignalsMap.insert(objectType,
                                     parseSignalsXml( outputPath + "/visualizer_class_signals_" + activeDevice.value( "name" ) + ".xml" ));
            }
        }
    }
}


void MainWindow::updateApiTableContent() {

    int currentItemPtr = ( int )( objectTree->currentItem() );

    // clear methods table contents
    apiTable->clearContents();
    apiTable->setRowCount( 0 );

    // update table only if item selected in object tree
    if ( objectTree->currentItem() != NULL && apiFixtureEnabled ) {

        QString objectType = objectTreeData.value( currentItemPtr ).value( "type" );

        if ( !objectType.isEmpty() ) {

            if ( !apiFixtureChecked ) {

                if ( !checkApiFixture() ) {

                    // fixture not found
                    QMessageBox::critical( 0, tr( "Error" ), "API fixture is not installed, unable to retrieve class methods.\n\nDisabling API tab from Properties table." );
                    tabWidget->setTabEnabled( tabWidget->indexOf( apiTab ), false );
                    apiFixtureEnabled = false;
                    apiFixtureChecked = true;
                    return;

                } else {

                    // fixture found
                    apiFixtureChecked = true;

                }

            }

            // retrieve methods using fixture if not already found from api methods cache
            if ( !apiMethodsMap.contains( objectType ) ) {

                getClassMethods( objectType );

            } else {

                // qDebug() << "apiMethods for " << objectType << " found from cache";

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
                if ( returnValueType.isEmpty() || returnValueType == "void" ) { methodReturnValue->setBackground( QBrush( Qt::lightGray ) ); }
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

    propertyTabLastTimeUpdated.insert( "api_methods", currentItemPtr );

}

void MainWindow::updateMethodsTableContent() {

    // qDebug() << "updateMethodsTableContent";

    int currentItemPtr = (int)( objectTree->currentItem() );

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
void MainWindow::updateSignalsTableContent() {

    // qDebug() << "updateSignalsTableContent";

    int currentItemPtr = (int)( objectTree->currentItem() );

    // clear signals table contents
    signalsTable->clearContents();
    signalsTable->setRowCount( 0 );

    // update table only if item selected in object tree
    if ( objectTree->currentItem() != NULL ) {

        // retrieve current item object type
        QString currentItemObjectType = objectTree->currentItem()->data( 0, Qt::DisplayRole ).toString();

        if (currentItemObjectType == "sut" || currentItemObjectType == "application") return;

        int currentItemPtr = ( int )( objectTree->currentItem() );
        QHash<QString, QString> treeItemData = objectTreeData.value( currentItemPtr );
        QString objectId   = treeItemData.value( "id" );

        // Retrieve the signals from the device
        getClassSignals(currentItemObjectType, objectId);

        // If no signals found for the current type of object, return
        if (!apiSignalsMap.contains(currentItemObjectType)) {return;}
        QStringList signalsList = apiSignalsMap[currentItemObjectType];

        for ( int signalIndex = 0; signalIndex < signalsList.size(); signalIndex++ ) {

            // retrieve signal hash
            QString signal = signalsList.at( signalIndex  );

            // retrieve current item count
            int rowNumber = signalsTable->rowCount();

            // insert new line to table
            signalsTable->insertRow( rowNumber );

            // add signal name
            QTableWidgetItem *signalName = new QTableWidgetItem( signalsList.at( signalIndex ) );
            signalName->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            signalName->setFont( *defaultFont );
            signalsTable->setItem( rowNumber, 0, signalName );

        }
        // sort signals table
        signalsTable->sortItems( 0 );
        signalsTable->resizeColumnToContents (0);

    }

    // store pointer of current item to table, so signals table won't be updated unless item is changed on object tree
    propertyTabLastTimeUpdated.insert( "signals", currentItemPtr );

}


void MainWindow::updateAttributesTableContent() {

    // qDebug() << "updateAttributesTableContent()";

    // disconnect itemChanged signal listening, reconnect after table is updated
    QObject::disconnect( propertiesTable, SIGNAL( itemChanged( QTableWidgetItem * ) ), this, SLOT( changePropertiesTableValue( QTableWidgetItem* ) ) );

    // retrieve pointer of currently selected objectTree item
    int currentItemPtr = ( int )( objectTree->currentItem() );

    QMap<QString, QHash<QString, QString> > attributeMap;
    QHash<QString, QString> attributeHash;

    // clear properties table contents
    propertiesTable->clearContents();
    propertiesTable->setRowCount( 0 );

    if ( attributesMap.contains( currentItemPtr ) && objectTree->currentItem() != NULL ) {

        // set number of attributes in table
        propertiesTable->setRowCount( attributesMap.value( currentItemPtr ).size() );

        // retrieve current objects attributes (using pointer of QTreeWidgetItem)
        QMapIterator<QString, QHash<QString, QString> > iterator( attributesMap.value( currentItemPtr ) );

        int index = 0;

        while ( iterator.hasNext() ) {

            iterator.next();

            QString attributeName      = iterator.value().value("name");
            QString attributeValue     = iterator.value().value("value");
            QString attributeType      = iterator.value().value("type");

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

        QObject::connect( propertiesTable, SIGNAL( itemChanged( QTableWidgetItem * ) ), this, SLOT( changePropertiesTableValue( QTableWidgetItem* ) ) );

        propertiesTable->update();

    }

    propertyTabLastTimeUpdated.insert( "attributes", currentItemPtr );

}


void MainWindow::connectTabWidgetSignals()
{

    QObject::connect( tabWidget, SIGNAL( currentChanged( int ) ), this, SLOT( tabWidgetChanged( int ) ) );

    QObject::connect( methodsTable, SIGNAL( itemPressed( QTableWidgetItem* ) ), this, SLOT( methodItemPressed( QTableWidgetItem* ) ) );
    QObject::connect( propertiesTable, SIGNAL( itemPressed( QTableWidgetItem * ) ), this, SLOT( propertiesItemPressed( QTableWidgetItem* ) ) );
    QObject::connect( apiTable, SIGNAL( itemPressed( QTableWidgetItem * ) ), this, SLOT( apiItemPressed( QTableWidgetItem* ) ) );

}


void MainWindow::changePropertiesTableValue( QTableWidgetItem *item )
{
    // this feature is not supported in with s60 devices
    if ( activeDevice.value( "type" ).toLower() != "s60" ) {

        int currentItemPtr = ( int )( objectTree->currentItem() );

        QTableWidgetItem *currentItem = propertiesTable->item( propertiesTable->row( item ), 0 );
        QString attributeName = currentItem->text();

        QHash<QString, QString> treeItemData = objectTreeData.value( currentItemPtr );

        QString objType = treeItemData.value( "type" );
        QString objName = treeItemData.value( "name" );
        QString objId   = treeItemData.value( "id"   );

        QString objIdentification = objType;

        if ( objName != "application" ) {
            objIdentification.append("(:name=>'" + objName + "',:id=>'" + objId + "')");

        } else {
            objIdentification.append("(:id=>'" + objId + "')");
        }

        QString targetDataType = attributesMap.value( currentItemPtr ).value( attributeName ).value( "datatype" );

        if (targetDataType.size() == 0) {
            QMessageBox::critical( 0, "Error", "No data type found for attribute " + attributeName );

        } else {
            QString attributeNameAndValue = attributeName + " '" + item->text() + "'";
            if ( execute_command( commandSetAttribute,
                                  activeDevice.value( "name" ) + " set_attribute " + objIdentification + " " + targetDataType + " " + attributeNameAndValue ) ) {
                refreshData();
            }
        }
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
                int sutItemPtr = ( int )objectTree->topLevelItem(0);
                do {
                    text.prepend( treeObjectIdentification((int)treeItem, sutItemPtr) + '.');
                } while ((int)treeItem != sutItemPtr && (treeItem = treeItem->parent()));
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
            QString objectIdentification = objectType + "(";

            for ( int i = 0; i < selectedItems.size(); i++ ) {
                if (i > 0) { objectIdentification += ", "; }
                objectIdentification += ":" + propertiesTable->item( selectedItems[ i ]->row(), 0 )->text() + " => '" + propertiesTable->item( selectedItems[ i ]->row(), 1 )->text() + "'";
            }

            objectIdentification += ")";

            if (fullPath) {
                int sutItemPtr = ( int )objectTree->topLevelItem(0);
                while ((int)treeItem != sutItemPtr && (treeItem = treeItem->parent())) {
                    objectIdentification.prepend( treeObjectIdentification((int)treeItem, sutItemPtr) + '.');
                }
            }

            switch (action) {

            case appendAction:
            case copyAction:
            case appendPathAction:
            case copyPathAction:
                updateClipboardText( objectIdentification, action == appendAction );
                break;

            case insertAction:
            case insertPathAction:
                emit insertToCodeEditor(objectIdentification, !fullPath, !fullPath);
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
