## Copyright 2016 The University of Oklahoma.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

QT       += core gui network widgets qml quick quickcontrols2 webengine quickwidgets
CONFIG   += c++11

TARGET = research_control
TEMPLATE = app

BUILD_DIR = ../build/research_control
DESTDIR = ../bin
OBJECTS_DIR = $$BUILD_DIR
MOC_DIR = $$BUILD_DIR
RCC_DIR = $$BUILD_DIR
UI_DIR = $$BUILD_DIR
PRECOMPILED_DIR = $$BUILD_DIR

SOURCES += main.cpp\
    researchprocess.cpp \
    researchmainwindow.cpp \
    settingsmodel.cpp \
    hudpowerimpl.cpp \
    hudlatencygraphimpl.cpp \
    hudorientationsideimpl.cpp \
    hudorientationbackimpl.cpp \
    abstracthudorientationimpl.cpp \
    latencycsvseries.cpp \
    commentcsvseries.cpp \
    connectioneventcsvseries.cpp

HEADERS  += \
    researchprocess.h \
    researchmainwindow.h \
    settingsmodel.h \
    hudpowerimpl.h \
    hudlatencygraphimpl.h \
    hudorientationsideimpl.h \
    hudorientationbackimpl.h \
    abstracthudorientationimpl.h \
    latencycsvseries.h \
    commentcsvseries.h \
    connectioneventcsvseries.h

FORMS    += \
    researchmainwindow.ui

RESOURCES += \
    qml.qrc \
    assets.qrc

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../..

LIBS += -lSDL2 -lQt5GStreamer-1.0 -lQt5GLib-2.0 -lQt5GStreamerUi-1.0 -lQt5GStreamerUtils-1.0 -lQt5GStreamerQuick-1.0
#LIBS += -lflycapture
LIBS += -L../lib -lsoro
LIBS += -L../lib -lsoromc
LIBS += -L../lib -lsorogst

DISTFILES +=
