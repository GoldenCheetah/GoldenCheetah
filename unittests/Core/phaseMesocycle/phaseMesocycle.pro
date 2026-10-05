QT += testlib widgets

SOURCES = testPhaseMesocycle.cpp
GC_OBJS = Season \
          Utils

include(../../unittests.pri)
