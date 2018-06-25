#!/bin/bash

set -x
set -e


# Add recent Qt dependency ppa
# Update this file on a newer qt version.
sudo add-apt-repository ppa:beineri/opt-qt595-trusty -y
sudo apt update -qq
sudo apt install -y qt59base qt59tools qt59serialport qt59svg\
 qt59multimedia qt59script qt59connectivity qt59webengine qt59charts-no-lgpl\
 flex bison git qt5-default libical-dev

#source /opt/qt*/bin/qt*-env.sh
export QTDIR=/opt/qt59
export PATH=/opt/qt59/bin:$PATH
export LD_LIBRARY_PATH=/opt/qt59/lib/x86_64-linux-gnu:/opt/qt59/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/opt/qt59/lib/pkgconfig:$PKG_CONFIG_PATH

# Temporarily alias ldrelease
# sudo ln -s $QTDIR/bin/lrelease $QTDIR/bin/lrelease-qt4

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
