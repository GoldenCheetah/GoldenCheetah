QT += testlib
QT += gui

TARGET = testSplineCrash
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

include(../../unittests.pri)

INCLUDEPATH += ../../../qwt/src
SOURCES += testSplineCrash.cpp \
           ../../../src/Core/SplineLookup.cpp
