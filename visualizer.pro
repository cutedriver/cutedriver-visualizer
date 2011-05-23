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


TEMPLATE = subdirs

# header files and data files used by other subprojects, for example version.h, but no dynamic libraries
SUBDIRS += common

# library of independent utility classes, such as localization string database access
SUBDIRS += libtdriverutil

# library which implements code editor and ruby debugger widgets
SUBDIRS += libtdrivereditor

# library which implements feature test viewer/editor
SUBDIRS += libtdriverfeatureditor

# minimal wrapper around libtdrivereditor to create a standalone code editor application, mostly for testing
#SUBDIRS += editor_proto

# full Testability Driver Visualizer application
SUBDIRS += tdriver_editor

# Testability Driver fixture for tdriver_editor, for running feature tests
SUBDIRS += fixtures

CONFIG += ordered
