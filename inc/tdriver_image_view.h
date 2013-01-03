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
#include <QLabel>
#include <QWidget>
#include <QFrame>
#include <QPaintEvent>
#include <QPainter>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
//#include <QWaitCondition>
#include <QPointF>

#include <QRect>
#include <QList>

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

    void refreshImage(const QString &imagePath);

    void drawHighlights( RectList geometries, bool multiple );
    void disableDrawHighlight();

    int imageWidth() { return image->width(); }
    int imageHeight() { return image->height(); }
    QString tasIdString() { return imageTasId; }
    QString lastImageFileName() const { return imageFileName; }

    QPoint getPosInImage(const QPoint &pos) {
        return QPoint(float(pos.x()) / zoomFactor, float(pos.y()) / zoomFactor);
    }

    QPoint getMousePosInImage() {
        return QPoint(float(mousePos.x() - imageOffset.x()) / zoomFactor,
                      float(mousePos.y() - imageOffset.y()) / zoomFactor);
    }


    //    QPoint &convertS60Pos(QPoint &pos) {
    //        // rotate when in portrait mode
    //        if ( image->width() > image->height())
    //            pos = QPoint(image->height() - pos.y(), pos.x());
    //        return pos;
    //    }

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

    void imageTasIdChanged(QString tasId);

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
    QString imageFileName;
    QString imageTasId;
    QPixmap *pixmap;

    int highlightEnabledMode; // 0=disabled, 1=single, 2=multiple

    bool updatePixmap;
    bool scaleImage;
    int leftClickAction;

    QPoint imageOffset; // coordinates of image upper left corner
    QPoint mousePos; // coordinates of last relevant mouse event
    QPoint stopPos; // coordinates where mouse cursor last stopped

    bool dragging; // when true, drag in progress, dragStart and dragEnd valid
    QPoint dragStart; // dragged areas first corner in *pixmap coordinates
    QPoint dragEnd; // dragged areas 2nd, moving corner in *pixmap coordinates

    float zoomFactor;

    RectList rects;

    MainWindow *objTreeOwner;
};


#endif
