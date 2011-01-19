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
#include <tdriver_util.h>

#include <tdriver_debug_macros.h>

#include <QTimer>
#include <QProgressDialog>


QPoint MainWindow::getItemPos(QTreeWidgetItem *item)
{
    const QMap<QString, QHash<QString, QString> > &attributes = attributesMap.value( ptr2TestObjectKey( item ) );
    QPoint ret;

    if (sutName.toLower() == "symbian" && objectTreeData.value(ptr2TestObjectKey( item )).env.toLower() == "qt") {

        ret = QPoint(attributes.value("x_absolute").value("value", "-1").toInt(),
                      attributes.value("y_absolute").value("value", "-1").toInt());
    }
    else {

        ret = QPoint(attributes.value("x").value("value", "-1").toInt(),
                      attributes.value("y").value("value", "-1").toInt());
    }

    return ret;
}


bool MainWindow::getParentItemOffset( QTreeWidgetItem * item, int & x, int & y )
{
    //qDebug() << "getParentItemOffset";
    bool result = false;
    // QTreeWidgetItem * tmpItem = item;

    while ( x == -1 && y == -1 && item != NULL ) {
        // retrieve selected items attributes
        QPoint pos = getItemPos(item);
        x = pos.x();
        y = pos.y();
        item = item->parent();
    }

    result = ( x != -1 && y != -1 );
    return result;
}




void MainWindow::collectGeometries( QTreeWidgetItem * item, RectList & geometries)
{
    //qDebug() << "collectGeometries";

    if ( item != NULL ) {
        TestObjectKey itemPtr = ptr2TestObjectKey( item );

        if ( geometriesMap.contains( itemPtr ) ) {
            // retrieve geometries list from cache
            geometries = geometriesMap.value( itemPtr );
        }
        else {

            geometries.clear();
            // retrieve child nodes geometries first
            for ( int childIndex = 0; childIndex < item->childCount(); childIndex++ ) {
                RectList childGeometries;
                collectGeometries( item->child( childIndex ), childGeometries);
                geometries << childGeometries;
            }

            // retrieve selected items attributes
            QMap<QString, QHash<QString, QString> > attributes = attributesMap.value( itemPtr );

            bool ok = true;

            // retrieve x, y, widht height, or ok=false if fail
            int x, y, width, height;
            QPoint pos = getItemPos(item);
            x = pos.x();
            y = pos.y();
            if (ok) width = attributes.value( "width" ).value( "value").toInt(&ok);
            if (ok) height = attributes.value( "height" ).value( "value").toInt(&ok);

            if ( ok ) {
                // use values from separate attributes
                geometries.prepend(QRect(x, y, width, height));
            }
            else {
                // parse values from geometry attribute
                const QString &geometry = attributes.value( "geometry" ).value( "value" );
                QStringList geometryList = geometry.split(',');

                if ( geometryList.size() >= 4) {
                    x = geometryList.at(0).toInt(&ok);
                    if (ok) y = geometryList.at(1).toInt(&ok);
                    if (ok) width = geometryList.at(2).toInt(&ok);
                    if (ok) height = geometryList.at(3).toInt(&ok);

                    if (ok) {
                        // retrieve parent location as offset, looping down the tree for correct offset
                        int px=-1, py=-1;
                        ok = getParentItemOffset( item, px, py);
                        if (ok) geometries.prepend(QRect(px+x, py+y, width, height));
                    }
                }
            }

            if (!ok) {
                // not ok, prepend Null rectangle
                geometries.prepend(QRect());
            }

            geometriesMap.insert( itemPtr, geometries);
        }
    }
}



void MainWindow::objectTreeItemChanged()
{
    //qDebug() << "objectTreeItemChanged";
    // if item changed, tabs in properties table willss be updated when activated next time
    propertyTabLastTimeUpdated.clear();
    // update current properties table
    updatePropertiesTable();
    drawHighlight( ptr2TestObjectKey(objectTree->currentItem()), true );
}


QTreeWidgetItem * MainWindow::createObjectTreeItem( QTreeWidgetItem *parentItem, const TreeItemInfo &data)
{

    //qDebug() << "createObjectTreeItem";

    QTreeWidgetItem *item = new QTreeWidgetItem( parentItem );

    // if type or id is empty...
    QString type = data.type;
    QString id = data.id;
    if ( type.isEmpty() ) { type = "<NoName>"; }
    if ( id.isEmpty()   ) { id = "<None>";     }

    item->setData( 0, Qt::DisplayRole, type);
    if (!data.env.isEmpty())
        item->setData( 0, Qt::ToolTipRole, data.env);
    item->setData( 1, Qt::DisplayRole, data.name);
    item->setData( 2, Qt::DisplayRole, id);

    item->setFont( 0, *defaultFont );
    item->setFont( 1, *defaultFont );
    item->setFont( 2, *defaultFont );

    parentItem->addChild( item );

    return item;
}


void MainWindow::storeItemToObjectTreeMap( QTreeWidgetItem *item, const TreeItemInfo &data)
{
    TestObjectKey itemPtr = ptr2TestObjectKey( item );

    // store object tree data
    objectTreeData.insert( itemPtr, data );
    objectIdMap.insert(data.id, itemPtr);
}

void MainWindow::refreshScreenshotObjectList()
{
    screenshotObjects.clear();

    if (imageWidget) {
        // collect geometries for item and its childs
        buildScreenshotObjectList();

        imageWidget->update();
    }
}


/* Recursive function.
   First call from outside should omit arguments, so default value for parentKey is used.
*/
void MainWindow::buildScreenshotObjectList(TestObjectKey parentKey)
{
    if (!parentKey) {
        // first call
        QString id = imageWidget->tasIdString();
        if (id.isEmpty()) {
            // image metadata didn't have id, so find first object included in attributesMap
            QTreeWidgetItem *item = objectTree->invisibleRootItem();
            parentKey = ptr2TestObjectKey(item);

            while (item) {
                if (attributesMap.contains(parentKey)) break; // found!
                item = item->child(0);
                parentKey = ptr2TestObjectKey(item);
            }
        }
        else {
            // get parent based on id received in image metadata
            parentKey = objectIdMap.value(id);
        }
    }
    // check validity
    if ( parentKey && attributesMap.contains(parentKey) ) {

        const QMap<QString, QHash<QString, QString> > &attributeContainer = attributesMap.value(parentKey);

        QPoint pos = getItemPos(testObjectKey2Ptr(parentKey));
        // assuming that if height and width attributes are present, then getItemPos returned a reasonable pos

        bool ok = (( attributeContainer.contains("height") && attributeContainer.contains("width") )
                   || attributeContainer.contains("geometry"));

        if (ok && 0 == attributeContainer.value( "visible" ).value( "value" ).compare("false", Qt::CaseInsensitive))
            ok = false;

        // isVisible is only used by AVKON traverser
        if (ok && 0 == attributeContainer.value( "isvisible" ).value( "value" ).compare("false", Qt::CaseInsensitive))
            ok = false;

        /* no need to care if object is obscured, highlight should be drawn anyway to show position
        if (ok && 0 == attributeContainer.value( "visibleonscreen" ).value( "value" ).compare("false", Qt::CaseInsensitive))
            ok = false;
        */

        if (ok) {
            screenshotObjects << parentKey;
        }

        // recurse into all children
        QTreeWidgetItem *parentItem = testObjectKey2Ptr(parentKey);
        for (int ii=0; ii < parentItem->childCount(); ++ii) {
            buildScreenshotObjectList(ptr2TestObjectKey(parentItem->child(ii)));
        }
    }
}


void MainWindow::buildObjectTree( QTreeWidgetItem *parentItem, QDomElement parentElement )
{
    //qDebug() << "buildObjectTree";

    // create attribute hash for each attribute
    QHash<QString, QString> attributeHash;

    QTreeWidgetItem *childItem = 0;
    QDomNode node = parentElement.firstChild();
    TestObjectKey parentPtr = ptr2TestObjectKey( parentItem );
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

            // empty attribute hash
            attributeHash.clear();

            // create attributes container for each QTreeWidgetItem
            QMap<QString, QHash<QString, QString> > attributeContainer;

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
            TreeItemInfo data = {
                element.attribute( "type" ),
                element.attribute( "name" ),
                element.attribute( "id" ),
                element.attribute( "env" ) };

            // store id of current application ui dump
            if ( data.type.compare("application", Qt::CaseInsensitive )==0 ) {
                qDebug() << FCFL << "got application id" << data.id << "name" << data.name;
                currentApplication.set(data.id, data.name);
            }

            childItem = createObjectTreeItem( parentItem, data);
            storeItemToObjectTreeMap( childItem, data );

            // iterate the node recursively if child nodes exists
            if ( node.hasChildNodes() ) {
                buildObjectTree( childItem, element );
            }


        } // end if node is element and node name is "object"

        node = node.nextSibling();
    } // end while node not null
}


void MainWindow::clearObjectTreeMappings()
{
    // empty visible objects list
    screenshotObjects.clear();

    // empty attributes of each object tree item
    attributesMap.clear();

    // empty geometry values of each object tree item
    geometriesMap.clear();

    // empty status of last updated properties table tab
    propertyTabLastTimeUpdated.clear();

    // empty object tree data mappings (eg. type, name & id)
    objectTreeData.clear();
    objectIdMap.clear();
}


void MainWindow::updateObjectTree( QString filename )
{
    //qDebug() << "updateObjectTree";
    QTreeWidgetItem *sutItem  = NULL;

    // store id value of focused node in object tree
    QString currentFocusId = objectTreeData.value(ptr2TestObjectKey( objectTree->currentItem())).id;

    clearObjectTreeMappings();
    // empty object tree
    objectTree->clear();
    uiDumpFileName.clear();
    sutName.clear();

    // parse ui dump xml
    if (parseXml( filename, xmlDocument )) {
        uiDumpFileName = filename;
        QDomNode node = xmlDocument.documentElement().firstChild();

        while ( !node.isNull() ) {
            // find tasInfo element from xmlDocument
            if ( node.isElement() && node.nodeName() == "tasInfo" ) {
                if (sutItem) {
                    qWarning("%s:%i: Duplicate tasInfo element, ignoring remaining XML!", __FILE__, __LINE__);
                    break;
                }
                sutItem = new QTreeWidgetItem(0);

                QDomElement element = node.toElement();
                QString sutId = element.attribute( "id" );

                sutName = element.attribute("name");

                // add sut to the top of the object tree
                sutItem->setData( 0, Qt::DisplayRole, QString("sut") );
                sutItem->setData( 1, Qt::DisplayRole, sutName );
                sutItem->setData( 2, Qt::DisplayRole, sutId );

                sutItem->setFont( 0, *defaultFont );
                sutItem->setFont( 1, *defaultFont );
                sutItem->setFont( 2, *defaultFont );

                objectTree->addTopLevelItem ( sutItem );
                objectIdMap.insert(sutId, ptr2TestObjectKey(sutItem));

                TestObjectKey itemPtr = ptr2TestObjectKey( sutItem );

                TreeItemInfo treeItemData = {
                    QString("sut"),
                    sutName,
                    element.attribute("id"),
                    element.attribute("env") };

                // store object tree data
                objectTreeData.insert( itemPtr, treeItemData );

                // build object tree with xml
                buildObjectTree( sutItem, element );

                break; // while node is not null
            }

            node = node.nextSibling();
        }
    }

    if (!sutItem) {
        qWarning("%s:%i: got no tasInfo elements from XML, returning from method", __FILE__, __LINE__);
        return;
    }

    RectList dummy;
    collectGeometries(sutItem, dummy);
    refreshScreenshotObjectList();

    bool itemFocusChanged = false;

    // restore focus if object is still visible/available
    if ( !currentFocusId.isEmpty() ) {

        TestObjectKey currentFocusKey = objectIdMap.value(currentFocusId);

        if (currentFocusKey) {
            objectTree->setCurrentItem(testObjectKey2Ptr(currentFocusKey));
            itemFocusChanged = true;
        }
    }

    // set focus to SUT item unless previously focused item visible
    if ( !itemFocusChanged ) {
        objectTree->setCurrentItem( sutItem );
    }

    // highlight current object
    drawHighlight( ptr2TestObjectKey(objectTree->currentItem()), true );
    updatePropertiesTable();
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

        delayedRefreshAction->setDisabled(true);
        refreshAction->setDisabled(true);
        QTimer::singleShot(5000, this, SLOT(forceRefreshData()));
    }
}


void MainWindow::forceRefreshData()
{
    if  ( !isDeviceSelected() ) {
        noDeviceSelectedPopup();
    }
    else {
        delayedRefreshAction->setDisabled(false);
        refreshAction->setDisabled(false);
        refreshData();
    }
}


static void doProgress(QProgressDialog *progress, const QString &prefix, const QString &arg, int value)
{
    if (!prefix.isNull()) {
        if (!arg.isNull()) {
            progress->setLabelText(prefix.arg(arg));
        }
        else {
            progress->setLabelText(prefix);
        }
    }
    progress->setValue(value);
    progress->show();
    progress->repaint();
}


void MainWindow::refreshData()
{
    bool canceled = false;
    bool giveup = false;
    bool isS60 = false;
    bool listAppsOk = false;
    QString refreshUiFileName;
    bool refreshImageOk = false;
    bool behavioursOk = false;
    BAListMap reply;
    QString progressTemplate = tr("Refreshing: %1...");
    QString refreshCmdTemplate = activeDevice.value( "name" ) + " %1";

    QProgressDialog *progress = new QProgressDialog("Visualizer Progress Dialog", tr("Cancel"), 0, 100, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setAutoReset(false);
    progress->setAutoClose(true);
    progress->setMinimumDuration(0);
    progress->setCancelButtonText(QString()); // hide cancle button here, because it seems to work badly

    QTime t;
    t.start();

    // purpose of the doProgress below is to force QProgress dialog window to be wide enough to not need resize
    doProgress(progress, QString(50, '_'), QString(), 0);

    // request application list (unless s60 AVKON)
    if ( activeDevice.value( "type" ).toLower() == "s60" ) {
        resetApplicationsList();
        appsMenu->setDisabled( true );
        foregroundApplication = true;
        isS60 = true;
    }
    else {
        QString listCommand = QString( activeDevice.value( "name" ) + " list_apps" );
        doProgress(progress, progressTemplate, tr("requesting application list"), 1);
        statusbar( "Refreshing application list...", 0);
        qDebug() << FCFL << "app list refresh started at" << float(t.elapsed())/1000.0;
        listAppsOk = executeTDriverCommand( commandListApps, listCommand, "", &reply );
        if (!listAppsOk) giveup = true;
    }
    //canceled = progress->wasCanceled();

    if (!giveup && !canceled && !isS60 && listAppsOk) {
        qDebug() << FCFL << "app list parse started at" << float(t.elapsed())/1000.0;
        doProgress(progress, progressTemplate, tr("parsing application list"), 20);
        statusbar( "Updating applications list...", 0);
        parseApplicationsXml( reply.value("applications_filename").value(0) );
    }
    //canceled = progress->wasCanceled();

    // use target application if user has chosen one
    if (foregroundApplication) {
        qDebug() << FCFL << "---------------------- Refreshing foreground app";
    }
    else if (!currentApplication.isNull() && applicationsNamesMap.contains( currentApplication.id )) {
        qDebug() << FCFL << "---------------------- Refreshing current application, id:" << currentApplication.id;
        refreshCmdTemplate += " " + currentApplication.id;
    }
    else {
        qWarning("Current application not set and foregroundApplication false! Refreshing foreground application.");
    }

    // request ui xml dump
    if (!giveup && !canceled) {
        doProgress(progress, progressTemplate, tr("requesting UI XML data"), 25);
        statusbar( "Getting UI XML data...", 0);
        qDebug() << FCFL << "xml refresh started at" << float(t.elapsed())/1000.0;
        if (executeTDriverCommand( commandRefreshUI, refreshCmdTemplate.arg("refresh_ui"), "", &reply ))
            refreshUiFileName = reply.value("ui_filename").value(0);
        else
            giveup = true;
    }
    //canceled = progress->wasCanceled();

    // request screen capture image
    if (!giveup && !canceled) {
        doProgress(progress, progressTemplate, tr("getting screen capture image"), 65);
        statusbar( "Getting UI image data...", 0);
        qDebug() << FCFL << "image refresh started at" << float(t.elapsed())/1000.0;
        refreshImageOk = executeTDriverCommand( commandRefreshImage, refreshCmdTemplate.arg("refresh_image"), "", &reply );
    }
    //progress->setCancelButtonText(QString()); // hide cancle button here
    //canceled = progress->wasCanceled();

    if (!giveup && !canceled && refreshImageOk) {
        qDebug() << FCFL << "image load started at" << float(t.elapsed())/1000.0;
        doProgress(progress, progressTemplate, tr("loading screen capture image"), 90);
        statusbar( "Loading screen capture image...", 0);
        imageWidget->disableDrawHighlight();
        imageWidget->refreshImage( reply.value("image_filename").value(0));
        imageWidget->repaint();
    }

    if (!giveup && !canceled && !refreshUiFileName.isEmpty()) {

        doProgress(progress, progressTemplate, tr("parsing UI XML data"), 95);
        statusbar( "Updating object tree...", 0);
        qDebug() << FCFL << "object tree update / ui xml parse started at" << float(t.elapsed())/1000.0;
        updateObjectTree( refreshUiFileName );

        qDebug() << FCFL << "behaviour update started at" << float(t.elapsed())/1000.0;
        statusbar( "Updating behaviours...", 0);
        behavioursOk = updateBehaviourXml();

        qDebug() << FCFL << "properties and title update started at" << float(t.elapsed())/1000.0;
        statusbar( "Almost done...", 0);
        updatePropertiesTable();
        updateWindowTitle();
    }

    qDebug() << FCFL << "done (listAppsOk" << listAppsOk << "refreshUiFileName" << refreshUiFileName << "refreshImageOk" << refreshImageOk << "behavioursOk" << behavioursOk << ") at" << float(t.elapsed())/1000.0;
    progress->reset();

    /*if (canceled) statusbar( "Refresh canceled!", 1500 );
    else*/ if ((listAppsOk || isS60) && !refreshUiFileName.isEmpty()) statusbar( "Refresh done!", 1500 );
    else if (!refreshUiFileName.isEmpty()) statusbar( "Refreshed only UI XML data!", 1500 );
    else if (listAppsOk) statusbar( "Refreshed only Application List!", 1500 );
    else statusbar( "Refresh failed!", 1500 );

    if (progress) delete progress;
}


// Function to refresh visible data - creates path to xml file and loads it.
void MainWindow::refreshDataDisplay()
{
    // disable all current highlightings
    imageWidget->disableDrawHighlight();
    //load XMLfile and populate tree
    updateObjectTree( uiDumpFileName);
    updateBehaviourXml();
    updatePropertiesTable();
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


QString MainWindow::treeObjectRubyId(TestObjectKey treeItemPtr, TestObjectKey sutItemPtr)
{
    const TreeItemInfo &treeItemData = objectTreeData.value( treeItemPtr );
    QString objRubyId = treeItemData.type;
    QString objName = treeItemData.name;
    QString objText = attributesMap.value( treeItemPtr ).value("text").value("value");

    if ( sutItemPtr == treeItemPtr && objRubyId == "sut" ) {
        objRubyId = tr( "TDriver.sut( :Id => '" ) + activeDevice.value( "name" ) + tr( "' )" );
    }
    else if ( objName != "NoName" && !objName.isEmpty() ) {
        objRubyId.append("( :name => '" + objName + "' )");
    }
    else if(objText != "" && !objText.isEmpty()) {
        objRubyId.append("( :text => '" + attributesMap.value( treeItemPtr ).value("text").value("value") + "' )");
    }
    else {
        objRubyId.append("( :name => '' )");
    }
    return objRubyId;
}


void MainWindow::objectViewItemAction( QTreeWidgetItem * item, int column, ContextMenuSelection action, QString method ) {

    Q_UNUSED( column );

    if ( action > cancelAction ) {

        TestObjectKey sutItemPtr = ptr2TestObjectKey(objectTree->topLevelItem(0));
        const bool fullPath = (ptr2TestObjectKey(item) == sutItemPtr) || isPathAction(action) ;

        // TODO: use XPath to determine if object is unique, and eg. insert line in comments if it's not unique
        QString result;

        do {
            result = TDriverUtil::smartJoin(
                        treeObjectRubyId(ptr2TestObjectKey(item), sutItemPtr), '.', result);
        } while (fullPath && ptr2TestObjectKey(item) != sutItemPtr && (item = item->parent()));

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
                result = TDriverUtil::smartJoin( result, '.', method) + '\n';
            }
            emit insertToCodeEditor(result, !fullPath, !fullPath);
            // else pop up a warnig dialog?
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

    collapsedObjectTreeItemPtr = ptr2TestObjectKey( item );
    expandedObjectTreeItemPtr = 0;

    resizeObjectTree();

}

// Store last expanded objectTreeWidgetItem - Note: value will be set to NULL when focus is changed
void MainWindow::expandTreeWidgetItem(QTreeWidgetItem * item) {

    collapsedObjectTreeItemPtr = 0;
    expandedObjectTreeItemPtr = ptr2TestObjectKey( item );

    resizeObjectTree();
}

void MainWindow::objectTreeExpandAll() {

    TestObjectKey currentItem = ptr2TestObjectKey( objectTree->currentItem() );

    // exit if object tree is empty
    if ( currentItem == 0 ) { return; }

    objectTree->expandAll();
    objectTree->scrollToItem( objectTree->currentItem() );

}

void MainWindow::objectTreeCollapseAll() {

    TestObjectKey currentItem = ptr2TestObjectKey( objectTree->currentItem() );

    // exit if object tree is empty
    if ( currentItem == 0 ) { return; }

    objectTree->collapseAll();
    objectTree->setCurrentItem( objectTree->topLevelItem( 0 ) );
    objectTree->scrollToItem( objectTree->currentItem() );

}

void MainWindow::objectTreeKeyPressEvent( QKeyEvent * event ) {

    TestObjectKey currentItem = ptr2TestObjectKey( objectTree->currentItem() );

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




