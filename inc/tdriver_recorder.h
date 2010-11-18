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



#ifndef TDRIVERRECORDER_H
#define TDRIVERRECORDER_H

#include <QtCore/QTextStream>

#include <QtGui/QDialog>
#include <QtGui/QTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>

#include <QMap>

#include <tdriver_rubyinterface.h>

class TDriverRecorder : public QDialog {

    Q_OBJECT

    public:

        TDriverRecorder(QWidget* parent = 0 );
        ~TDriverRecorder();

    public:

        void setActiveDevAndApp( QString StrActiveDevice, QString activeApp );
        void setOutputPath( QString path );

    protected slots:

        void startRecording();
        void stopRecording();
        void testRecording();

    private:
        void setup();
        void setActionsEnabled(bool start, bool stop, bool test);

    private:
        QString outputPath;

        QString mCurrentApplication;

        QTextEdit* mScriptField;

        QPushButton* mRecButton;
        QPushButton* mStopButton;
        QPushButton* mTestButton;

        QString mActiveApp;
        QString mStrActiveDevice;
};

#endif

