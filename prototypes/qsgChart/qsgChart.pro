QT += core gui widgets quick quickwidgets
CONFIG += c++17
TARGET = qsgChart
TEMPLATE = app

SOURCES += main.cpp LineCurveItem.cpp AxisItem.cpp
HEADERS += LineCurveItem.h AxisItem.h
RESOURCES += qsgChart.qrc
