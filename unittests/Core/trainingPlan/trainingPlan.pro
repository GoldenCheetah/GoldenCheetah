QT += testlib widgets

SOURCES = testTrainingPlan.cpp
GC_OBJS = TrainingPlan TrainingConstraintChecker BanisterSimulator

include(../../unittests.pri)
