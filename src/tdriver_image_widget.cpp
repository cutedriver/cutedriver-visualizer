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

    connect( imageWidget, SIGNAL( imageTapCoords() ), this, SLOT( clickedImage() ) );
    connect( imageWidget, SIGNAL( imageInspectCoords( ) ), this, SLOT( imageInspectFindItem() ) );
    connect( imageWidget, SIGNAL( imageInsertCoordsAtClick()), this, SLOT( imageInsertCoords() ) );
    connect( imageWidget, SIGNAL( imageInsertObjectAtClick()), this, SLOT( imageInsertFindItem() ) );

    connect( imageWidget, SIGNAL( imageInsertObjectById(TestObjectKey)), this, SLOT(imageInsertObjectFromId(TestObjectKey)));
    connect( imageWidget, SIGNAL( imageInspectById(TestObjectKey)), this, SLOT(imageInspectFromId(TestObjectKey)));
    connect( imageWidget, SIGNAL( imageTapById(TestObjectKey)), this, SLOT(imageTapFromId(TestObjectKey)));

    connect( imageWidget, SIGNAL(imageTasIdChanged(QString)), this, SLOT(refreshScreenshotObjectList()));
}


// This is triggered from ImageWidget when hoovering on image - gets x,y from imagewidget and searches for item.
// If an item is found it is higlighted
 void MainWindow::imageInspectFindItem()
 {
    QRect area(0, 0, imageWidget->imageWidth()-1, imageWidget->imageHeight()-1);
    QPoint pos(imageWidget->getEventPosInImage());

    if (area.isValid() && area.contains(pos)) {
        highlightAtCoords( pos, true );
        // convert to S60 coordinates
        if ( activeDevice.value( "type" ).toLower() == "s60" ) {
            QPoint s60Pos(pos);
            imageWidget->convertS60Pos(s60Pos);
            statusbar( tr("Inspect: image coordinates (%1, %2) = S60 coordinates (%3, %4)")
                      .arg(pos.x()).arg(pos.y()).arg(s60Pos.x()).arg(s60Pos.y()),
                      2000 );
        }
        else statusbar( tr("inspect: (%1, %2)").arg(pos.x()).arg(pos.y()), 2000 );
    }
}


void MainWindow::imageInsertFindItem()
{
    QRect area(0, 0, imageWidget->imageWidth()-1, imageWidget->imageHeight()-1);
    QPoint pos(imageWidget->getEventPosInImage());

    if (area.isValid() && area.contains(pos)) {

        highlightAtCoords( pos, true, "tap" );

        if ( activeDevice.value( "type" ).toLower() == "s60" ) {
            QPoint s60Pos(pos);
            imageWidget->convertS60Pos(s60Pos);
            statusbar( tr("Inserted: image coordinates (%1, %2) = S60 coordinates (%3, %4)")
                      .arg(pos.x()).arg(pos.y()).arg(s60Pos.x()).arg(s60Pos.y()),
                      2000 );
        }
        else statusbar( tr("Inserted: (%1, %2)").arg(pos.x()).arg(pos.y()), 2000 );
    }
}


void MainWindow::imageInsertCoords()
{
    QPoint pos = imageWidget->getEventPosInImage();
    //emit insertToCodeEditor(QString("@sut.tap_screen(%1, %2)\n").arg(pos.x()).arg(pos.y()), false, false);
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
    //qDebug() << __FUNCTION__ << id;

    if ( highlightByKey( id, false ) && lastHighlightedObjectKey != 0 && !currentApplication.isNull()) {
        const TreeItemInfo &treeItemData = objectTreeData.value( lastHighlightedObjectKey );
        tapScreen( "tap " + treeItemData.type + "(:id=>" + treeItemData.id + ") " + currentApplication.id );
        highlightByKey( id, true );
    }
    else {
        QMessageBox::critical( 0, "Tap to screen", "Bad tap attempt, object id:" + testObjectKey2Str(id) + ", app id:" + currentApplication.id);
    }
}


void MainWindow::tapScreen( QString target )
{
    //qDebug() << "tapScreen";
    statusbar( "Tapping...", 0, 1 );
    typedef QList<QByteArray> QByteArrayList;

    if ( !executeTDriverCommand( commandTapScreen, QString( activeDevice.value( "name" ) + " " + target ))) {
        statusbar( "Error: Failed to tap the screen", 1000 );
    }

    else {
        statusbar( "Tapping...", 1000 );
        refreshData();
    }
}


// Image has been clicked - fetch X, Y from imageWidget, and
// if S60 without Qt, send tap_screen command to coordinates,
// else try to tap object that was clicked
void MainWindow::clickedImage()
{
    QPoint pos(imageWidget->getEventPosInImage());

    if ( activeDevice.value( "type" ).toLower() == "s60" ) {
        imageWidget->convertS60Pos(pos);
        tapScreen( "tap_screen " + QString::number( pos.x() ) + " " + QString::number( pos.y() ) );
    }

    else {
        if ( highlightAtCoords( pos, false ) && lastHighlightedObjectKey != 0 ) {
            const TreeItemInfo &treeItemData = objectTreeData.value( lastHighlightedObjectKey );
            tapScreen( "tap " + treeItemData.type + "(:id=>" + treeItemData.id + ") " + currentApplication.id );
            highlightAtCoords( pos, true );
        }

        else {
            QMessageBox::critical( 0, "Tap to screen", "No object found at coordinates x: " + QString::number( pos.x() ) + ", y: " + QString::number( pos.y() ) );
        }
    }
}


void MainWindow::drawHighlight( TestObjectKey itemKey, bool multiple )
{
    if ( itemKey && itemKey != lastHighlightedObjectKey) {

        bool valid = false;

        if (screenshotObjects.contains(itemKey)) {
            QMap<QString, QHash<QString, QString> > attributes = attributesMap.value( itemKey );

            // collect geometries for item and its childs
            const RectList &geometries = geometriesMap.value( itemKey );

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



bool MainWindow::collectMatchingVisibleObjects( QPoint pos, QList<TestObjectKey> & matchingObjects ) {

    //qDebug() << "collectMatchingVisibleObjects";

    bool result = false;

    matchingObjects.clear();

    QSet<TestObjectKey>::const_iterator visibleObject;

    for ( visibleObject = screenshotObjects.constBegin(); visibleObject != screenshotObjects.constEnd(); ++visibleObject ) {

        const RectList &geometries = geometriesMap.value( *visibleObject );

        if ( !geometries.isEmpty() ) {

            if (geometries.first().contains( pos.x(), pos.y() ) ) {
                matchingObjects << (TestObjectKey)( *visibleObject );
                result = true;
            }

        }

    }

    return result;

}



bool MainWindow::getSmallestObjectFromMatches( QList<TestObjectKey> *matchingObjects, TestObjectKey & objectPtr ) {

    //qDebug() << "getSmallestObjectFromMatches";

    bool result = false;
    int smallestObjectSize = -1;
    TestObjectKey smallestObjectPtr = 0;

    QList<TestObjectKey>::const_iterator matchingObject;
    for ( matchingObject = matchingObjects->constBegin(); matchingObject != matchingObjects->constEnd(); ++matchingObject ) {

        RectList geometries = geometriesMap.value( *matchingObject );

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

bool MainWindow::highlightAtCoords( QPoint pos, bool selectItem, QString insertMethodToEditor ) {

    //qDebug() << __FUNCTION__;

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


