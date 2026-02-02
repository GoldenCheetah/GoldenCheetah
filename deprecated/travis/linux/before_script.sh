#!/bin/bash
set -ev

cp qwt/qwtconfig.pri.in qwt/qwtconfig.pri
cp src/gcconfig.pri.in src/gcconfig.pri
# Define GC version string, only for tagged builds
if [ -n "$TRAVIS_TAG" ]; then echo DEFINES += GC_VERSION=VERSION_STRING >> src/gcconfig.pri; fi
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
# VLC & VIDEO
sed -i "s|#\(VLC_INSTALL =.*\)|\1 /usr|" src/gcconfig.pri
sed -i "s|#\(VLC_LIBS    =.*\)|\1 -lvlc|" src/gcconfig.pri
sed -i "s|^#HTPATH|HTPATH|" src/gcconfig.pri
sed -i "s|\(DEFINES += GC_VIDEO_NONE.*\)|#\1 |" src/gcconfig.pri
sed -i "s|#\(DEFINES += GC_VIDEO_VLC.*\)|\1|" src/gcconfig.pri
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
sed -i "s/|#GC_WANT_PYTHON.=.*|GC_WANT_PYTHON=true|" src/gcconfig.pri
# GSL
sed -i "s|#GSL_INCLUDES =.*|GSL_INCLUDES = /usr/include|" src/gcconfig.pri
echo GSL_LIBS = -lgsl -lgslcblas -lm >> src/gcconfig.pri
# TrainerDay Query API
echo DEFINES += GC_WANT_TRAINERDAY_API >> src/gcconfig.pri
echo DEFINES += GC_TRAINERDAY_API_PAGESIZE=25 >> src/gcconfig.pri

# Patch Secrets.h
sed -i "s/__GC_GOOGLE_CALENDAR_CLIENT_SECRET__/"$GC_GOOGLE_CALENDAR_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_STRAVA_CLIENT_SECRET__/"$GC_STRAVA_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_DROPBOX_CLIENT_SECRET__/"$GC_DROPBOX_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_CYCLINGANALYTICS_CLIENT_SECRET__/"$GC_CYCLINGANALYTICS_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_TWITTER_CONSUMER_SECRET__/"$GC_TWITTER_CONSUMER_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_DROPBOX_CLIENT_ID__/"$GC_DROPBOX_CLIENT_ID"/" src/Core/Secrets.h
sed -i "s/__GC_MAPQUESTAPI_KEY__/"$GC_MAPQUESTAPI_KEY"/" src/Core/Secrets.h
sed -i "s/__GC_CLOUD_DB_BASIC_AUTH__/"$GC_CLOUD_DB_BASIC_AUTH"/" src/Core/Secrets.h
sed -i "s/__GC_CLOUD_DB_APP_NAME__/"$GC_CLOUD_DB_APP_NAME"/" src/Core/Secrets.h
sed -i "s/__GC_GOOGLE_DRIVE_CLIENT_ID__/"$GC_GOOGLE_DRIVE_CLIENT_ID"/" src/Core/Secrets.h
sed -i "s/__GC_GOOGLE_DRIVE_CLIENT_SECRET__/"$GC_GOOGLE_DRIVE_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_GOOGLE_DRIVE_API_KEY__/"$GC_GOOGLE_DRIVE_API_KEY"/" src/Core/Secrets.h
sed -i "s/__GC_WITHINGS_CONSUMER_SECRET__/"$GC_WITHINGS_CONSUMER_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_NOKIA_CLIENT_SECRET__/"$GC_NOKIA_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_SPORTTRACKS_CLIENT_SECRET__/"$GC_SPORTTRACKS_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/OPENDATA_DISABLE/OPENDATA_ENABLE/" src/Core/Secrets.h
sed -i "s/__GC_CLOUD_OPENDATA_SECRET__/"$GC_CLOUD_OPENDATA_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_RWGPS_API_KEY__/"$GC_RWGPS_API_KEY"/" src/Core/Secrets.h
sed -i "s/__GC_POLARFLOW_CLIENT_SECRET__/"$GC_POLARFLOW_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_NOLIO_CLIENT_ID__/"$GC_NOLIO_CLIENT_ID"/" src/Core/Secrets.h
sed -i "s/__GC_NOLIO_SECRET__/"$GC_NOLIO_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_XERT_CLIENT_SECRET__/"$GC_XERT_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_AZUM_CLIENT_SECRET__/"$GC_AZUM_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_TRAINERDAY_API_KEY__/"$GC_TRAINERDAY_API_KEY"/" src/Core/Secrets.h
cat src/gcconfig.pri
# update translations
lupdate src/src.pro
exit
