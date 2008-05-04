
TEMPLATE = lib
TARGET = 
DEPENDPATH += .
INCLUDEPATH += /sw/include
LIBS += -lftd2xx
CONFIG += static debug

HEADERS += Device.h D2XX.h PowerTap.h
SOURCES += Device.cpp D2XX.cpp PowerTap.cpp

macx || unix {
    HEADERS += Serial.h
    SOURCES += Serial.cpp
}

