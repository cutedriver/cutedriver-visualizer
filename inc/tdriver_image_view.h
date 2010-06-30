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

class MainWindow;

class TDriverImageView : public QFrame
{
    Q_OBJECT

public:

    // constructor
    TDriverImageView( MainWindow *tdriverMainWindow, QWidget* parent = NULL);

    // destructor
    ~TDriverImageView();

    void refreshImage( QString imagePath, bool disableHighlight );

    void paintEvent( QPaintEvent *event );

    void drawHighlight( float x, float y, float width, float height );
    void drawMultipleHighlights( QStringList geometries, int offset_x, int offset_y );
    void disableDrawHighlight();

    int imageWidth() { return image->width();    }
    int imageHeight() { return image->height(); }

    QPoint getPressedPos() { return pressedPos; }
    QPoint &convertS60Pos(QPoint &pos) { // rotate when in portrait mode
        if ( image->width() > image->height())
            pos = QPoint(image->height() - pos.y(), pos.x());
        return pos;
    }

    void changeImageResized(int state);
    void setLeftClickAction (int action);
    void clearImage();

    enum LeftClickActionType { VISUALIZER_INSPECT, SUT_DEFAULT_TAP, ED_COORD_INSERT, ED_TESTOBJ_INSERT };

signals:

    void forceRefresh();
    void imageTapCoords();
    void imageInspectCoords();
    void imageInsertObjectAtClick();
    void imageInsertCoordsAtClick();

    void imageTapById(int id);
    void imageInspectById(int id);
    void imageInsertObjectById(int id);

    void insertToCodeEditor(QString text, bool prependParent, bool prependDot);

private slots:
    void forwardTapById();
    void forwardInspectById();
    void forwardInsertObjectById();

protected:
    virtual void mousePressEvent ( QMouseEvent *);
    virtual void mouseMoveEvent ( QMouseEvent *);
    virtual void contextMenuEvent ( QContextMenuEvent *);

private slots:
    void hoverTimeout();

private:

    QTimer * hoverTimer;
    QImage * image;
    QPixmap * pixmap;

    bool highlightEnabled;
    bool highlightMultipleObjects;

    bool scaleImage;
    int leftClickAction;

    QPoint pressedPos; // position in screen capture image

    QPoint hoverPos; // position where mouse movement stopped

    int highlight_offset_x;
    int highlight_offset_y;

    float x_rect;
    float y_rect;

    float width_rect;
    float height_rect;

    float aspectRatio;

    QStringList rects;

    MainWindow *objTreeOwner;
};


#endif
