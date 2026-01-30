#!/bin/bash
set -ev

cp qwt/qwtconfig.pri.in qwt/qwtconfig.pri
cp src/gcconfig.pri.in src/gcconfig.pri
# required to use bison version higher than 3.7
sed -i "s|#\(QMAKE_MOVE = cp.*\)|\1|" src/gcconfig.pri
# make a release build
sed -i "s|#\(CONFIG += release.*\)|\1 static|" src/gcconfig.pri
sed -i "s|^#QMAKE_CXXFLAGS|QMAKE_CXXFLAGS|" src/gcconfig.pri
# Enable -lz
sed -i "s|^#LIBZ_LIBS|LIBZ_LIBS|" src/gcconfig.pri
# ICAL
sed -i "s|#\(ICAL_INSTALL =.*\)|\1 /usr|" src/gcconfig.pri
# LIBUSB
sed -i "s|#\(LIBUSB_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri
sed -i "s|#\(LIBUSB_LIBS    =.*\)|\1 -lusb-1.0 -ldl -ludev|" src/gcconfig.pri
sed -i "s|#\(LIBUSB_USE_V_1 = true.*\)|\1|" src/gcconfig.pri
# VIDEO
sed -i "s|\(DEFINES += GC_VIDEO_NONE.*\)|#\1 |" src/gcconfig.pri
sed -i "s|#\(DEFINES += GC_VIDEO_QT6.*\)|\1|" src/gcconfig.pri
# HTTP Server
sed -i "s|^#HTPATH|HTPATH|" src/gcconfig.pri
# R
sed -i "s|#\(DEFINES += GC_WANT_R.*\)|\1|" src/gcconfig.pri
# Enable CloudDB
sed -i "s|^#CloudDB|CloudDB|" src/gcconfig.pri
# D2XX
sed -i "s|#\(D2XX_INCLUDE =.*\)|\1 ../D2XX/release|" src/gcconfig.pri
# SAMPLERATE
sed -i "s|#\(SAMPLERATE_INSTALL =\).*|\1 /usr|" src/gcconfig.pri
# SRMIO
sed -i "s|#\(SRMIO_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri
# Python
sed -i "s|#\(DEFINES += GC_WANT_PYTHON\).*|\1|" src/gcconfig.pri
sed -i "s|python3-config|python${PYTHON_VERSION}-config|" src/gcconfig.pri
# GSL
sed -i "s|#GSL_LIBS =.*|GSL_LIBS = -lgsl -lgslcblas -lm|" src/gcconfig.pri

# TrainerDay Query API
echo DEFINES += GC_WANT_TRAINERDAY_API >> src/gcconfig.pri
echo DEFINES += GC_TRAINERDAY_API_PAGESIZE=25 >> src/gcconfig.pri
cat src/gcconfig.pri

# update translations
lupdate src/src.pro
exit
