#!/bin/bash

set -x
set -e


# Add recent Qt dependency ppa
# Update this file on a newer qt version.
sudo add-apt-repository ppa:beineri/opt-qt595-trusty -y
sudo apt update -qq
sudo apt install -y qt59base qt59tools qt59serialport qt59svg\
 qt59multimedia qt59script qt59connectivity qt59webengine qt59charts-no-lgpl\
 flex bison git qt5-default

source /opt/qt*/bin/qt*-env.sh
#export QTDIR=/opt/qt59
#export PATH=/opt/qt59/bin:$PATH
#export LD_LIBRARY_PATH=/opt/qt59/lib/x86_64-linux-gnu:/opt/qt59/lib:$LD_LIBRARY_PATH
#export PKG_CONFIG_PATH=/opt/qt59/lib/pkgconfig:$PKG_CONFIG_PATH

# Temporarily alias ldrelease
#ln -s $QTDIR/bin/ldrelease-qt5 $QTDIR/bin/ldrelease-qt4
