QT += testlib widgets

SOURCES = testSeason.cpp
GC_OBJS = Season \
          Utils

include(../unittests.pri)
