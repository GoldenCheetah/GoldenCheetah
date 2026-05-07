QT += testlib widgets

SOURCES = testTtsReader.cpp
GC_OBJS = TTSReader LocationInterpolation BlinnSolver

include(../../unittests.pri)
