QT += testlib widgets xml

SOURCES = testSeasonParser.cpp
GC_OBJS = Seasons \
          moc_Seasons \
          Season \
          Utils

include(../../unittests.pri)
