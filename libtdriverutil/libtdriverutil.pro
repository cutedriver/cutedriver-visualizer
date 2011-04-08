# ###########################################################################
# #
# # Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
# # All rights reserved.
# # Contact: Nokia Corporation (testabilitydriver@nokia.com)
# #
# # This file is part of Testability Driver.
# #
# # If you have questions regarding the use of this file, please contact
# # Nokia at testabilitydriver@nokia.com .
# #
# # This library is free software; you can redistribute it and/or
# # modify it under the terms of the GNU Lesser General Public
# # License version 2.1 as published by the Free Software Foundation
# # and appearing in the file LICENSE.LGPL included in the packaging
# # of this file.
# #
# ###########################################################################
# -------------------------------------------------
# Project created by QtCreator 2010-04-16T14:55:40
# -------------------------------------------------
include (../visualizer.pri)

TARGET = tdriverutil

QT += network


TEMPLATE = lib
CONFIG += shared
CONFIG += create_prl

DEFINES += LIBTDRIVERUTIL_LIBRARY


!CONFIG(no_sql) {
    QT += sql
    SOURCES += tdriver_translationdb.cpp
    HEADERS += tdriver_translationdb.h
} else {
    !build_pass:message(SQL dependent features disabled)
}

SOURCES += tdriver_util.cpp \
    tdriver_rubyinterface.cpp \
    tdriver_rbiprotocol.cpp \
    tdriver_executedialog.cpp \
    flowlayout.cpp

HEADERS += libtdriverutil_global.h \
    tdriver_util.h \
    tdriver_rubyinterface.h \
    tdriver_rbiprotocol.h \
    tdriver_debug_macros.h \
    tdriver_executedialog.h \
    flowlayout.h

FORMS += \
    tdriver_executedialog.ui

OTHER_FILES += tdriver_interface.rb

# install
unix:!symbian {
    target.path = /usr/lib
    rbfiles.path = /etc/tdriver/visualizer
}
win32 {
    target.path = C:/tdriver/visualizer
    rbfiles.path = $$target.path
}

rbfiles.files += tdriver_interface.rb


INSTALLS += target rbfiles
