QT += testlib network core

HEADERS += ../../../src/Cloud/WahooClient.h \
           ../mockNetworkReply.h

SOURCES += testWahoo.cpp \
           ../../../src/Cloud/WahooClient.cpp

include(../../unittests.pri)

INCLUDEPATH += $$GC_SRC_DIR/Cloud/
INCLUDEPATH += ../