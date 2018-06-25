#!/bin/sh

# Build GoldenCheetah

set -x
set -e

cd qwt
cp qwtconfig.pri.in qwtconfig.pri
cd ../src
cp gcconfig.pri.in gcconfig.pri
cd ..

# Can we enable optimizations in the appimage?
#QMAKE_CXXFLAGS += -O3

# Disable webkit, and enable webengine?
if [[ "$WEBKIT" == "0" ]]; then echo DEFINES += NOWEBKIT >> src/gcconfig.pri; fi

qmake -recursive
make
