############################################################################
## 
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). 
## All rights reserved. 
## Contact: Nokia Corporation (testabilitydriver@nokia.com) 
## 
## This file is part of Testability Driver. 
## 
## If you have questions regarding the use of this file, please contact 
## Nokia at testabilitydriver@nokia.com . 
## 
## This library is free software; you can redistribute it and/or 
## modify it under the terms of the GNU Lesser General Public 
## License version 2.1 as published by the Free Software Foundation 
## and appearing in the file LICENSE.LGPL included in the packaging 
## of this file. 
## 
############################################################################


# -------------------------------------------------
# Project created by QtCreator 2010-02-02T10:14:36
# -------------------------------------------------
TEMPLATE = app
TARGET = editor_proto
CONFIG += link_prl

INCLUDEPATH += $$EDITORLIBDIR
LIBS += -L$$EDITORLIBDIR -l$$EDITOR_LIB
QT += network
LIBS += -L$$UTILLIBDIR -l$$UTIL_LIB
QT += sql

SOURCES += src/main.cpp \
    src/mainwindow.cpp
HEADERS += src/mainwindow.h
FORMS +=
OTHER_FILES +=

unix: {
    target.path = /opt/tdriver_visualizer
        #tdriver_editor.files += editor_proto
        #tdriver_editor.path = /opt/tdriver_visualizer
}
win32: {
    target.path = C:/tdriver/visualizer
        #tdriver_editor.files += editor_proto.exe
        #tdriver_editor.path = C:/tdriver/visualizer
}
#INSTALLS += tdriver_editor
INSTALLS += target
