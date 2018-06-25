#!/bin/sh

# Build GoldenCheetah

cd qwt
cp qwtconfig.pri.in qwtconfig.pri
cd ../src
cp gcconfig.pri.in gcconfig.pri


# Can we enable optimizations in the appimage?
#QMAKE_CXXFLAGS += -O3

source /opt/qt*/bin/qt*-env.sh; qmake -recursive; make
