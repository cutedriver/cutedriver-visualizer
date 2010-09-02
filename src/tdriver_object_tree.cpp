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
#include "tdriver_image_view.h"

#include <tdriver_debug_macros.h>

#include <QTimer>

void MainWindow::collectGeometries( QTreeWidgetItem * item )
{
    //qDebug() << "collectGeometries";
    QStringList geometries;
    collectGeometries( item, geometries );
}


bool MainWindow::getParentItemOffset( QTreeWidgetItem * item, float & x, float & y )
{
    //qDebug() << "getParentItemOffset";
    bool result = false;
    // QTreeWidgetItem * tmpItem = item;

    while ( x == -1 && y == -1 && item != NULL ) {
        // retrieve selected items attributes
        QMap<QString, QHash<QString, QString> > attributes = attributesMap.value( ( int )( item ) );
        x = attributes.value( "x" ).value( "value", "-1" ).toFloat();
        y = attributes.value( "y" ).value( "value", "-1" ).toFloat();
        item = item->parent();
    }

    result = ( x != -1 && y != -1 );
    return result;
}


void MainWindow::collectGeometries( QTreeWidgetItem * item, QStringList & geometries )
{
    //qDebug() << "collectGeometries";
    float x, y, width, height;
    QString geometry;

    if ( item != NULL ) {
        int itemPtr = ( int )( item );

        if ( geometriesMap.contains( itemPtr ) ) {
            // retrieve geometries list from cache
            geometries = geometriesMap.value( itemPtr );
        }
        else {
            // retrieve selected items attributes
            QMap<QString, QHash<QString, QString> > attributes = attributesMap.value( itemPtr );
            geometry = attributes.value( "geometry" ).value( "value" );

            // retrieve x, y: if value not found return -1
            x = attributes.value( "x" ).value( "value", "-1" ).toFloat();
            y = attributes.value( "y" ).value( "value", "-1" ).toFloat();

            // retrieve width, height: if value not found return -1
            width = attributes.value( "width" ).value( "value", "-1" ).toFloat();
            height = attributes.value( "height" ).value( "value", "-1" ).toFloat();

            // if x, y, width, height found add, create geometry of those
            if ( x != -1 && y != -1 && width != -1 && height != -1 ) {
                // create geometry for object
                geometry = QString( QString::number( x ) + "," + QString::number( y ) + "," + QString::number( width )+ "," + QString::number( height )    );
                geometriesMap.insert( itemPtr, QStringList( geometry ) );
            }

            else if ( !geometry.isEmpty() ) {
                // retrieve parent location as offset, looking recursively through parents until an offset is obtained
                if ( getParentItemOffset( item, x, y) ) {
                    QStringList geometryList = geometry.split(",");

                    // create geometry for object
                    geometry = QString(
                            QString::number( geometryList[ 0 ].toFloat() + x ) + "," + QString::number( geometryList[ 1 ].toFloat() + y ) + "," +
                            QString::number( geometryList[ 2 ].toFloat() )+ "," + QString::number( geometryList[ 3 ].toFloat() )
                            );
                    geometriesMap.insert( itemPtr, QStringList( geometry ) );
                }
            }

            // retrieve child nodes geometries
            if ( item->childCount() > 0 ) {
                for ( int childIndex = 0; childIndex < item->childCount(); childIndex++ ) {
                    QStringList childGeometries;
                    collectGeometries( item->child( childIndex ), childGeometries );
                    QStringList tmp = geometriesMap.value( itemPtr );
                    // store current items and its childs geometries to cache
                    geometriesMap.insert( itemPtr, tmp << geometriesMap.value( ( int )( item->child( childIndex ) ) ) );
                }
            }
        }
    }
}



void MainWindow::objectTreeItemChanged()
{
    //qDebug() << "objectTreeItemChanged";
    // if item changed, tabs in properties table willss be updated when activated next time
    propertyTabLastTimeUpdated.clear();
    // update current properties table
    updatePropetriesTable();
    drawHighlight( ( int )( objectTree->currentItem() ) );
}


QTreeWidgetItem * MainWindow::createObjectTreeItem( QTreeWidgetItem * parentItem, QString type, QString name, QString id ) {

    //qDebug() << "createObjectTreeItem";

    QTreeWidgetItem *item = new QTreeWidgetItem( parentItem );

    // if type or id is empty...
    if ( type.isEmpty() ) { type = "<NoName>"; }
    if ( id.isEmpty()   ) { id = "<None>";     }

    item->setData( 0, Qt::DisplayRole, type );
    item->setData( 1, Qt::DisplayRole, name );
    item->setData( 2, Qt::DisplayRole, id   );

    item->setFont( 0, *defaultFont );
    item->setFont( 1, *defaultFont );
    item->setFont( 2, *defaultFont );

    parentItem->addChild( item );

    return item;
}


void MainWindow::storeItemToObjectTreeMap( QTreeWidgetItem *item, QString type, QString name, QString id ) {

    //qDebug() << "storeItemToObjectTreeMap";

    int itemPtr = ( int )( item );

    QHash<QString, QString> treeItemData;

    treeItemData.insert( "type", type );
    treeItemData.insert( "name", name );
    treeItemData.insert( "id",   id   );

    // store object tree data
    objectTreeData.insert( itemPtr, treeItemData );

    // store object tree pointer
    objectTreeMap.insert( itemPtr, item );

}

void MainWindow::buildObjectTree( QTreeWidgetItem *parentItem, QDomElement parentElement )
{
    //qDebug() << "buildObjectTree";

    // create attributes container for each QTreeWidgetItem
    QMap<QString, QHash<QString, QString> > attributeContainer;

    // create attribute hash for each attribute
    QHash<QString, QString> attributeHash;

    QTreeWidgetItem *childItem = 0;
    QDomNode node = parentElement.firstChild();
    int parentPtr = ( int )( parentItem );
    QDomElement element;

    while ( !node.isNull() ) {
        if ( node.isElement() && node.nodeName() == "attributes" ) {
            buildObjectTree( parentItem, node.toElement() );
        }

        if ( node.isElement() && node.nodeName() == "objects" ) {
            buildObjectTree( parentItem, node.toElement() );
        }

        if ( node.isElement() && node.nodeName() == "attribute" ) {
            element = node.toElement();

            // empty attributes container
            attributeContainer.clear();

            // empty attribute hash
            attributeHash.clear();

            // retrieve container if already exists
            if ( attributesMap.contains( parentPtr ) ) { attributeContainer = attributesMap.value( parentPtr ); }

            // store attribute values to attribute hash
            attributeHash[ "name" ] = element.attribute( "name" );
            attributeHash[ "datatype" ] = element.attribute( "dataType" );
            attributeHash[ "type" ] = element.attribute( "type" );
            attributeHash[ "value" ] = element.elementsByTagName( "value" ).item( 0 ).toElement().text();

            // insert attribute hash to attribute container
            attributeContainer.insert( element.attribute( "name" ).toLower(), attributeHash );

            // insert container to attributesMap
            attributesMap.insert( parentPtr, attributeContainer );
        }

        if ( node.isElement() && node.nodeName() == "object" ) {
            element = node.toElement();
            QString type = element.attribute( "type" );
            QString name = element.attribute( "name" );
            QString id   = element.attribute( "id"   );

            // store id of current application ui dump
            if ( type.toLower() == "application" ) {
                currentApplication.clear();
                currentApplication.insert( "name", name );
                currentApplication.insert( "id", id );
            }

            // create child item
            childItem = createObjectTreeItem( parentItem, type, name, id );
            int childPtr = ( int )( childItem );

            // iterate the node recursively if child nodes exists
            if ( node.hasChildNodes() ) { buildObjectTree( childItem, element ); }

            // store current item data to object tree map
            storeItemToObjectTreeMap( childItem, type, name, id );

            // is visible?
            if ( attributesMap.contains( childPtr ) ) {
                attributeContainer = attributesMap.value( childPtr );

                if ( attributeContainer.value( "visible" ).value( "value" ).toLower() == "true"  ||
                     attributeContainer.value( "isvisible" ).value( "value" ).toLower() == "true" ||
                     attributeContainer.value( "iswindow" ).value( "value" ).toLower() == "true" )
                {
                    // sometimes object has visible and visibleOnScreen attribute, make sure that object is really visible also on screen
                    if ( attributeContainer.value("visibleonscreen").value("value").toLower() != "false" ) {
                        visibleObjectsList << childPtr;
                    }
                }
            }
        } // end if node is element and node name is "object"

        node = node.nextSibling();
    } // end while node not null
}


void MainWindow::clearObjectTreeMappings()
{
    // empty visible objects list
    visibleObjectsList.clear();

    // empty attributes of each object tree item
    attributesMap.clear();

    // empty geometry values of each object tree item
    geometriesMap.clear();

    // empty status of last updated properties table tab
    propertyTabLastTimeUpdated.clear();

    // empty pointer mappings to object tree items
    objectTreeMap.clear();

    // empty object tree data mappings (eg. type, name & id)
    objectTreeData.clear();
}


void MainWindow::updateObjectTree( QString filename )
{
    //qDebug() << "updateObjectTree";
    QTreeWidgetItem *sutItem  = new QTreeWidgetItem( 0 );

    // QDomDocument xmlDocument;
    QDomElement element;
    QDomNode node;

    // store id value of focused node in object tree
    QString currentFocusId = objectTreeData.value( ( int )( objectTree->currentItem() ) ).value( "id" );

    clearObjectTreeMappings();
    // empty object tree
    objectTree->clear();

    // parse ui dump xml
    parseXml( filename, xmlDocument );

    //qDebug() << "xmlDocument ptr: " << (int)(&xmlDocument);

    if ( !xmlDocument.isNull() ) {
        node = xmlDocument.documentElement().firstChild();

        while ( !node.isNull() ) {
            // find tasInfo element from xmlDocument
            if ( node.isElement() && node.nodeName() == "tasInfo" ) {
                element = node.toElement();

                // add sut to the top of the object tree
                sutItem->setData( 0, Qt::DisplayRole, "sut" );
                sutItem->setData( 1, Qt::DisplayRole, element.attribute( "name" ) );
                sutItem->setData( 2, Qt::DisplayRole, element.attribute( "id" ) );

                sutItem->setFont( 0, *defaultFont );
                sutItem->setFont( 1, *defaultFont );
                sutItem->setFont( 2, *defaultFont );

                objectTree->addTopLevelItem ( sutItem );

                int itemPtr = ( int )( sutItem );

                QHash<QString, QString> treeItemData;

                treeItemData.insert( "type", "sut" );
                treeItemData.insert( "name", element.attribute( "name" ) );
                treeItemData.insert( "id",   element.attribute( "id" )   );

                // store object tree data
                objectTreeData.insert( itemPtr, treeItemData );

                // build object tree with xml
                buildObjectTree( sutItem, element );

                break; // while node is not null
            }

            node = node.nextSibling();
        }
    }

    // collect geometries for item and its childs
    collectGeometries( sutItem );

    bool itemFocusChanged = false;

    // restore focus if object is still visible/available
    if ( !currentFocusId.isEmpty() ) {

        QMap<int, QHash<QString, QString> >::const_iterator objectTreeIterator;

        for ( objectTreeIterator = objectTreeData.constBegin(); objectTreeIterator != objectTreeData.constEnd(); ++objectTreeIterator ) {

            if ( objectTreeIterator.value().value( "id" ) == currentFocusId ) {
                // select item that was focused before refresh
                objectTree->setCurrentItem( objectTreeMap.value( objectTreeIterator.key() ) );
                itemFocusChanged = true;
                // item found, no need to iterate the list
                break;
            }
        }
    }

    // set focus to SUT item unless previously focused item visible
    if ( !itemFocusChanged ) { objectTree->setCurrentItem( sutItem ); }

    // highlight current object
    drawHighlight( (int)( objectTree->currentItem() ) );
    updatePropetriesTable();
}



void MainWindow::connectObjectTreeSignals()
{
    // Item select - command
    QObject::connect( objectTree, SIGNAL( itemPressed ( QTreeWidgetItem *, int )), this, SLOT(objectViewItemClicked ( QTreeWidgetItem *, int )));
    QObject::connect( objectTree, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT( objectViewCurrentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * ) ) );

    // Item expand/collapse
    QObject::connect( objectTree, SIGNAL( itemExpanded( QTreeWidgetItem* ) ), this, SLOT( expandTreeWidgetItem( QTreeWidgetItem* ) ) );
    QObject::connect( objectTree, SIGNAL( itemCollapsed( QTreeWidgetItem* ) ), this, SLOT( collapseTreeWidgetItem( QTreeWidgetItem* ) ) );
}


void MainWindow::delayedRefreshData()
{
    if  ( !isDeviceSelected() ) {
        noDeviceSelectedPopup();
    }
    else {
        delayedRefreshButton->setDisabled(true);
        delayedRefreshButton->setDown(true);
        refreshButton->setDisabled(true);
        refreshButton->setDown(true);
        QTimer::singleShot(5000, this, SLOT(forceRefreshData()));
    }
}


void MainWindow::forceRefreshData()
{
    if  ( !isDeviceSelected() ) {
        noDeviceSelectedPopup();
    }
    else {
        delayedRefreshButton->setDisabled(false);
        delayedRefreshButton->setDown(false);
        refreshButton->setDisabled(false);
        refreshButton->setDown(false);
        refreshData();
    }
}


void MainWindow::refreshData()
{
    bool giveup = false;
    bool isS60 = false;
    bool listAppsOk = false;
    bool refreshUiOk = false;
    bool refreshImageOk = false;
    bool behavioursOk = false;
    BAListMap listAppsReply;
    BAListMap dummyReply;
    QString filenamePrefix = outputPath + "/visualizer_dump_" + activeDevice.value( "name" );

    QTime t;
    t.start();

    // request application list (unless s60)
    if ( activeDevice.value( "type" ).toLower() == "s60" ) {
        resetApplicationsList();
        appsMenu->setDisabled( true );
        isS60 = true;
    }
    else {
        QString listCommand = QString( activeDevice.value( "name" ) + " list_apps" );
        statusbar( "Refreshing application list...", 0);
        qDebug() << FCFL << "app list refresh started at" << float(t.elapsed())/1000.0;
        listAppsOk = executeTDriverCommand( commandListApps, listCommand, "", &listAppsReply );
    }

    if (listAppsOk) {
        qDebug() << FCFL << "app list parse started at" << float(t.elapsed())/1000.0;
        statusbar( "Updating applications list...", 0);
        parseApplicationsXml( outputPath + "/visualizer_applications_" + activeDevice.value( "name" ) + ".xml" );
    }
    else giveup = true;

    // use target application if user has chosen one
    QString refreshCmdTemplate = activeDevice.value( "name" ) + " %1";
    if ( !currentApplication.value( "id" ).isEmpty() && applicationsHash.contains( currentApplication.value( "id" ) ) ) {
        refreshCmdTemplate += " " + currentApplication.value( "id" );
    }

    // request ui xml dump
    if (!giveup) {
        statusbar( "Getting UI XML data...", 0);
        qDebug() << FCFL << "xml refresh started at" << float(t.elapsed())/1000.0;
        refreshUiOk = executeTDriverCommand( commandRefreshUI, refreshCmdTemplate.arg("refresh_ui"), "", &dummyReply );
        if (!refreshUiOk) giveup = true;
    }

    // request screen capture image
    if (!giveup) {
        statusbar( "Getting UI image data...", 0);
        qDebug() << FCFL << "image refresh started at" << float(t.elapsed())/1000.0;
        refreshImageOk = executeTDriverCommand( commandRefreshImage, refreshCmdTemplate.arg("refresh_image"), "", &dummyReply );
    }

    if (refreshImageOk) {
        qDebug() << FCFL << "image load started at" << float(t.elapsed())/1000.0;
        statusbar( "Loading screen capture image...", 0);
        imageWidget->disableDrawHighlight();
        imageWidget->refreshImage( QString( filenamePrefix + ".png"), false );
        imageWidget->repaint();
    }

    if (refreshUiOk) {
        qDebug() << FCFL << "object tree update / ui xml parse started at" << float(t.elapsed())/1000.0;
        statusbar( "Updating object tree...", 0);
        updateObjectTree( filenamePrefix + ".xml" );
        qDebug() << FCFL << "behaviour update started at" << float(t.elapsed())/1000.0;
        statusbar( "Updating behaviours...", 0);
        behavioursOk = updateBehaviourXml();
        qDebug() << FCFL << "properties and title update started at" << float(t.elapsed())/1000.0;
        statusbar( "Almost done...", 0);
        updatePropetriesTable();
        updateWindowTitle();
    }

    qDebug() << FCFL << "done (listAppsOk" << listAppsOk << "refreshUiOk" << refreshUiOk << "refreshImageOk" << refreshImageOk << "behavioursOk" << behavioursOk << ") at" << float(t.elapsed())/1000.0;

    if ((listAppsOk || isS60) && refreshUiOk) statusbar( "Refresh done!", 1500 );
    else if (refreshUiOk) statusbar( "Refreshed only UI XML data!", 1500 );
    else if (listAppsOk) statusbar( "Refreshed only Application List!", 1500 );
    else statusbar( "Refresh failed!", 1500 );
}


// Function to refresh visible data - creates path to xml file (located in same directory as tdriver_visualizer) and loads it.
void MainWindow::refreshDataDisplay()
{
    // disable all current highlightings
    imageWidget->disableDrawHighlight();
    //load XMLfile and populate tree
    updateObjectTree( outputPath + "/visualizer_dump_" + activeDevice.value( "name" ) + ".xml" );
    updateBehaviourXml();
    updatePropetriesTable();
}


void MainWindow::resizeObjectTree() {

    int old_width = objectTree->columnWidth( objectTree->currentColumn() );

    objectTree->resizeColumnToContents( objectTree->currentColumn() );

    if ( objectTree->columnWidth( objectTree->currentColumn() ) < old_width ) {
        // column width smaller after resize --> restore to previous width
        objectTree->setColumnWidth( objectTree->currentColumn(), old_width );

    } else {
        // add some padding
        objectTree->setColumnWidth( objectTree->currentColumn(), objectTree->columnWidth( objectTree->currentColumn() ) + 25 );
    }
}


void MainWindow::objectViewCurrentItemChanged ( QTreeWidgetItem * itemCurrent, QTreeWidgetItem * /*itemPrevious*/ )
{
    Q_UNUSED( itemCurrent );
    // qDebug() << "objectViewCurrentItemChanged";

    collapsedObjectTreeItemPtr = 0;
    expandedObjectTreeItemPtr = 0;
    objectTreeItemChanged();
    imageWidget->update();
    resizeObjectTree();
}


QString MainWindow::treeObjectIdentification(int treeItemPtr, int sutItemPtr)
{
    QHash<QString, QString> treeItemData = objectTreeData.value( treeItemPtr );
    QString objIdentification = treeItemData.value("type");
    QString objName = treeItemData.value("name");
    QString objText = attributesMap.value( treeItemPtr ).value("text").value("value");

    if ( sutItemPtr == treeItemPtr && objIdentification == "sut" ) {
        objIdentification = tr( "TDriver.sut( :Id => '" ) + activeDevice.value( "name" ) + tr( "' )" );
    } else if ( objName != "NoName" && !objName.isEmpty() ) {
        objIdentification.append("( :name => '" + objName + "' )");
    } else if(objText != "" && !objText.isEmpty()) {
        objIdentification.append("( :text => '" + attributesMap.value( treeItemPtr ).value("text").value("value") + "' )");
    }
    return objIdentification;
}


void MainWindow::objectViewItemAction( QTreeWidgetItem * item, int column, ContextMenuSelection action, QString method ) {

    Q_UNUSED( column );

    if ( action > cancelAction ) {

        int sutItemPtr = ( int )objectTree->topLevelItem(0);
        const bool fullPath = ((int)item == sutItemPtr) || isPathAction(action) ;

        // TODO: use XPath to determine if object is unique, and eg. insert line in comments if it's not unique
        QString result;

        do {
            if (!result.isEmpty()) result.prepend('.');
            result.prepend( treeObjectIdentification((int)item, sutItemPtr) );
        } while (fullPath && (int)item != sutItemPtr && (item = item->parent()));

        switch (action) {

        case appendAction:
        case copyAction:
        case appendPathAction:
        case copyPathAction:
            updateClipboardText( result, action == appendAction );
            break;

        case insertAction:
        case insertPathAction:
            if (!method.isEmpty()) {
                result = result + '.' + method + '\n';
            }
            emit insertToCodeEditor(result, !fullPath, !fullPath);
            break;

        default:
            qDebug() << __FILE__ << __FUNCTION__ << __LINE__ << "Unhandled context menu action" << action;
        }
    }
}

void MainWindow::objectViewItemClicked( QTreeWidgetItem * item, int column) {

    // if right mouse button pressed open "copy/append to clipboard" dialog
    if ( QApplication::mouseButtons() == Qt::RightButton ) {

        ContextMenuSelection action = showCopyAppendContextMenu();

        objectViewItemAction(item, column, action);
    }
}

// Store last collapsed objectTreeWidgetItem - Note: value will be set to NULL when focus is changed
void MainWindow::collapseTreeWidgetItem( QTreeWidgetItem * item ) {

    collapsedObjectTreeItemPtr = ( int )( item );
    expandedObjectTreeItemPtr = 0;

    resizeObjectTree();

}

// Store last expanded objectTreeWidgetItem - Note: value will be set to NULL when focus is changed
void MainWindow::expandTreeWidgetItem(QTreeWidgetItem * item) {

    collapsedObjectTreeItemPtr = 0;
    expandedObjectTreeItemPtr = ( int )( item );

    resizeObjectTree();

}

void MainWindow::objectTreeExpandAll() {

    int currentItem = ( int )( objectTree->currentItem() );

    // exit if object tree is empty
    if ( currentItem == 0 ) { return; }

    objectTree->expandAll();
    objectTree->scrollToItem( objectTree->currentItem() );

}

void MainWindow::objectTreeCollapseAll() {

    int currentItem = ( int )( objectTree->currentItem() );

    // exit if object tree is empty
    if ( currentItem == 0 ) { return; }

    objectTree->collapseAll();
    objectTree->setCurrentItem( objectTree->topLevelItem( 0 ) );
    objectTree->scrollToItem( objectTree->currentItem() );

}

void MainWindow::objectTreeKeyPressEvent( QKeyEvent * event ) {

    int currentItem = ( int )( objectTree->currentItem() );

    // exit if object tree is empty
    if ( currentItem == 0 ) { return; }

    if ( event->modifiers() == Qt::ControlModifier ) {

        if ( event->key() == Qt::Key_Right ) {

            objectTreeExpandAll();

        } else if ( event->key() == Qt::Key_Left ) {

            objectTreeCollapseAll();
        }

    } else if ( event->key() == Qt::Key_Right ) {

        if ( objectTree->currentItem()->childCount() > 0 ) {

            // if item is exapanded and childs available, go to first child
            if ( expandedObjectTreeItemPtr != 0 || expandedObjectTreeItemPtr != currentItem ) {

                objectTree->setCurrentItem( objectTree->currentItem()->child( 0 ) );

            }

        } else {

            int selectItem = objectTree->currentItem()->parent()->childCount() - 1;

            for( int iter = objectTree->currentItem()->parent()->indexOfChild( objectTree->currentItem() ); iter < objectTree->currentItem()->parent()->childCount(); iter++ ) {

                // go to next item that has childs
                if ( objectTree->currentItem()->parent()->child( iter )->childCount() > 0 ) { selectItem = iter; break;    }

            }

            objectTree->setCurrentItem( objectTree->currentItem()->parent()->child( selectItem ) );

        }

    } else if (event->key() == Qt::Key_Left) {

        if ( objectTree->currentItem()->parent() != NULL ) {

            // if item did not collapse, just to parent
            if ( collapsedObjectTreeItemPtr != 0 || collapsedObjectTreeItemPtr != currentItem ) {

                objectTree->setCurrentItem( objectTree->currentItem()->parent() );

            }

        }

    }

}




