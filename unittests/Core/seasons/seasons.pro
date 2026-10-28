QT += testlib widgets xml

SOURCES = testSeasons.cpp
GC_OBJS = Seasons \
          moc_Seasons \
          Season \
          Utils

include(../../unittests.pri)
