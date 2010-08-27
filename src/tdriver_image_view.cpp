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



#include "tdriver_image_view.h"
#include "tdriver_main_window.h"

#include <QMenu>
#include <QPoint>


TDriverImageView::TDriverImageView( MainWindow *tdriverMainWindow, QWidget* parent) :
        QFrame( parent ),
        objTreeOwner(tdriverMainWindow)
{
    //setup timer
    hoverTimer = new QTimer(this);
    connect( hoverTimer, SIGNAL( timeout() ), this, SLOT( hoverTimeout() ) );

    setMouseTracking( true ); //enable tracking of mouse movement

    scaleImage = true;

    leftClickAction = SUT_DEFAULT_TAP;

    pixmap = NULL;

    image = new QImage();
}

TDriverImageView::~TDriverImageView()
{}



void TDriverImageView::changeImageResized(int state) {
    scaleImage = (state != Qt::Unchecked);
}

void TDriverImageView::setLeftClickAction(int action){
    leftClickAction = action;
    if (leftClickAction == VISUALIZER_INSPECT) {
        hoverTimer->stop();
    }
}

void TDriverImageView::clearImage() {

    qDebug() << "clean image";
    //image = NULL;
    image = new QImage();
    this->repaint();

}

void TDriverImageView::paintEvent( QPaintEvent *event )
{
    //qDebug() << "paintEvent";

    Q_UNUSED( event );

    QPainter painter( this );

    if ( pixmap ) { delete pixmap; }

    if ( scaleImage && !image->isNull() ) {
        pixmap = new QPixmap( QPixmap::fromImage( image->scaled( size(), Qt::KeepAspectRatio, Qt::SmoothTransformation) ) );
    } else {
        pixmap = new QPixmap( QPixmap::fromImage( image->copy(), Qt::AutoColor ) );
    }

    if ( pixmap->isNull() || !painter.isActive() ) {


    } else {

        painter.drawPixmap( pixmap->rect(), *pixmap );

        aspectRatio = float( image->width() ) / float( pixmap->width() );

        if ( highlightEnabled ) {

            QPen myPen;
            myPen.setColor( Qt::red );
            myPen.setWidth( 3 );
            painter.setPen( myPen );
            painter.setBrushOrigin( 0, 0 );

            if ( highlightMultipleObjects ) {

                for ( int n = 0; n < rects.size(); n++ ) {

                    QStringList dimensions = rects[n].split(",");

                    if ( dimensions.size() == 4 && dimensions[ 0 ].size() > 0 && dimensions[ 1 ].size() > 0 && dimensions[ 2 ].size() > 0 && dimensions[ 3 ].size() > 0) {

                        float temp_x = dimensions[ 0 ].toFloat() + highlight_offset_x;
                        float temp_y = dimensions[ 1 ].toFloat() + highlight_offset_y;

                        float temp_w = dimensions[ 2 ].toFloat();
                        float temp_h = dimensions[ 3 ].toFloat();

                        QRectF rectangle(
                                qreal( temp_x != 0 ? temp_x / aspectRatio : temp_x ),
                                qreal( temp_y != 0 ? temp_y / aspectRatio : temp_y ),
                                qreal( temp_w != 0 ? temp_w / aspectRatio : temp_w ),
                                qreal( temp_h != 0 ? temp_h / aspectRatio : temp_h )
                                );

                        painter.drawRect( rectangle );

                    }
                }

            } else {
                QRectF rectangle(
                        qreal( x_rect != 0 ? x_rect / aspectRatio : x_rect ),
                        qreal( y_rect != 0 ? y_rect / aspectRatio : y_rect ),
                        qreal( width_rect != 0 ? width_rect / aspectRatio : width_rect ),
                        qreal( height_rect != 0 ? height_rect / aspectRatio : height_rect )
                        );

                painter.drawRect( rectangle );

            }
        }
    }
}


// Stores current position and emits a signal that mouse was clicked
void TDriverImageView::mousePressEvent( QMouseEvent * event ) {

    //qDebug() << "mousePressEvent";

    if ( !( event->x() < pixmap->width()  &&  event->y() < pixmap->height() ) ) {
//        event->ignore();
        pressedPos = QPoint(-1, -1);
        return;
    }

    pressedPos = event->pos() * aspectRatio;

    switch (event->button()) {

    case Qt::LeftButton:
        switch (leftClickAction) {

        case VISUALIZER_INSPECT:
            emit imageInspectCoords();
            break;

        case SUT_DEFAULT_TAP:
            emit imageTapCoords();
            break;

        case ED_COORD_INSERT:
            emit imageInsertCoordsAtClick();
            break;

        case ED_TESTOBJ_INSERT:
            emit imageInsertObjectAtClick();
            break;
        }
        break; // event->button()

    default:
        QFrame::mousePressEvent(event);
        //event->ignore();
    }
}

// Function called when hover timer times out. If same global positions then emit signal to mainWindow
void TDriverImageView::hoverTimeout()
{
    if ( QCursor::pos() == hoverPos ) {
        emit imageInspectCoords();
    }
}


// MouseMoveEvent - stores current coordinates accordint to aspect ratio + stores current position for update
void TDriverImageView::mouseMoveEvent( QMouseEvent * event )
{
    if (leftClickAction != VISUALIZER_INSPECT) {
        pressedPos = event->pos() * aspectRatio;
        hoverPos = QCursor::pos();

        hoverTimer->setSingleShot( true );
        hoverTimer->start(500);
    }

    QFrame::mouseMoveEvent(event);
}


static int idFromActionData(QObject *sender)
{
    QAction *senderAct = qobject_cast<QAction*>(sender);

    if (!senderAct) return -1;

    bool ok = false;
    int id = senderAct->data().toInt(&ok);

    if(!ok || id < 0) return -1;

    return id;
}


void TDriverImageView::forwardTapById()
{
    int id = idFromActionData(sender());
    //qDebug() << __FUNCTION__ << id;
    if (id >= 0) emit imageTapById(id);
}


void TDriverImageView::forwardInspectById()
{
    int id = idFromActionData(sender());
    //qDebug() << __FUNCTION__ << id;
    if (id >= 0) emit imageInspectById(id);
}


void TDriverImageView::forwardInsertObjectById()
{
    int id = idFromActionData(sender());
    if (id >= 0) emit imageInsertObjectById(id);
}


void TDriverImageView::contextMenuEvent ( QContextMenuEvent *event)
{
    QMenu *menu = new QMenu("Image View Context Menu");

    if (objTreeOwner) {
        QList<int> matchingObjects;

        if ( objTreeOwner->collectMatchingVisibleObjects( pressedPos, matchingObjects ) ) {
            qSort( matchingObjects.begin(), matchingObjects.end() );
            QMap<QAction*, QString> sortKeys;

            foreach(int id, matchingObjects) {
                const QMap<QString, QHash<QString, QString> > &attrMap = objTreeOwner->testobjAttributes(id);
                const QHash<QString, QString> &treeData = objTreeOwner->testobjTreeData(id);

                // create heading entry to context menu
                QString tmpText;
                QString idText;
                QString sortKey;

                tmpText = treeData["name"];
                if ( tmpText.isEmpty())
                    tmpText = attrMap["objectname"]["value"];
                if ( !tmpText.isEmpty()) {
                    idText += QString(" name '%1'").arg(tmpText);
                    sortKey = "1"+tmpText;
                }

                tmpText = attrMap["text"]["value"];
                if ( !tmpText.isEmpty()) {
                    idText += QString(" text '%1'").arg(tmpText);
                    if (sortKey.isEmpty())
                        sortKey = "2"+tmpText;
                }

                if (idText.isEmpty())
                    idText += QString(" id %1").arg(id);

                if (sortKey.isEmpty())
                    sortKey = "3"+QString::number(id).rightJustified(11);

                QString typeText = treeData["type"];
                if (typeText.isEmpty())
                    typeText = attrMap["objecttype"]["value"];
                if (typeText.isEmpty())
                    typeText = "????";

                // find position in menu
                QAction *before = NULL;
                foreach (QAction *act, menu->actions()) {
                    //qDebug() << FCFL << "comparing" << sortKeys[act] << ">" << sortKey;
                    QString key = sortKeys[act];
                    if ( !key.isEmpty() && key > sortKey) {
                        before = act;
                        break;
                    }
                }

                // Object description performs find in tree
                QAction *tmpAct = new QAction(typeText + idText, menu);
                sortKeys[tmpAct] = sortKey;
                tmpAct->setData(id);
                QFont tmpFont = tmpAct->font();
                tmpFont.setBold(true);
                tmpFont.setUnderline(true);
                tmpAct->setFont(tmpFont);
                menu->insertAction(before, tmpAct);
                connect(tmpAct, SIGNAL(triggered()), this, SLOT(forwardInspectById()));

                // extra actions
                tmpAct = new QAction(tr("    Send Tap to SUT"), menu);
                tmpAct->setData(id);
                menu->insertAction(before, tmpAct);
                connect(tmpAct, SIGNAL(triggered()), this, SLOT(forwardTapById()));

                tmpAct = new QAction(tr("    Insert to Editor"), menu);
                tmpAct->setData(id);
                menu->insertAction(before, tmpAct);
                connect(tmpAct, SIGNAL(triggered()), this, SLOT(forwardInsertObjectById()));
            }
        }

        menu->addSeparator();
    }

    QAction *refreshAct = menu->addAction(tr("Refresh"));
    connect(refreshAct, SIGNAL(triggered()), this, SIGNAL(forceRefresh()));

#if 0
    tmpAct =
    tmpAct->setText(tr("Inspect testobject under cursor"));
    connect(tmpAct, SIGNAL(triggered()), this, SIGNAL(imageInspectCoords()));
    menu->addAction(tmpAct);

    tmpAct =
    tmpAct->setText(tr("Insert testobject tap to Code Editor"));
    //insertObjectTap->setToolTip(selText);
    //insertObjectTap->setStatusTip(cur.selectedText());
    //connect(tmpAct, SIGNAL(triggered()), this, SLOT());
    connect(tmpAct, SIGNAL(triggered()), this, SIGNAL(imageInsertObjectAtClick()));
    menu->addAction(tmpAct);

    tmpAct =
    tmpAct->setText(tr("Insert coordinates Code Editor"));
    //insertCoordTap->setToolTip(selText);
    //insertCoordTap->setStatusTip(cur.selectedText(§§§));
    //connect(tmpAct, SIGNAL(triggered()), this, SLOT());
    connect(tmpAct, SIGNAL(triggered()), this, SIGNAL(imageInsertCoordsAtClick()));
    menu->addAction(tmpAct);
#endif
    menu->exec(event->globalPos());
    delete menu;
}


void TDriverImageView::refreshImage( QString imagePath, bool disableHighlight ) {

    //qDebug() << "refreshImage";

    if ( image ) {

        delete image;
        if ( disableHighlight ) { disableDrawHighlight(); }
    }

    image = new QImage( imagePath );

}

void TDriverImageView::drawHighlight( float x, float y, float width, float height ) {

    //qDebug() << "drawHighlight";

    x_rect = x;
    y_rect = y;

    width_rect = width;
    height_rect = height;

    highlightEnabled = true;
    highlightMultipleObjects = false;

}

void TDriverImageView::disableDrawHighlight() {

    //qDebug() << "disableDrawHighlight";

    highlightEnabled = false;
    highlightMultipleObjects = false;
}

void TDriverImageView::drawMultipleHighlights( QStringList geometries, int offset_x, int offset_y )
{

    //qDebug() << "drawHighlight";

    highlightEnabled = true;
    highlightMultipleObjects = true;

    highlight_offset_x = offset_x;
    highlight_offset_y = offset_y;

    rects = geometries;
}


