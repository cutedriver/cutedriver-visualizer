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
#include <QRect>

#include <tdriver_debug_macros.h>


static inline bool testDragThreshold(const QPoint &p1, const QPoint &p2)
{
    return (p1.x() != p2.x() && p1.y() != p2.y());
    //return (qAbs(p1.x()-p2.x()) > 1 && qAbs(p1.y()-p2.y() > 1));
}

static inline void fixPoint(QPoint &point, QRect rect)
{
    rect = rect.normalized();

    if (point.x() < rect.left()) point.setX(rect.left());
    else if (point.x() > rect.right()) point.setX(rect.right());

    if (point.y() < rect.top()) point.setY(rect.top());
    else if (point.y() > rect.bottom()) point.setY(rect.bottom());
}


TDriverImageView::TDriverImageView( MainWindow *tdriverMainWindow, QWidget* parent) :
    QFrame( parent ),
    hoverTimer(new QTimer(this)),
    image(new QImage),
    pixmap(NULL),
    highlightEnabled(false),
    highlightMultipleObjects(false),
    updatePixmap(true),
    scaleImage(true),
    leftClickAction(SUT_DEFAULT_TAP),
    dragging(false),
    zoomFactor(1),
    objTreeOwner(tdriverMainWindow)
{
    connect( hoverTimer, SIGNAL( timeout() ), this, SLOT( hoverTimeout() ) );
    setMouseTracking( true ); //enable tracking of mouse movement
}


TDriverImageView::~TDriverImageView()
{
    if (image) {
        delete image;
        image=NULL;
    }
    if (pixmap) {
        delete pixmap;
        pixmap=NULL;
    }
}



void TDriverImageView::changeImageResized(int state) {
    scaleImage = (state != Qt::Unchecked);
    updatePixmap = true;
}

void TDriverImageView::setLeftClickAction(int action){
    leftClickAction = action;
    if (leftClickAction == VISUALIZER_INSPECT) {
        hoverTimer->stop();
    }
}

void TDriverImageView::clearImage()
{
    if ( image ) delete image;
    image = new QImage();
    updatePixmap = true;
    update();
}

void TDriverImageView::paintEvent( QPaintEvent *event )
{
    //qDebug() << "paintEvent";

    Q_UNUSED( event );

    QPainter painter( this );

    if( !pixmap || updatePixmap) {
        if ( pixmap ) {
            delete pixmap;
            pixmap = NULL;
        }

        if ( scaleImage && !image->isNull() ) {
            pixmap = new QPixmap( QPixmap::fromImage( image->scaled( size(), Qt::KeepAspectRatio, Qt::SmoothTransformation) ) );
            zoomFactor = float(pixmap->width()) / float(image->width());
        } else {
            pixmap = new QPixmap( QPixmap::fromImage( image->copy(), Qt::AutoColor ) );
            zoomFactor = 1;
        }

        updatePixmap = false;
    }

    painter.drawPixmap( pixmap->rect(), *pixmap );
    painter.setOpacity(0.5);

    if ( highlightEnabled ) {
        static QPen highlightPen(QBrush(Qt::red), 2);
        painter.setPen( highlightPen );

        if ( highlightMultipleObjects ) {
            for ( int n = 0; n < rects.size(); n++ ) {
                QStringList dimensions = rects[n].split(",");
                if ( dimensions.size() == 4 && dimensions[ 0 ].size() > 0 && dimensions[ 1 ].size() > 0 && dimensions[ 2 ].size() > 0 && dimensions[ 3 ].size() > 0) {
                    float temp_x = dimensions[ 0 ].toFloat() + highlight_offset_x;
                    float temp_y = dimensions[ 1 ].toFloat() + highlight_offset_y;
                    float temp_w = dimensions[ 2 ].toFloat();
                    float temp_h = dimensions[ 3 ].toFloat();

                    QRectF rectangle(temp_x * zoomFactor, temp_y * zoomFactor,
                                     temp_w * zoomFactor, temp_h * zoomFactor);
                    painter.drawRect( rectangle );
                }
            }

        } else {
            QRectF rectangle(x_rect * zoomFactor, y_rect * zoomFactor,
                             width_rect * zoomFactor, height_rect * zoomFactor);
            painter.drawRect( rectangle );

        }
    }

    if (dragging) {
        //qDebug()  << FCFL << "dragged enough?" << testDragThreshold(dragEnd, dragStart);
        if (testDragThreshold(dragStart, dragEnd)) {
            static QPen dragPen(Qt::white);
            painter.setPen(dragPen);
            QRect draggedRect = QRect(dragStart, dragEnd).normalized();
            painter.fillRect(draggedRect, Qt::SolidPattern);
            painter.drawRect(draggedRect);
        }
    }
}


void TDriverImageView::mousePressEvent(QMouseEvent *event)
{
    mousePos = event->pos();
    hoverTimer->stop();

    if (dragging) {
        // pressing other button while dragging cancels the drag
       dragging = false;
        update();
    }
    if (event->button() == Qt::LeftButton && pixmap) {
        // prepare for possible drag
        dragStart = mousePos;
        fixPoint(dragStart, pixmap->rect());
    }
    else {
        QFrame::mousePressEvent(event);
    }
}


// Stores current position and emits a signal that mouse was clicked
void TDriverImageView::mouseReleaseEvent(QMouseEvent *event)
{
    //qDebug() << "mousePressEvent";

    if (event->button() == Qt::LeftButton) {

        mousePos = event->pos();

        if (dragging && pixmap) {
            // drag in progress, finish it and test if result is valid selection
            dragEnd = mousePos;
            fixPoint(dragEnd, pixmap->rect());
            dragging = testDragThreshold(dragStart, dragEnd);
            if (!dragging) update();
        }

        if (dragging) {
            // valid drag happened
            dragAction();
            dragging = false;
            update();
        }
        else if (pixmap && pixmap->rect().contains(event->pos())) {
            // non-dragging click
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
        }
        else {
        }
    }
    else {
        QFrame::mousePressEvent(event);
    }
}


// MouseMoveEvent - stores current coordinates accordint to aspect ratio + stores current position for update
void TDriverImageView::mouseMoveEvent( QMouseEvent * event )
{
    mousePos = event->pos();

    if (event->buttons() == Qt::LeftButton && pixmap) {
        dragEnd = mousePos;
        fixPoint(dragEnd, pixmap->rect());
        if (!dragging && testDragThreshold(dragStart, dragEnd)) {
            hoverTimer->stop();
            dragging = true;
        }
    }

    if (dragging) {
        repaint();
    }
    else {
        if (leftClickAction != VISUALIZER_INSPECT && event->buttons() == Qt::NoButton) {
            stopPos = QCursor::pos();
            hoverTimer->setSingleShot( true );
            hoverTimer->start(500);
        }
    }

    QFrame::mouseMoveEvent(event);
}


void TDriverImageView::contextMenuEvent ( QContextMenuEvent *event)
{
    QMenu *menu = new QMenu("Image View Context Menu");

    if (objTreeOwner) {
        QList<int> matchingObjects;

        if ( objTreeOwner->collectMatchingVisibleObjects( getEventPosInImage(), matchingObjects ) ) {
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

    tmpAct = tmpAct->setText(tr("Insert testobject tap to Code Editor"));
    //insertObjectTap->setToolTip(selText);
    //insertObjectTap->setStatusTip(cur.selectedText());
    //connect(tmpAct, SIGNAL(triggered()), this, SLOT());
    connect(tmpAct, SIGNAL(triggered()), this, SIGNAL(imageInsertObjectAtClick()));
    menu->addAction(tmpAct);

    tmpAct = tmpAct->setText(tr("Insert coordinates Code Editor"));
    //insertCoordTap->setToolTip(selText);
    //insertCoordTap->setStatusTip(cur.selectedText(§§§));
    //connect(tmpAct, SIGNAL(triggered()), this, SLOT());
    connect(tmpAct, SIGNAL(triggered()), this, SIGNAL(imageInsertCoordsAtClick()));
    menu->addAction(tmpAct);
#endif
    menu->exec(event->globalPos());
    delete menu;
}


// Function called when hover timer times out. If same global positions then emit signal to mainWindow
void TDriverImageView::hoverTimeout()
{
    if ( QCursor::pos() == stopPos && !dragging) {
        emit imageInspectCoords();
    }
}

#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QImageWriter>
#include <QByteArray>
#include <QStringList>
#include <QMessageBox>
#include <QPalette>

void TDriverImageView::dragAction()
{
    if (image && !image->isNull()) {
        QRect cutRect = QRect(dragStart/zoomFactor, dragEnd/zoomFactor).normalized().intersected(image->rect());

        if (cutRect.width() > 0 && cutRect.height() > 0) {
            QPixmap cut(QPixmap::fromImage(image->copy(cutRect)));

            // TODO: move lastSaveDir to application's QSettings
            static QString lastSaveDir = QDir::homePath();

            QStringList imgFormats;
            foreach(const QByteArray &format, QImageWriter::supportedImageFormats()) {
                imgFormats << QString::fromAscii(format);
            }
            QFileDialog *dialog = new QFileDialog(0,
                                              tr("Save selected area as new image"),
                                              lastSaveDir,
                                              tr("Images (*.") + imgFormats.join(" *.") + tr(")"));

            dialog->setOption(QFileDialog::DontUseNativeDialog, true);
            dialog->setOption(QFileDialog::DontConfirmOverwrite, false);
            dialog->setAcceptMode(QFileDialog::AcceptSave);
            dialog->setFileMode(QFileDialog::AnyFile);
            dialog->setDefaultSuffix("png");

            // create a preview of the saved piece

            {
                QScrollArea *previewArea = new QScrollArea(dialog);

                QLabel *previewLabel = new QLabel();
                previewLabel->setPixmap(cut);
                previewLabel->show();
                previewArea->setWidget(previewLabel);

                QPalette pal = previewArea->palette();
                pal.setBrush(QPalette::Window, QBrush(Qt::lightGray, Qt::SolidPattern));
                previewArea->setPalette(pal);
                previewArea->setAutoFillBackground(true);

                previewArea->setAlignment(Qt::AlignCenter);
                previewArea->setWindowFlags(Qt::Dialog);
                previewArea->setWindowTitle(tr("Preview"));
                previewArea->show();
                previewArea->raise();
            }
            dialog->exec();
            if (dialog->result() == QDialog::Accepted) {
                QString fileName(dialog->selectedFiles().first());
                QFileInfo fileInfo(fileName);
                if (!cut.save(fileName)) {
                    QMessageBox::warning(this,
                                         tr("Failed to save"),
                                         tr("Failed to save using filename\n\n%1").arg(fileName));
                }
                else {
                    lastSaveDir = fileInfo.absolutePath();
                }
            }
            delete dialog;
        }
    }
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


void TDriverImageView::refreshImage(QString imagePath)
{
    if ( image ) delete image;
    image = new QImage( imagePath );
    updatePixmap = true;
    update();
}


void TDriverImageView::drawHighlight( float x, float y, float width, float height )
{
    x_rect = x;
    y_rect = y;

    width_rect = width;
    height_rect = height;

    highlightEnabled = true;
    highlightMultipleObjects = false;

}


void TDriverImageView::disableDrawHighlight()
{
    highlightEnabled = false;
    highlightMultipleObjects = false;
    update();
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


