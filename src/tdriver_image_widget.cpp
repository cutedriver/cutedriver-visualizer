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


#include <QPoint>

#include <tdriver_tabbededitor.h>

#include "tdriver_main_window.h"
#include "tdriver_image_view.h"
#include "tdriver_debug_macros.h"

void MainWindow::connectImageWidgetSignals() {

    connect( imageWidget, SIGNAL( forceRefresh()), this, SLOT(forceRefreshData()));

    connect( imageWidget, SIGNAL( imageTapCoords() ), SLOT( clickedImage() ) );
    connect( imageWidget, SIGNAL( imageInspectCoords( ) ), SLOT( imageInspectFindItem() ) );
    connect( imageWidget, SIGNAL( imageInsertCoordsAtClick()), SLOT( imageInsertCoords() ) );
    connect( imageWidget, SIGNAL( imageInsertObjectAtClick()), SLOT( imageInsertFindItem() ) );

    connect( imageWidget, SIGNAL( imageInsertObjectById(TestObjectKey)), SLOT(imageInsertObjectFromId(TestObjectKey)));
    connect( imageWidget, SIGNAL( imageInspectById(TestObjectKey)), SLOT(imageInspectFromId(TestObjectKey)));
    connect( imageWidget, SIGNAL( imageTapById(TestObjectKey)), SLOT(imageTapFromId(TestObjectKey)));

    connect( imageWidget, SIGNAL(imageTasIdChanged(QString)), SLOT(refreshScreenshotObjectList()));
}


// This is triggered from ImageWidget when hovering on image.
// Gets x,y from imagewidget and searches for item. If an item is found it is higlighted.
 void MainWindow::imageInspectFindItem()
 {
    QRect area(0, 0, imageWidget->imageWidth()-1, imageWidget->imageHeight()-1);
    QPoint pos(imageWidget->getEventPosInImage());

    if (area.isValid() && area.contains(pos)) {
        highlightAtCoords( pos, true );
        if (TDriverUtil::isSymbianSut(activeDeviceParams.value("type"))){
              statusbar( tr("inspect: (%1, %2) landscape: (%3, %4)").arg(pos.x()).arg(pos.y()).arg(pos.x()).arg(imageWidget->imageHeight() - 1 - pos.y()), 3000 );
            else
              statusbar( tr("inspect: (%1, %2)").arg(pos.x()).arg(pos.y()), 2000 );
        }
        
    }
}


void MainWindow::imageInsertFindItem()
{
    QRect area(0, 0, imageWidget->imageWidth()-1, imageWidget->imageHeight()-1);
    QPoint pos(imageWidget->getEventPosInImage());

    if (area.isValid() && area.contains(pos)) {

        highlightAtCoords( pos, true, "tap" );
        statusbar( tr("Inserted: (%1, %2)").arg(pos.x()).arg(pos.y()), 2000 );
    }
}


void MainWindow::imageInsertCoords()
{
    QPoint pos = imageWidget->getEventPosInImage();
    emit insertToCodeEditor(QString(" %1, %2 ").arg(pos.x()).arg(pos.y()), false, false);
}


void MainWindow::imageInsertObjectFromId(TestObjectKey id)
{
    //qDebug() << __FUNCTION__ << id;
    highlightByKey(id, true, "");
}


void MainWindow::imageInspectFromId(TestObjectKey id)
{
    //qDebug() << __FUNCTION__ << id;
    highlightByKey(id, true);
}


void MainWindow::imageTapFromId(TestObjectKey id)
{
    if ( highlightByKey( id, false ) && lastHighlightedObjectKey != 0 && currentApplication.haveId()) {
        TreeItemInfo treeItemData = objectTreeData.value( lastHighlightedObjectKey );
        sendTapScreen(QStringList() << "tap"
                      << treeItemData.type + "(:id=>" + TDriverUtil::rubySingleQuote(treeItemData.id) + ")"
                      << currentApplication.id);
        highlightByKey( id, true );
    }
    else {
        QMessageBox::critical(this,
                              tr("Tap to screen"),
                              tr("Bad tap attempt, object id %1, app id %2")
                              .arg(testObjectKey2Str(id), currentApplication.id));
    }
}


// Send tap command to SUT.
void MainWindow::sendTapScreen(const QStringList &target)
{
    qDebug() << FCFL << target;
    statusbar(tr("Tapping..."));
    typedef QList<QByteArray> QByteArrayList;

    QStringList input(QStringList() << activeDevice << target);
    if ( !sendTDriverCommand(commandTapScreen, input, "screen tapping") ) {
        statusbar( "Error: Failed to send tap to the screen", 2000);
    }
}


// Image has been clicked.
// Fetch X, Y from imageWidget, and try to tap object that was clicked
void MainWindow::clickedImage()
{
    QPoint pos(imageWidget->getEventPosInImage());

    if ( highlightAtCoords( pos, false ) && lastHighlightedObjectKey != 0 ) {
        const TreeItemInfo &treeItemData = objectTreeData.value( lastHighlightedObjectKey );
        sendTapScreen( QStringList() << "tap"
                      << treeItemData.type + "(:id=>" + TDriverUtil::rubySingleQuote(treeItemData.id) + ")"
                      << currentApplication.id);
        highlightAtCoords( pos, true );
    }
    else {
        QMessageBox::critical(
                    this,
                    tr("Tap to screen"),
                    tr("No object found at coordinates x: %1, y: %2").arg(pos.x()).arg(pos.y()) );
    }
}


void MainWindow::drawHighlight( TestObjectKey itemKey, bool multiple )
{
    if ( itemKey && itemKey != lastHighlightedObjectKey) {

        bool valid = false;

        if (screenshotObjects.contains(itemKey)) {
            // collect geometries for item and its childs
            RectList geometries;
            collectGeometries( testObjectKey2Ptr(itemKey), geometries);

            if ( !geometries.isEmpty() ) {

                imageWidget->drawHighlights( geometries, multiple );
                lastHighlightedObjectKey = itemKey;
                valid = true;
            }
        }

        if (!valid) {
            // nothing valid to highlight
            //qDebug() << FCFL << "not on screenshot:" << testObjectKey2Str(itemKey);
            imageWidget->disableDrawHighlight();
            lastHighlightedObjectKey = 0;
        }
    }
}


// Get list of all visible objects that are under given position
bool MainWindow::collectMatchingVisibleObjects( QPoint pos, QList<TestObjectKey> &matchingObjects)
{
    QSet<TestObjectKey>::const_iterator visibleObject;

    matchingObjects.clear();

    for (visibleObject = screenshotObjects.constBegin();
         visibleObject != screenshotObjects.constEnd();
         ++visibleObject ) {

        // add only objects that are in geometriesMap
        const RectList &geometries = geometriesMap.value( *visibleObject );
        if ( !geometries.isEmpty() ) {

            // add only objects that contain pos
            if (geometries.first().contains( pos.x(), pos.y() ) ) {

                // don't add Layouts and LayoutItems, they shouldn't be selectable from the image
                QString objectType = attributesMap.value(*visibleObject).value("objecttype").value;
                if (objectType != "Layout" && objectType != "LayoutItem") {
                    matchingObjects << (TestObjectKey)( *visibleObject );
                }
            }
        }
    }

    return !matchingObjects.isEmpty();
}


// Get smallest object from list of objects
bool MainWindow::getSmallestObjectFromMatches( QList<TestObjectKey> *matchingObjects, TestObjectKey & objectPtr ) {

    //qDebug() << "getSmallestObjectFromMatches";

    bool result = false;
    int smallestObjectSize = -1;
    TestObjectKey smallestObjectPtr = 0;

    QList<TestObjectKey>::const_iterator matchingObject;
    for ( matchingObject = matchingObjects->constBegin(); matchingObject != matchingObjects->constEnd(); ++matchingObject ) {

        const RectList &geometries = geometriesMap.value( *matchingObject );

        if ( !geometries.isEmpty() ) {

            // calculate size of object in pixels
            int matchingObjectArea = geometries.first().height() * geometries.first().width();

            if ( ( smallestObjectPtr == 0 || matchingObjectArea < smallestObjectSize ) && matchingObjectArea > 0 ) {
                smallestObjectPtr = *matchingObject;
                smallestObjectSize = matchingObjectArea;
                result = true;
            }
        }
    }

    objectPtr = smallestObjectPtr;
    return result;
}


// Highlight object specified by itemKey in the image.
// Optionally select it in the object tree.
// Optionally call popup method to do editor insertion.
bool MainWindow::highlightByKey( TestObjectKey itemKey, bool selectItem, QString insertMethodToEditor )
{
    if (!itemKey) {
        return false;
    }

    drawHighlight( itemKey, false );

    QTreeWidgetItem *item = testObjectKey2Ptr(itemKey);

    // select item from object tree if selectItem is true
    if ( selectItem ) {
        objectTree->scrollToItem( item );
        objectTree->setCurrentItem( item );
    }

    if (!insertMethodToEditor.isNull()) {
        objectViewItemAction(item, 0, insertAction, insertMethodToEditor);
    }

    return true;
}


// Get item key for coordinates and call highlightByKey if successful
bool MainWindow::highlightAtCoords( QPoint pos, bool selectItem, QString insertMethodToEditor )
{
    bool result = false;
    QList<TestObjectKey> matchingObjects;

    if ( collectMatchingVisibleObjects( pos, matchingObjects ) ) {

        qSort( matchingObjects.begin(), matchingObjects.end() );
        TestObjectKey matchingObject = 0;

        if ( getSmallestObjectFromMatches( &matchingObjects, matchingObject ) ) {
            result = highlightByKey(matchingObject, selectItem, insertMethodToEditor);
        }
    }

    return result;
}


