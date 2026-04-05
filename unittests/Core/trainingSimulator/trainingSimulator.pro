QT += testlib widgets

SOURCES = testTrainingSimulator.cpp
GC_OBJS = TrainingSimulator \
           TrainingConstraintChecker

include(../../unittests.pri)
