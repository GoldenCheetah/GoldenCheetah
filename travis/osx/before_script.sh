#!/bin/bash
set -ev

# Python 3.7.8
curl -O https://www.python.org/ftp/python/3.7.9/python-3.7.9-macosx10.9.pkg
sudo installer -pkg python-3.7.9-macosx10.9.pkg -target /

python3.7 --version
python3.7-config --prefix

# Python mandatory packages - refresh cache if folder is empty
if [ -z "$(ls -A site-packages)" ]; then
    python3.7 -m pip install -q -r src/Python/requirements.txt -t site-packages
fi

# Python SIP
curl -k -L -O https://sourceforge.net/projects/pyqt/files/sip/sip-4.19.8/sip-4.19.8.tar.gz
tar xf sip-4.19.8.tar.gz
cd sip-4.19.8
python3.7 configure.py
make -j4
sudo make install
cd ..

cp qwt/qwtconfig.pri.in qwt/qwtconfig.pri
cp src/gcconfig.pri.in src/gcconfig.pri
# Bison
echo QMAKE_YACC=/usr/local/opt/bison@2.7/bin/bison >> src/gcconfig.pri
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
sed -i "" "s|#\(ICAL_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(ICAL_LIBS    =.*\)|\1 -L/usr/local/lib -lical|" src/gcconfig.pri
sed -i "" "s|#\(LIBUSB_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(LIBUSB_LIBS    =.*\)|\1 -L/usr/local/lib -lusb-1.0|" src/gcconfig.pri
sed -i "" "s|#\(LIBUSB_USE_V_1 = true.*\)|\1|" src/gcconfig.pri
sed -i "" "s|#\(SAMPLERATE_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(SAMPLERATE_LIBS =\).*|\1 -L/usr/local/lib -lsamplerate|" src/gcconfig.pri
sed -i "" "s|#\(LMFIT_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(DEFINES += GC_HAVE_LION*\)|\1|" src/gcconfig.pri
sed -i "" "s|#\(HTPATH = ../httpserver.*\)|\1 |" src/gcconfig.pri
sed -i "" "s|#\(DEFINES += GC_WANT_ROBOT.*\)|\1 |" src/gcconfig.pri
# VLC & VIDEO
sed -i "" "s|#\(VLC_INSTALL =.*\)|\1 ../VLC|" src/gcconfig.pri
sed -i "" "s|\(DEFINES += GC_VIDEO_NONE.*\)|#\1 |" src/gcconfig.pri
sed -i "" "s|#\(DEFINES += GC_VIDEO_VLC.*\)|\1 |" src/gcconfig.pri
# Enable R embedding
sed -i "" "s|#\(DEFINES += GC_WANT_R.*\)|\1 |" src/gcconfig.pri
# Python (avoiding collision between GC Context.h and Python context.h)
echo DEFINES += GC_WANT_PYTHON >> src/gcconfig.pri
echo PYTHONINCLUDES = -ICore `python3.7-config --includes` >> src/gcconfig.pri
echo PYTHONLIBS = `python3.7-config --ldflags` >> src/gcconfig.pri
# GSL
echo GSL_LIBS = -lgsl -lgslcblas -lm >> src/gcconfig.pri
# TrainerDay Query API
echo DEFINES += GC_WANT_TRAINERDAY_API >> src/gcconfig.pri
# macOS version config
echo "QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -arch x86_64" >> src/gcconfig.pri
echo "QMAKE_CFLAGS_RELEASE += -mmacosx-version-min=10.7 -arch x86_64" >> src/gcconfig.pri
echo "QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15" >> src/gcconfig.pri

# Patch Secrets.h
sed -i "" "s/__GC_GOOGLE_CALENDAR_CLIENT_SECRET__/"$GC_GOOGLE_CALENDAR_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_STRAVA_CLIENT_SECRET__/"$GC_STRAVA_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_DROPBOX_CLIENT_SECRET__/"$GC_DROPBOX_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_CYCLINGANALYTICS_CLIENT_SECRET__/"$GC_CYCLINGANALYTICS_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_TWITTER_CONSUMER_SECRET__/"$GC_TWITTER_CONSUMER_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_DROPBOX_CLIENT_ID__/"$GC_DROPBOX_CLIENT_ID"/" src/Core/Secrets.h
sed -i "" "s/__GC_MAPQUESTAPI_KEY__/"$GC_MAPQUESTAPI_KEY"/" src/Core/Secrets.h
sed -i "" "s/__GC_CLOUD_DB_BASIC_AUTH__/"$GC_CLOUD_DB_BASIC_AUTH"/" src/Core/Secrets.h
sed -i "" "s/__GC_CLOUD_DB_APP_NAME__/"$GC_CLOUD_DB_APP_NAME"/" src/Core/Secrets.h
sed -i "" "s/__GC_GOOGLE_DRIVE_CLIENT_ID__/"$GC_GOOGLE_DRIVE_CLIENT_ID"/" src/Core/Secrets.h
sed -i "" "s/__GC_GOOGLE_DRIVE_CLIENT_SECRET__/"$GC_GOOGLE_DRIVE_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_GOOGLE_DRIVE_API_KEY__/"$GC_GOOGLE_DRIVE_API_KEY"/" src/Core/Secrets.h
sed -i "" "s/__GC_WITHINGS_CONSUMER_SECRET__/"$GC_WITHINGS_CONSUMER_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_NOKIA_CLIENT_SECRET__/"$GC_NOKIA_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_SPORTTRACKS_CLIENT_SECRET__/"$GC_SPORTTRACKS_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/OPENDATA_DISABLE/OPENDATA_ENABLE/" src/Core/Secrets.h
sed -i "" "s/__GC_CLOUD_OPENDATA_SECRET__/"$GC_CLOUD_OPENDATA_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_NOLIO_CLIENT_ID__/"$GC_NOLIO_CLIENT_ID"/" src/Core/Secrets.h
sed -i "" "s/__GC_NOLIO_SECRET__/"$GC_NOLIO_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_RWGPS_API_KEY__/"$GC_RWGPS_API_KEY"/" src/Core/Secrets.h
sed -i "" "s/__GC_XERT_CLIENT_SECRET__/"$GC_XERT_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_AZUM_CLIENT_SECRET__/"$GC_AZUM_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "" "s/__GC_TRAINERDAY_API_KEY__/"$GC_TRAINERDAY_API_KEY"/" src/Core/Secrets.h
cat src/gcconfig.pri
# update translations
/usr/local/opt/qt5/bin/lupdate src/src.pro
exit
