#!/usr/bin/env bash
# shellcheck enable=all
# shellcheck shell=bash

set -Eeuf -o pipefail

main() {
  cp qwt/qwtconfig.pri{.in,}

  local GSL_PATH
  GSL_PATH=$(brew --prefix gsl)
  sed < src/gcconfig.pri.in > src/gcconfig.pri -e '
   # GSL
   s|#GSL_INCLUDES =.*|GSL_INCLUDES = '"${GSL_PATH}"'/include|
   s|#GSL_LIBS =.*|GSL_LIBS = -L'"${GSL_PATH}"'/lib -lgsl -lgslcblas -lm|

   s|#\(CONFIG += release.*\)|\1 static|
   s|^#CloudDB|CloudDB|
   s|^#LIBZ|LIBZ|

   # SRMIO
   s|#\(SRMIO_INSTALL =.*\)|\1 /opt/homebrew|

   # D2XX
   # this requires one additional change to src/FileIO/D2XX.cpp
   # which is done below
   s|#\(D2XX_INCLUDE =.*\)|\1 ../D2XX|
   s|#\(D2XX_LIBS    =.*\)|\1 -L../D2XX -lftd2xx|

   # ICAL
   #s|#\(ICAL_INSTALL =.*\)|\1 /opt/homebrew|
   #s|#\(ICAL_LIBS    =.*\)|\1 -L/opt/homebrew/lib -lical|

   # LIBUSB
   s|#\(LIBUSB_INSTALL =\).*|\1 /opt/homebrew|
   s|#\(LIBUSB_LIBS    =.*\)|\1 -L/opt/homebrew/lib -lusb-1.0|
   s|#\(LIBUSB_USE_V_1 = true.*\)|\1|

   # SAMPLERATE
   s|#\(SAMPLERATE_INSTALL =\).*|\1 /opt/homebrew|
   s|#\(SAMPLERATE_LIBS =\).*|\1 -L/opt/homebrew/lib -lsamplerate|

   # LMFIT
   s|#\(LMFIT_INSTALL =\).*|\1 /opt/homebrew|

   s|#\(DEFINES += GC_HAVE_LION*\)|\1|

   # HTTP Server
   s|#\(HTPATH = ../httpserver.*\)|\1|

   # Robot
   s|#\(DEFINES += GC_WANT_ROBOT.*\)|\1|

   # Qt6 VIDEO
   s|\(DEFINES += GC_VIDEO_NONE.*\)|#\1|
   s|#\(DEFINES += GC_VIDEO_QT6.*\)|\1|

   # Enable R embedding
   s|#\(DEFINES += GC_WANT_R.*\)|\1|

   # Python (avoiding collision between GC Context.h and Python context.h)
   s|#\(DEFINES += GC_WANT_PYTHON\)\.*|\1|
   '

  {
    # Bison
    echo 'QMAKE_YACC=/opt/homebrew/opt/bison/bin/bison'
    echo 'QMAKE_MOVE = cp'

    # TrainerDay Query API
    echo 'DEFINES += GC_WANT_TRAINERDAY_API'
    echo 'DEFINES += GC_TRAINERDAY_API_PAGESIZE=25'

    # macOS version config
    echo 'QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -arch arm64'
    echo 'QMAKE_CFLAGS_RELEASE += -mmacosx-version-min=10.7 -arch arm64'
    echo 'QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15'

    # Enable compress library
    echo 'LIBZ_LIBS = -lz'

    # Define GC version string, only for tagged builds
    if [[ "${GITHUB_REF_TYPE:-}" = tag ]]; then
      echo 'DEFINES += GC_VERSION=VERSION_STRING'
    fi
  } >> src/gcconfig.pri

  sed -i '' -e 's|libftd2xx.dylib|@executable_path/../Frameworks/libftd2xx.1.4.24.dylib|' src/FileIO/D2XX.cpp
}
main "$@"
