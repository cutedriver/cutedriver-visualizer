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


#ifndef TDRIVERIMAGEVIEW_H
#define TDRIVERIMAGEVIEW_H

#include <QPoint>
#include <QtGui/QLabel>
#include <QtGui/QWidget>
#include <QtGui/QFrame>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
//#include <QWaitCondition>
#include <QPointF>

#include "tdriver_main_types.h"

class MainWindow;

class TDriverImageView : public QFrame
{
    Q_OBJECT

public:

    // constructor
    TDriverImageView( MainWindow *tdriverMainWindow, QWidget* parent = NULL);

    // destructor
    ~TDriverImageView();

    void refreshImage(QString imagePath);

    void drawHighlight( float x, float y, float width, float height );
    void drawMultipleHighlights( QStringList geometries, int offset_x, int offset_y );
    void disableDrawHighlight();

    int imageWidth() { return image->width(); }
    int imageHeight() { return image->height(); }

    QPoint getPosInImage(const QPoint &pos) {
        return QPoint(float(pos.x()) / zoomFactor, float(pos.y()) / zoomFactor);
    }

    QPoint getEventPosInImage() {
        return QPoint(float(mousePos.x()) / zoomFactor, float(mousePos.y()) / zoomFactor);
    }



    QPoint &convertS60Pos(QPoint &pos) {
        // rotate when in portrait mode
        if ( image->width() > image->height())
            pos = QPoint(image->height() - pos.y(), pos.x());
        return pos;
    }

    void changeImageResized(bool checked);
    void setLeftClickAction (int action);
    void clearImage();

    enum LeftClickActionType { VISUALIZER_INSPECT, SUT_DEFAULT_TAP, ED_COORD_INSERT, ED_TESTOBJ_INSERT };

signals:

    void forceRefresh();
    void imageTapCoords();
    void imageInspectCoords();
    void imageInsertObjectAtClick();
    void imageInsertCoordsAtClick();

    void imageTapById(TestObjectKey id);
    void imageInspectById(TestObjectKey id);
    void imageInsertObjectById(TestObjectKey id);

    void insertToCodeEditor(QString text, bool prependParent, bool prependDot);

    void statusBarMessage(QString text, int timeout);

protected:
    virtual void paintEvent( QPaintEvent *event );
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void contextMenuEvent(QContextMenuEvent *);
    virtual void resizeEvent(QResizeEvent *);

private slots:
    void hoverTimeout();
    void dragAction();
    void forwardTapById();
    void forwardInspectById();
    void forwardInsertObjectById();

private:

    QTimer * hoverTimer;
    QImage *image;
    QPixmap *pixmap;

    bool highlightEnabled;
    bool highlightMultipleObjects;

    bool updatePixmap;
    bool scaleImage;
    int leftClickAction;

    QPoint mousePos; // coordinates of last relevant mouse event
    QPoint stopPos; // coordinates where mouse cursor last stopped

    bool dragging; // when true, drag in progress, dragStart and dragEnd valid
    QPoint dragStart; // dragged areas first corner in *pixmap coordinates
    QPoint dragEnd; // dragged areas 2nd, moving corner in *pixmap coordinates

    int highlight_offset_x;
    int highlight_offset_y;

    float x_rect;
    float y_rect;

    float width_rect;
    float height_rect;

    float zoomFactor;

    QStringList rects;

    MainWindow *objTreeOwner;
};


#endif
