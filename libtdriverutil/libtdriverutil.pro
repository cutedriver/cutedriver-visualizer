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
QT += network \
    sql
QT -= gui
TARGET = tdriverutil
DESTDIR = ./
TEMPLATE = lib
CONFIG += shared
CONFIG += create_prl
DEFINES += LIBTDRIVERUTIL_LIBRARY
//DEFINES += QT_NO_CAST_FROM_ASCII
SOURCES += tdriver_translationdb.cpp \
    tdriver_util.cpp \
    tdriver_rubyinterface.cpp \
    tdriver_rbiprotocol.cpp
HEADERS += tdriver_translationdb.h \
    libtdriverutil_global.h \
    tdriver_util.h \
    tdriver_rubyinterface.h \
    tdriver_rbiprotocol.h \
    tdriver_debug_macros.h
OTHER_FILES += tdriver_interface.rb

target.files += $$PWD/tdriver_interface.rb

# install
unix:!symbian:target.path = /usr/lib
win32::target.path = C:/tdriver/visualizer
INSTALLS += target
