#!/bin/bash
set -ev

# Get config
cp qwt/qwtconfig.pri.in qwt/qwtconfig.pri
cp src/gcconfig.pri.in src/gcconfig.pri

# Bison
echo QMAKE_YACC=/usr/local/opt/bison@2.7/bin/bison >> src/gcconfig.pri

# Enable compress library
echo LIBZ_LIBS = -lz >> src/gcconfig.pri

# GSL
GSL_PATH=$(brew --prefix gsl)
sed -i "" "s|#GSL_INCLUDES =.*|GSL_INCLUDES = ${GSL_PATH}/include|" src/gcconfig.pri
sed -i "" "s|#GSL_LIBS =.*|GSL_LIBS = -L${GSL_PATH}/lib -lgsl -lgslcblas -lm|" src/gcconfig.pri

# Define GC version string, only for tagged builds
if [ -n "$TRAVIS_TAG" ]; then echo DEFINES += GC_VERSION=VERSION_STRING >> src/gcconfig.pri; fi

sed -i "" "s|#\(CONFIG += release.*\)|\1 static |" src/gcconfig.pri
sed -i "" "s|^#CloudDB|CloudDB|" src/gcconfig.pri
sed -i "" "s|^#LIBZ|LIBZ|" src/gcconfig.pri

# SRMIO
sed -i "" "s|#\(SRMIO_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri

# D2XX
sed -i "" "s|libftd2xx.dylib|@executable_path/../Frameworks/libftd2xx.1.4.24.dylib|" src/FileIO/D2XX.cpp
sed -i "" "s|#\(D2XX_INCLUDE =.*\)|\1 ../D2XX|" src/gcconfig.pri
sed -i "" "s|#\(D2XX_LIBS    =.*\)|\1 -L../D2XX -lftd2xx|" src/gcconfig.pri

# ICAL
sed -i "" "s|#\(ICAL_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(ICAL_LIBS    =.*\)|\1 -L/usr/local/lib -lical|" src/gcconfig.pri

# LIBUSB
sed -i "" "s|#\(LIBUSB_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(LIBUSB_LIBS    =.*\)|\1 -L/usr/local/lib -lusb-1.0|" src/gcconfig.pri
sed -i "" "s|#\(LIBUSB_USE_V_1 = true.*\)|\1|" src/gcconfig.pri

# SAMPLERATE
sed -i "" "s|#\(SAMPLERATE_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(SAMPLERATE_LIBS =\).*|\1 -L/usr/local/lib -lsamplerate|" src/gcconfig.pri

# LMFIT
sed -i "" "s|#\(LMFIT_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri

sed -i "" "s|#\(DEFINES += GC_HAVE_LION*\)|\1|" src/gcconfig.pri

# HTTP Server
sed -i "" "s|#\(HTPATH = ../httpserver.*\)|\1 |" src/gcconfig.pri

# Robot
sed -i "" "s|#\(DEFINES += GC_WANT_ROBOT.*\)|\1 |" src/gcconfig.pri

# Qt6 VIDEO
sed -i "" "s|\(DEFINES += GC_VIDEO_NONE.*\)|#\1 |" src/gcconfig.pri
sed -i "" "s|#\(DEFINES += GC_VIDEO_QT6.*\)|\1 |" src/gcconfig.pri

# Enable R embedding
sed -i "" "s|#\(DEFINES += GC_WANT_R.*\)|\1 |" src/gcconfig.pri

# Python (avoiding collision between GC Context.h and Python context.h)
sed -i "" "s|#\(DEFINES += GC_WANT_PYTHON\)\.*|\1 |" src/gcconfig.pri

# TrainerDay Query API
echo DEFINES += GC_WANT_TRAINERDAY_API >> src/gcconfig.pri
echo DEFINES += GC_TRAINERDAY_API_PAGESIZE=25 >> src/gcconfig.pri

# macOS version config
echo "QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -arch x86_64" >> src/gcconfig.pri
echo "QMAKE_CFLAGS_RELEASE += -mmacosx-version-min=10.7 -arch x86_64" >> src/gcconfig.pri
echo "QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15" >> src/gcconfig.pri

cat src/gcconfig.pri

# update translations
lupdate src/src.pro

exit
