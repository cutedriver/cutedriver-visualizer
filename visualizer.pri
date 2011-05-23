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

# This file assumes it's being included by first level subdir .pro

EDITORLIBDIR = ../libtdrivereditor
EDITOR_LIB = tdrivereditor

FEATUREDITORLIBDIR = ../libtdriverfeatureditor
FEATUREDITOR_LIB = tdriverfeatureditor

UTILLIBDIR = ../libtdriverutil
UTIL_LIB = tdriverutil

EDITOR_PROTO_DIR=../editor_proto

DESTDIR = ../bin
QMAKE_LIBDIR = $$DESTDIR $$QMAKE_LIBDIR

CONFIG(no_sql) {
    DEFINES *= TDRIVER_NO_SQL
}
