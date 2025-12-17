QT += testlib
QT -= gui

TARGET = testSignalSafety
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

include(../../unittests.pri)

SOURCES += main.cpp
