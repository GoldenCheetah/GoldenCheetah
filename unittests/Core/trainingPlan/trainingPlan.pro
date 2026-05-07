QT += testlib widgets core5compat sql xml network concurrent serialport positioning dbus \
      charts printsupport multimedia multimediawidgets webenginewidgets webenginequick \
      webenginecore quick opengl openglwidgets webchannel webchannelquick qml bluetooth svg

SOURCES = testTrainingPlan.cpp
GC_OBJS = TrainingPlan TrainingConstraintChecker BanisterSimulator

include(../../unittests.pri)
