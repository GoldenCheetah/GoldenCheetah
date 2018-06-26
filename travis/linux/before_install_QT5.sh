#!/bin/bash

set -x
set -e

# Add recent Qt dependency ppa
# Update this file on a newer qt version.
sudo add-apt-repository ppa:beineri/opt-qt595-trusty -y
sudo apt update -qq
sudo apt install -y qt5-default qt59base qt59tools qt59serialport qt59svg\
 qt59multimedia qt59script qt59connectivity qt59webengine qt59charts-no-lgpl\
 bison flex libboost-dev libcurl4-nss-dev zlib1g-dev libgcrypt-dev libnss3\
 libexpat1-dev libkml-dev libical-dev libusb-dev libvlc-dev libvlccore-dev\
 libssl-dev libpulse-dev zlib1g

cat /opt/qt*/bin/qt*-env.sh
source /opt/qt*/bin/qt*-env.sh
#export QTDIR=/opt/qt59
#export PATH=/opt/qt59/bin:$PATH
#export LD_LIBRARY_PATH=/opt/qt59/lib/x86_64-linux-gnu:/opt/qt59/lib:$LD_LIBRARY_PATH
#export PKG_CONFIG_PATH=/opt/qt59/lib/pkgconfig:$PKG_CONFIG_PATH

cd qwt
cp qwtconfig.pri.in qwtconfig.pri
cd ../src
cp gcconfig.pri.in gcconfig.pri

# Add ICAL Settings
sed -i -e "s|#ICAL_INSTALL =|ICAL_INSTALL = /usr/include|g" gcconfig.pri
sed -i -e "s|#ICAL_LIBS    =|ICAL_LIBS    = -lical|g" gcconfig.pri

# Add vlc Settings
sed -i -e "s|#VLC_INSTALL =|VLC_INSTALL = /usr/include/vlc/|g" gcconfig.pri

# Enable -lz
sed -i -e "s|#LIBZ_LIBS    = -lz|LIBZ_LIBS    = -lz|g" gcconfig.pri

cd ..

# Can we enable optimizations in the appimage?
#QMAKE_CXXFLAGS += -O3

# Disable webkit, and enable webengine?
if [[ "$WEBKIT" == "0" ]]; then echo DEFINES += NOWEBKIT >> src/gcconfig.pri; fi
