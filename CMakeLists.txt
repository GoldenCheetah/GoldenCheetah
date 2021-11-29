project (GoldenCheetah)

# See http://doc.qt.io/qt-5/cmake-manual.html
cmake_minimum_required(VERSION 3.4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Grab the version number from the git repo.
#
# http://brianmilco.blogspot.no/2012/11/cmake-automatically-use-git-tags-as.html
include(GetGitRevisionDescription)
#git_describe(VERSION --tags --dirty=-dirty)
git_describe(VERSION tag)

# Parse the version information into pieces (version tags must have the format v0.3.4)
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${VERSION}")
set(GC_VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

#get_git_head_revision(GIT_REFSPEC GIT_SHA1)

message(STATUS "GoldenCheetah version: ${GC_VERSION_SHORT} [${VERSION_SHA1}]")

###############################################################################
#                                                                             #
#              ***  SECTION ONE - CORE CONFIGURATION  ***                     #
#                                                                             #
#                                                                             #
###############################################################################

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# GNU Scientific Library
find_package(GSL REQUIRED)

find_package(LMFIT REQUIRED)

###========================================================================
### QT5.14.2 officially supported which mandates c++11 support in toolchain
###========================================================================

find_package (Qt5Widgets 5.8 REQUIRED)
find_package (Qt5Core REQUIRED)
find_package (Qt5Xml REQUIRED)
find_package (Qt5Sql REQUIRED)
find_package (Qt5Network REQUIRED)
find_package (Qt5Svg REQUIRED)
find_package (Qt5Concurrent REQUIRED)
find_package (Qt5SerialPort REQUIRED)
find_package (Qt5MultimediaWidgets REQUIRED)
find_package (Qt5PrintSupport REQUIRED)

if (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.6) # We require Qt >= 5.8 so we could skip the 2nd test here (?).
  set (CMAKE_CXX_STANDARD 11)
  find_package (Qt5WebEngine REQUIRED)
  find_package (Qt5WebEngineCore REQUIRED)
  find_package (Qt5WebEngineWidgets REQUIRED)
  find_package (Qt5WebChannel REQUIRED)
  find_package (Qt5Positioning REQUIRED)
else (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.6)
  # On 5.5 or earlier we can still use WebKit
  find_package (Qt5WebKitWidgets)
endif (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.6)

set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

message (STATUS "Qt version: ${Qt5Widgets_VERSION}")


###=======================================================================
### Directory Structure - Split into subdirs to be more manageable
###=======================================================================

set (GC_INCLUDE_DIRS
  src/ANT
  src/Charts
  src/Cloud
  src/Core
  src/FileIO
  src/Gui
  src/Metrics
  src/Planning
  src/Python
  src/R
  src/Resources
  src/Train
  )

###=======================================================================
### DISTRIBUTED SOURCE [Snaffled in sources to avoid further dependencies]
###=======================================================================

set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "qwt/src")

set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/qxt/src")
set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/qtsolutions/json")
set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/qtsolutions/qwtcurve")
set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/lmfit")
set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/levmar")
set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/boost")
set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/kmeans")
set (GC_INCLUDE_DIRS  ${GC_INCLUDE_DIRS} "contrib/voronoi")

#
# Qwt
#

# We could 1) write a CMakeFile.txt for building the local (static) Qwt or 2) just add the sources to GC
#
# 1)
#add_subdirectory("qwt/src")
#
# 2) Add Qwt sources
file (GLOB_RECURSE QWT_HEADER_FILES RELATIVE ${CMAKE_SOURCE_DIR} qwt/src/*.h)
file (GLOB_RECURSE QWT_SOURCE_FILES RELATIVE ${CMAKE_SOURCE_DIR} qwt/src/*.cpp)

#
# Qxt
#

# Only some of (the now unmaintained?) Qxt files is used by GC, see "Contributed solutions" further down.
# Hence, do not include all Qxt sources (which don't build with Qt 5?)
#file (GLOB_RECURSE QXT_HEADER_FILES RELATIVE ${CMAKE_SOURCE_DIR} qxt/src/*.h)
#file (GLOB_RECURSE QXT_SOURCE_FILES RELATIVE ${CMAKE_SOURCE_DIR} qxt/src/*.cpp)
add_definitions( -DQXT_STATIC )

# Compress and math libs must be defined in gcconfig.pri
# if they're not part of the QT include
#INCLUDEPATH += $${LIBZ_INCLUDE}
#LIBS += $${LIBZ_LIBS}

find_package (ZLIB REQUIRED)	# TODO do this only when needed.

set (GC_INCLUDE_DIRS ${GC_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})

###===============================
### PLATFORM SPECIFIC DEPENDENCIES
###===============================

#
# TODO move this to Windows.cmake Darwin.cmake, Linux.cmake etc.
# For now we just support Linux
#

# Linux gcc 5 grumbles about unused static globals and leads
# to a gazillion warnings that are harmless so lets remove them
#QMAKE_CXXFLAGS += -Wno-unused-variable

# Linux Flex compiler grumbles about unsigned comparisons
#QMAKE_CXXFLAGS += -Wno-sign-compare

# TODO
#RC_FILE = Resources/images/gc.icns

set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Gui/QTFullScreen.h
  src/Train/VideoWindow.h
  )

set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Gui/QTFullScreen.cpp
  src/Train/VideoWindow.cpp
  )

#### these are no longer non-mac only
#set (GC_HEADER_FILES ${GC_HEADER_FILES}
#  qtsolutions/segmentcontrol/qtsegmentcontrol.h
#  )

#set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
#  qtsolutions/segmentcontrol/qtsegmentcontrol.cpp
#  )

###=================
### LANGUAGE SUPPORT
###=================

# To compile translation you need QT tool - lrelease
# If it is not found using he defaults in src/src.pro
# then set the full path and filename here.
#QMAKE_LRELEASE = /usr/bin/lrelease
find_program (LRELEASE NAMES lrelease REQUIRED)
if (LRELEASE)
  message (STATUS "Using lrelease: ${LRELEASE}")
endif (LRELEASE)

find_package (Qt5LinguistTools REQUIRED)

# set (GC_TS_FILES
#   src/Resources/translations/gc_fr.ts
#   src/Resources/translations/gc_ja.ts
#   src/Resources/translations/gc_it.ts
#   src/Resources/translations/gc_pt-br.ts
#   src/Resources/translations/gc_de.ts
#   src/Resources/translations/gc_cs.ts
#   src/Resources/translations/gc_es.ts
#   src/Resources/translations/gc_pt.ts
#   src/Resources/translations/gc_ru.ts
#   src/Resources/translations/gc_zh-cn.ts
#   src/Resources/translations/gc_zh-tw.ts
#   src/Resources/translations/gc_nl.ts
#   src/Resources/translations/gc_sv.ts
#   )

# This sort-of works but now we put generated files outside the build folder.
#set_source_files_properties(${GC_TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/../src/Resources/translations/)

# To avoid generating files outside the build folder just copy the Resources folder to the CMAKE_CURRENT_BINARY_DIR.
file (COPY "src/Resources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file (GLOB GC_TS_FILES "${CMAKE_CURRENT_BINARY_DIR}/Resources/translations/*.ts")
set_source_files_properties(${GC_TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)

qt5_add_translation (GC_GM_FILES ${GC_TS_FILES})

# Need lrelease to generate qm files
#if (NOT LRELEASE)
#
#  if (WIN32)
#    set (LRELEASE = ${QT_INSTALL_BINS}\\lrelease.exe)
#  endif (WIN32)
#
#  if (UNIX AND NOT APPLE)
#    set (LRELEASE  ${QT_INSTALL_BINS}/lrelease-qt4)
#  else (UNIX AND NOT APPLE)
#    set (LRELEASE  ${QT_INSTALL_BINS}/lrelease)
#  endif (UNIX AND NOT APPLE)
#
#endif (NOT LRELEASE)

# How to run lrelease
#if  (NOT TS_DIR)
#  set (TS_DIR "${PWD}/Resources/translations")
#  set (TSQM.name "lrelease ${QMAKE_FILE_IN}")
#  set (TSQM.input ${TRANSLATIONS})
#  set (TSQM.output ${TS_DIR}/${QMAKE_FILE_BASE}.qm)
#  set (TSQM.commands  "${QMAKE_LRELEASE} ${QMAKE_FILE_IN} -qm ${TS_DIR}/${QMAKE_FILE_BASE}.qm")
#  set (TSQM.CONFIG  no_link target_predeps)
#  set (QMAKE_EXTRA_COMPILERS ${QMAKE_EXTRA_COMPILERS} TSQM)
#endif  (NOT TS_DIR)

###==========
### RESOURCES
###==========

set (GC_RESOURCES
  ${GC_GM_FILES}
  ${CMAKE_CURRENT_BINARY_DIR}/Resources/application.qrc ${CMAKE_CURRENT_BINARY_DIR}/Resources/application.qrc
  )

#set (GC_RESOURCES - deprecated
#  ${GC_GM_FILES}
#  src/Resources/application.qrc src/Resources/RideWindow.qrc
#  )

###############################################################################
#                                                                             #
#         ***  SECTION TWO - OPTIONAL LIBRARIES AND FEATURES  ***             #
#                                                                             #
#                                                                             #
###############################################################################

option (USE_PYTHON "Enable if you want Python charting / ML etc" OFF)

option (USE_R "Enable if you want an R chart" OFF)

# uncomment below for R integration via webservices
#HTPATH = ../httpserver

option (USE_SRMIO "Enable if you want SRM download support" OFF)
if (USE_SRMIO)
  option (DOWNLOAD_SRMIO "Enable if you want to download, build, and install the SRMIO lib." OFF)
endif (USE_SRMIO)

option (USE_KQOAUTH "Enable if you want Twitter support" OFF)

# If you want support for Google Earth .kml files then you need
# to install the Google libkml library
#
# http://code.google.com/p/libkml/
# or on Linux sudo apt-get install libkml-dev
#
# then set the following variable appropriately
# to the root of the libs/include path
#KML_INSTALL = /usr/local
#KML_INCLUDE =
#KML_LIBS    =
# Since KML also requires BOOST you will need to install
# that too and then set BOOST_INCLUDE to that location
# If the files are in /usr/include/boost then set
#BOOST_INCLUDE = /usr/include
#Additionally, on MAC the latest libs also need the following
#QMAKE_CFLAGS_X86_64 += -mmacosx-version-min=10.7
#QMAKE_CXXFLAGS_X86_64 = $$QMAKE_CFLAGS_X86_64
#LIBS += -lexpat -luriparser -lminizip

option (USE_KML "Enable if you want support for Google Earth .kml files" OFF)

option (USE_ICAL "Enable if you want support for downloading from iCal calendars" OFF)

# If you want support for using USB1 sticks in Train View on Windows
# then install the SiLabs USBXpress Software Development Kit (SDK)
#     http://www.silabs.com/products/mcu/Pages/USBXpress.aspx
# and set the following to the 'Host' directory within the SDK
# USBXPRESS_INSTALL = /c/SiLabs/MCU/USBXpress/USBXpress_API/Host/
# You may override the INCLUDE and LIB files if you like.
# You *must* define USBXPRESS_INSTALL to use this feature.
#USBXPRESS_INSTALL =
#USBXPRESS_INCLUDE =
#USBXPRESS_LIBS    =

option (USE_USBXPRESS "Enable if you want support for using USB1 sticks in Train View on Windows" OFF)

option (USE_LIBUSB "Enable if you want support for using USB2 sticks in Train View on Linux or Windows" OFF)

option (USE_VLC "Enable if you want video playback on training mode" OFF)

option (USE_SAMPLERATE "Enable if you want to resample with high fidelity" OFF)

# If your system has PKG_CONFIG, QT can use this to get dependent libraries.
# Add the names of libraries with PKG_CONFIG information.
# eg. PKGCONFIG = oauth libical
# Uncomment the following two lines to enable.
#CONFIG   += link_pkgconfig
#PKGCONFIG =

option (USE_HTPATH "Enable if you want R integration via webservices" OFF)

# Enable if you want a 'robot' to test realtime code without having
# to get on your trainer and ride

option (USE_ROBOT "Enable if you want 'robot' to test realtime code" OFF)

if (USE_ROBOT)
  add_definitions( -DGC_WANT_ROBOT )
endif (USE_ROBOT)

#if you have a version of mingw that properly provides
#the Dwmapi.h header then uncomment this line
#DEFINES += GC_HAVE_DWM

# USING THE DROPBOX API
# register your app at https://www.dropbox.com/developers/apps and
# make sure the redirect_uri for the app includes  the url:
# https://goldencheetah.github.io/blank.html and full permissions
# are granted. Then set the client # and secret in the two defines
# below replacing the xxxxxxxxxxxxxx with the actual values.
#DEFINES += GC_DROPBOX_CLIENT_ID=\\\"xxxxxxxxxxxxxxx\\\"
#DEFINES += GC_DROPBOX_CLIENT_SECRET=\\\"xxxxxxxxxxxxxxx\\\"

# if you have your own MAPQUEST KEY
#DEFINES += GC_MAPQUESTAPI_KEY=\\\"xxxxxxxxxxxxxxxxxxxxxx\\\"

# USING THE WITHINGS API (http://oauth.withings.com/api)
#DEFINES += GC_WITHINGS_CONSUMER_KEY=\\\"xxxxxxxxxxxxxxx\\\"
#DEFINES += GC_WITHINGS_CONSUMER_SECRET=\\\"xxxxxxxxxxxxxxx\\\"

option (USE_CloudDB "Enable if you want support for the GoldenCheetah/CloudDB feature (with a private CloudDB instance)" OFF)


###=========================
### OPTIONAL => Embed Python
###=========================

if (USE_PYTHON) # TODO: We also need NOWEBKIT=on here?

  find_package (PythonLibs 3.6 REQUIRED) # TODO: Is v 3.6 required?

  find_package(PythonInterp REQUIRED)
  #ind_package (SIP 4.19 REQUIRED)
  find_package (SIP REQUIRED)

  message (STATUS "Python libs:${PYTHON_LIBRARIES}")
  message (STATUS "Python include dir: ${PYTHON_INCLUDE_DIRS}")
  message (STATUS "Python lib version: ${PYTHONLIBS_VERSION_STRING}")

  message (STATUS "Using Python: ${PYTHON_EXECUTABLE}")
  message (STATUS "Python version: ${PYTHON_VERSION_STRING}")
  message (STATUS "SIP version: ${SIP_VERSION_STR}")

  if (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.8 AND SIP_VERSION_STR VERSION_GREATER_EQUAL 4.19)

    # TODO: Do we really need to check for Qt 4.8+ on GC 3?

    add_definitions( -DGC_WANT_PYTHON ) # TODO Is this one really needed?
    set (GC_INCLUDE_DIRS ${GC_INCLUDE_DIRS} "src/Python")

    add_definitions( -DSIP_STATIC_MODULE )
    set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
    set (GC_OPT_LIBS ${GC_OPT_LIBS} ${PYTHON_LIBRARIES})

    ## Python integration

    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/Python/PythonEmbed.h src/Charts/PythonChart.h src/Python/PythonSyntax.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Python/PythonEmbed.cpp src/Charts/PythonChart.cpp src/Python/PythonSyntax.cpp
      )

    ## Python SIP generated module

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Python/SIP/sipgoldencheetahBindings.cpp src/Python/SIP/sipgoldencheetahcmodule.cpp
      src/Python/SIP/Bindings.cpp
      )

    ## SIP type conversion

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Python/SIP/sipgoldencheetahQString.cpp
      src/Python/SIP/sipgoldencheetahPythonDataSeries.cpp
      )

    add_definitions( -DGC_HAVE_PYTHON )

  else (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.8 AND SIP_VERSION_STR VERSION_GREATER_EQUAL 4.19)

    message (STATUS "INFO: Embedded Python requires version QT >= 5.8 (no support for ${Qt5Widgets_VERSION}) and SIP 4.19+")

  endif(Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.8 AND SIP_VERSION_STR VERSION_GREATER_EQUAL 4.19)

endif (USE_PYTHON)

###====================
### OPTIONAL => Embed R
###====================

if (USE_R)

  # Only supports Linux and OSX until RInside and Rcpp support MSVC
  # This is not likely to be very soon, they are heavily dependant on GCC
  # see: http://dirk.eddelbuettel.com/blog/2011/03/25/#rinside_and_qt

  find_package (R REQUIRED)
  find_r_package (RInside REQUIRED)
  #install_r_package (RIn side)
  find_r_package (Rcpp REQUIRED)
  #install_r_package (Rcpp)

  if (R_EXEC)

    add_definitions( -DGC_WANT_R )

    # Path to R.h
    execute_process(COMMAND ${R_EXEC} "RHOME" RESULT_VARIABLE R_RES OUTPUT_VARIABLE R_HOME OUTPUT_STRIP_TRAILING_WHITESPACE)
    message (STATUS "RHOME: ${R_HOME}")
    set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${R_HOME}/include)

    # add R subdirectory to include path
    set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} "src/R")

    ## include headers and libraries for R
    #QMAKE_CXXFLAGS += $$system($$R_HOME/bin/R CMD config --cppflags)

    ## R has lots of compatibility headers for S and legacy R code we don't want
    add_definitions( -DSTRICT_R_HEADERS )

    ## R integration
    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/R/REmbed.h src/R/RTool.h src/R/RGraphicsDevice.h src/R/RSyntax.h src/R/RLibrary.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/R/REmbed.cpp src/R/RTool.cpp src/R/RGraphicsDevice.cpp src/R/RSyntax.cpp src/R/RLibrary.cpp
      )

    ## R based charts
    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/Charts/RChart.h src/Charts/RCanvas.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Charts/RChart.cpp src/Charts/RCanvas.cpp
      )

    ## For hardware accelerated scene rendering
    find_package(Qt5OpenGL)

  endif (R_EXEC)

endif (USE_R)

###====================
### OPTIONAL => KQOAUTH
###====================

if (USE_KQOAUTH)

  find_package (KQOAUTH REQUIRED) # TODO check for version>=0.98
  add_definitions( -DGC_HAVE_KQOAUTH )

  set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${KQOAUTH_INCLUDE_DIR})
  set (GC_OPT_LIBS ${GC_OPT_LIBS} ${KQOAUTH_LIBRARIES})

endif (USE_KQOAUTH)

###=======================================================
### OPTIONAL => D2XX FOR FTDI DRIVERS ON WINDOWS PLATFORMS
###=======================================================

if (USE_D2XX)

  #find_package (D2XX)

  #DEFINES     += GC_HAVE_D2XX
  #INCLUDEPATH += $${D2XX_INCLUDE}

  #!isEmpty(D2XX_LIBS) { LIBS += $${D2XX_LIBS} }
  #unix                { LIBS += -ldl }

  #HEADERS     += FileIO/D2XX.h
  #SOURCES     += FileIO/D2XX.cpp

endif (USE_D2XX)


###==================
### OPTIONAL => SRMIO
###==================

# http://www.zuto.de/project/srmio
# https://github.com/rclasen/srmio

if (USE_SRMIO)

  # Download, build, and install the SRMIO lib. You will be asked for the root
  # password by sudo to install the lib.
  if (DOWNLOAD_SRMIO)
    add_subdirectory(${CMAKE_SOURCE_DIR}/cmake/external/srmio srmio_build)
  endif (DOWNLOAD_SRMIO)

  find_package (SRMIO REQUIRED)

  set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${LIBSRMIO_INCLUDE_DIR})
  set (GC_OPT_LIBS ${GC_OPT_LIBS} ${LIBSRMIO_LIBRARIES})

  add_definitions( -DGC_HAVE_SRMIO )

  set (GC_HEADER_FILES ${GC_HEADER_FILES}
    src/FileIO/SrmDevice.h
    )

  set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
    src/FileIO/SrmDevice.cpp
    )

endif (USE_SRMIO)


###=====================================
### OPTIONAL => GOOGLE KML IMPORT EXPORT
###=====================================

# libkml currently don't build (on Linux)!?

# https://github.com/google/libkml

if (USE_KML)

  # we will work out the rest if you tell use where it is installed
  #isEmpty(KML_INCLUDE) { KML_INCLUDE = $${KML_INSTALL}/include }
  #isEmpty(KML_LIBS)    { KML_LIBS    = -L$${KML_INSTALL}/lib/ \
  #                                     -lkmldom -lkmlconvenience -lkmlengine -lkmlbase
  #}

  # on MS VS the linker wants /LTCG for libkmldom due to
  # "MSIL .netmodule or module compiled with /GL found"
  #win32-msvc* { QMAKE_LFLAGS +=  /LTCG }

  #DEFINES     += GC_HAVE_KML
  #INCLUDEPATH += $${KML_INCLUDE}  $${BOOST_INCLUDE}
  #LIBS        += $${KML_LIBS}

  # add kml file i/o
  #SOURCES     += FileIO/KmlRideFile.cpp
  #HEADERS     += FileIO/KmlRideFile.h

endif (USE_KML)

###=================
### OPTIONAL => ICAL
###=================

if (USE_ICAL)

  find_package (LIBICAL)
  add_definitions ( -DGC_HAVE_ICAL )

  set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${LIBICAL_INCLUDE_DIR})
  set (GC_OPT_LIBS ${GC_OPT_LIBS} ${LIBICAL_LIBRARIES})

  # add caldav and diary functions

  set (GC_HEADER_FILES ${GC_HEADER_FILES}
    src/Core/ICalendar.h src/Charts/DiaryWindow.h src/Cloud/CalDAV.h
    )

  set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
    src/Core/ICalendar.cpp src/Charts/DiaryWindow.cpp src/Cloud/CalDAV.cpp
    )

endif (USE_ICAL)

###===================
### OPTIONAL => LIBUSB
###===================

if (USE_LIBUSB)

  find_package (LIBUSB)
  add_definitions ( -DGC_HAVE_LIBUSB )

  set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIR})
  set (GC_OPT_LIBS ${GC_OPT_LIBS} ${LIBUSB_LIBRARIES})

  set (GC_HEADER_FILES ${GC_HEADER_FILES}
    src/Train/LibUsb.h src/Train/EzUsb.h src/Train/Fortius.cpp src/Train/FortiusController.h
    src/Train/Imagic.h src/Train/ImagicController.h
    )

  set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
    src/Train/LibUsb.cpp src/Train/EzUsb.c src/Train/Fortius.cpp src/Train/FortiusController.cpp
    src/Train/Imagic.cpp src/Train/ImagicController.cpp
    )

endif (USE_LIBUSB)

###===================================================
### OPTIONAL => USBXPRESS [Windows only for ANT+ USB1]
###===================================================

if (WIN32)

  # are we supporting USB1 devices on Windows?
  if (USE_USBXPRESS)

    # we will work out the rest if you tell use where it is installed
    #isEmpty(USBXPRESS_INCLUDE) { USBXPRESS_INCLUDE = $${USBXPRESS_INSTALL} }

    # this is windows only !
    #isEmpty(USBXPRESS_LIBS)    { USBXPRESS_LIBS    = $${USBXPRESS_INSTALL}/x86/SiUSBXp.lib }

    #DEFINES     += GC_HAVE_USBXPRESS
    #INCLUDEPATH += $${USBXPRESS_INCLUDE}
    #LIBS        += $${USBXPRESS_LIBS}

    #SOURCES += Train/USBXpress.cpp
    #HEADERS += Train/USBXpress.h

  endif (USE_USBXPRESS)

endif (WIN32)

###=============================================================
### OPTIONAL => VLC [Windows and Unix. OSX uses QuickTime Video]
###=============================================================

# if you want video playback on training mode then
# download and install vlc (videolan) from
# ftp.videolan.org/pub/vlc/1.1.8 or http://download.videolan.org/vlc/ for
# your platform there are sdks for Mac and Windows. On Linux you
# will need to use the latest distro (e.g. Meerkat
# on Ubuntu) to be sure apt-get installs the latest
# builds (we need 1.1.8 or higher).
# Set path to the root of the VLC installation
# If the are installed in /usr/local/vlc then set
# VLC_INSTALL = /usr/local/vlc
# This will automatically set (Windows and Unix):
# VLC_INCLUDE = $${VLC_INSTALL}/include
# For Windows
# VLC_LIBS    = $${VLC_INSTALL}/lib/libvlc.dll.a
# For Unix
# VLC_LIBS    = -lvlc
# You may override the INCLUDE and LIB files if you like.
# You *must* define VLC_INSTALL to use this feature.
#VLC_INSTALL =
#VLC_INCLUDE =
#VLC_LIBS    =

# *** Mac users NOTE ***
# On MAC you don't need libvlc since we use the
# native QTKit (OSX framework) for video playback
# using Quicktime and on 10.6 or higher Quicktime X
#

# What video playback do you want?
#DEFINES += GC_VIDEO_NONE             # dont add any video playback support
#DEFINES += GC_VIDEO_QUICKTIME        # mac only and the default
#DEFINES += GC_VIDEO_QT5              # use QT5 qvideowidget if QT > 5.2.1
#DEFINES += GC_VIDEO_VLC              # use VideoLan library needs VLC_INSTALL defined above
#DEFINES += GC_VIDEO_AV               # use AV Foundation on Mac now QTKit is deprecated

if (USE_VLC)

  # not on a mac as they use quicktime video
  if (NOT APPLE)

    find_package (LIBVLC)
    add_definitions ( -DGC_HAVE_VLC )

    set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${LIBVLC_INCLUDE_DIR})
    set (GC_OPT_LIBS ${GC_OPT_LIBS} ${LIBVLC_LIBRARIES})

  endif (NOT APPLE)

endif (USE_VLC)

###=======================
### OPTIONAL => SAMPLERATE
###=======================

# Mainly tested on Linux, compilation on Windows requires
# some modifications in the build process to build with MSVC2015
# Code is available at: http://www.mega-nerd.com/SRC/

if (USE_SAMPLERATE)

  find_package (LIBSAMPLERATE REQUIRED)
  add_definitions ( -DGC_HAVE_SAMPLERATE )

  set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} ${LIBSAMPLERATE_INCLUDE_DIR})
  set (GC_OPT_LIBS ${GC_OPT_LIBS} ${LIBSAMPLERATE_LIBRARIES})

endif (USE_SAMPLERATE)

###==================================
### OPTIONAL => HTTP API WEB SERVICES
###==================================

if (USE_HTPATH)

  add_definitions ( -DGC_WANT_HTTP )

  set (GC_OPT_INCLUDE_DIRS ${GC_OPT_INCLUDE_DIRS} "httpserver" )

  set (GC_HEADER_FILES ${GC_HEADER_FILES}
    src/Core/APIWebService.h
    httpserver/httpglobal.h
    httpserver/httplistener.h
    httpserver/httpconnectionhandler.h
    httpserver/httpconnectionhandlerpool.h
    httpserver/httprequest.h
    httpserver/httpresponse.h
    httpserver/httpcookie.h
    httpserver/httprequesthandler.h
    httpserver/httpsession.h
    httpserver/httpsessionstore.h
    httpserver/staticfilecontroller.h
    )

  set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
    src/Core/APIWebService.cpp
    httpserver/httpglobal.cpp
    httpserver/httplistener.cpp
    httpserver/httpconnectionhandler.cpp
    httpserver/httpconnectionhandlerpool.cpp
    httpserver/httprequest.cpp
    httpserver/httpresponse.cpp
    httpserver/httpcookie.cpp
    httpserver/httprequesthandler.cpp
    httpserver/httpsession.cpp
    httpserver/httpsessionstore.cpp
    httpserver/staticfilecontroller.cpp
    )

endif (USE_HTPATH)

###=====================================================
### OPTIONAL => CLOUD DB [Google App Engine Integration]
###=====================================================

##----------------------------------------------##
## CloudDB is only supported on QT5.5 or higher ##
##----------------------------------------------##

# Using the GoldenCheetah/CloudDB feature (with a private CloudDB instance)
# requires that you have a running instance of CloudDB on Google App Engine
# (for details on CloudDB please check the documentation here: https://github.com/GoldenCheetah/CloudDB

if (USE_CloudDB)

  if (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.5)

    #DEFINES +=GC_CLOUD_DB_BASIC_AUTH=
    #DEFINES +=GC_CLOUD_DB_APP_NAME=

    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/Cloud/CloudDBChart.h src/Cloud/CloudDBCommon.h
      src/Cloud/CloudDBCurator.h src/Cloud/CloudDBStatus.h
      src/Cloud/CloudDBVersion.h src/Cloud/CloudDBTelemetry.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Cloud/CloudDBChart.cpp src/Cloud/CloudDBCommon.cpp
      src/Cloud/CloudDBCurator.cpp src/Cloud/CloudDBStatus.cpp
      src/Cloud/CloudDBVersion.cpp src/Cloud/CloudDBTelemetry.cpp
      )

    add_definitions ( -DGC_HAS_CLOUD_DB )

  else (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.5)

    # QT5 but not 5.5 or higher
    message(STATUS "INFO: CloudDB requires version QT >= 5.5, no support for ${Qt5Widgets_VERSION}")

  endif (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.5)

endif (USE_CloudDB)

###############################################################################
#                                                                             #
#         ***  SECTION THREE - GOLDENCHEETAH SOURCE FILES  ***                #
#                                                                             #
#                                                                             #
###############################################################################

###===========================================
### FEATURES ENABLED WHEN HAVE QT5 [or higher]
###===========================================

if (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5)

  # Features that only work with QT5 or higher

  set (GC_HEADER_FILES ${GC_HEADER_FILES}
    src/Cloud/Dropbox.h
    src/Cloud/GoogleDrive.h src/Cloud/KentUniversity.h
    src/Cloud/OpenData.h
    )

  set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
    src/Cloud/Dropbox.cpp
    src/Cloud/GoogleDrive.cpp src/Cloud/KentUniversity.cpp
    src/Cloud/OpenData.cpp
    )

  if (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.4)

    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/Cloud/SixCycle.h
      src/Cloud/PolarFlow.h
      src/Cloud/SportTracks.h
      src/Cloud/TodaysPlan.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Cloud/SixCycle.cpp
      src/Cloud/PolarFlow.cpp
      src/Cloud/SportTracks.cpp
      src/Cloud/TodaysPlan.cpp
      )

  endif (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.4)

  set (GC_HEADER_FILES ${GC_HEADER_FILES}
    src/Train/MonarkController.h src/Train/MonarkConnection.h
    src/Train/Kettler.h src/Train/KettlerController.h src/Train/KettlerConnection.h
    src/Train/KettlerRacer.h src/Train/KettlerRacerController.h src/Train/KettlerRacerConnection.h
    src/Train/Ergofit.h src/Train/ErgofitController.h src/Train/ErgofitConnection.h
    src/Train/DaumController.h src/Train/Daum.h
    src/Train/KurtInRide.h src/Train/KurtSmartControl.h
    )

  set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
    src/Train/MonarkController.cpp src/Train/MonarkConnection.cpp
    src/Train/Kettler.cpp src/Train/KettlerController.cpp src/Train/KettlerConnection.cpp
    src/Train/KettlerRacer.cpp src/Train/KettlerRacerController.cpp src/Train/KettlerRacerConnection.cpp
    src/Train/Ergofit.cpp src/Train/ErgofitController.cpp src/Train/ErgofitConnection.cpp
    src/Train/DaumController.cpp src/Train/Daum.cpp
    src/Train/KurtInRide.cpp src/Train/KurtSmartControl.cpp
    )

  # bluetooth in QT5.5 or higher (5.4 was only a tech preview)
  if (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.5)

    #QT += bluetooth
    find_package (Qt5Bluetooth REQUIRED)

    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/Train/BT40Controller.h src/Train/BT40Device.h
      src/Train/VMProConfigurator.h src/Train/VMProWidget.h
      src/Train/Ftms.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Train/BT40Controller.cpp src/Train/BT40Device.cpp
      src/Train/VMProConfigurator.cpp src/Train/VMProWidget.cpp
      src/Train/Ftms.cpp
      )

  endif (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.5)

  # qt charts is officially supported from QT5.8 or higher
  # in 5.7 it is a tech preview and not always available

  if (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.8)

    # Dashboard uses qt charts, so needs at least Qt 5.7

    find_package(Qt5Charts REQUIRED)
    find_package(Qt5OpenGL REQUIRED)

    add_definitions( -DGC_HAVE_OVERVIEW )

    # Dashboard uses qt charts, so needs at least Qt 5.7

    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/Gui/ChartSpace.h src/Charts/OverviewItems.h src/Charts/Overview.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Gui/ChartSpace.cpp src/Charts/OverviewItems.cpp src/Charts/Overview.cpp
      )

    # generic chart

    add_definitions( -DGC_HAVE_GENERIC )

    set (GC_HEADER_FILES ${GC_HEADER_FILES}
      src/Charts/UserChartWindow.h src/Charts/UserChartOverviewItem.h src/Charts/UserChart.h src/Charts/UserChartData.h
      src/Charts/GenericChart.h src/Charts/GenericPlot.h src/Charts/GenericSelectTool.h src/Charts/GenericLegend.h src/Charts/GenericAnnotations.h
      )

    set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
      src/Charts/UserChartWindow.cpp src/Charts/UserChartOverviewItem.cpp src/Charts/UserChart.cpp src/Charts/UserChartData.cpp
      src/Charts/GenericChart.cpp src/Charts/GenericPlot.cpp src/Charts/GenericSelectTool.cpp src/Charts/GenericLegend.cpp src/Charts/GenericAnnotations.cpp
      )

  endif (Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5.8)

endif(Qt5Widgets_VERSION VERSION_GREATER_EQUAL 5)

###=====================
### LEX AND YACC SOURCES
###=====================

# qmake appends _yacc and _lex to genereted files so we need to do that too.
# We also need to add some flags to bison_target to make cmake generate the same output as qmake.

bison_target (DataFilter_y src/Core/DataFilter.y
  ${CMAKE_CURRENT_BINARY_DIR}/DataFilter_yacc.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/DataFilter_yacc.h
  COMPILE_FLAGS "-p DataFilter -b DataFilter -d"
  )
flex_target (DataFilter_l src/Core/DataFilter.l ${CMAKE_CURRENT_BINARY_DIR}/DataFilter_lex.cpp)
add_flex_bison_dependency (DataFilter_l DataFilter_y)
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  ${BISON_DataFilter_y_OUTPUTS}
  ${FLEX_DataFilter_l_OUTPUTS}
  )
set_property(SOURCE DataFilter_yacc.h PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE DataFilter_yacc.cpp PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE DataFilter_lex.cpp PROPERTY SKIP_AUTOGEN ON)

bison_target (JsonRideFile_y src/FileIO/JsonRideFile.y
  ${CMAKE_CURRENT_BINARY_DIR}/JsonRideFile_yacc.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/JsonRideFile_yacc.h
  COMPILE_FLAGS "-p JsonRideFile -b JsonRideFile -d"
  )
flex_target (JsonRideFile_l src/FileIO/JsonRideFile.l ${CMAKE_CURRENT_BINARY_DIR}/JsonRideFile_lex.cpp)
add_flex_bison_dependency (JsonRideFile_l JsonRideFile_y)
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  ${BISON_JsonRideFile_y_OUTPUTS}
  ${FLEX_JsonRideFile_l_OUTPUTS}
  )
set_property(SOURCE JsonRideFile_yacc.h PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE JsonRideFile_yacc.cpp PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE JsonRideFile_lex.cpp PROPERTY SKIP_AUTOGEN ON)

bison_target (RideDB_y src/Core/RideDB.y
  ${CMAKE_CURRENT_BINARY_DIR}/RideDB_yacc.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/RideDB_yacc.h
  COMPILE_FLAGS "-p RideDB -b RideDB -d"
  )
flex_target (RideDB_l src/Core/RideDB.l ${CMAKE_CURRENT_BINARY_DIR}/RideDB_lex.cpp)
add_flex_bison_dependency (RideDB_l RideDB_y)
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  ${BISON_RideDB_y_OUTPUTS}
  ${FLEX_RideDB_l_OUTPUTS}
  )
set_property(SOURCE RideDB_yacc.h PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE RideDB_yacc.cpp PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE RideDB_lex.cpp PROPERTY SKIP_AUTOGEN ON)

###=========================================
### HEADER FILES [scanned by qmake, for moc]
###=========================================

# ANT+
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/ANT/ANTChannel.h src/ANT/ANT.h src/ANT/ANTlocalController.h src/ANT/ANTLogger.h src/ANT/ANTMessage.h src/ANT/ANTMessages.h
  )

# Charts and associated widgets
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Charts/Aerolab.h src/Charts/AerolabWindow.h src/Charts/AllPlot.h src/Charts/AllPlotInterval.h src/Charts/AllPlotSlopeCurve.h
  src/Charts/AllPlotWindow.h src/Charts/BlankState.h src/Charts/ChartBar.h src/Charts/ChartSettings.h
  src/Charts/CpPlotCurve.h src/Charts/CPPlot.h src/Charts/CriticalPowerWindow.h src/Charts/DaysScaleDraw.h src/Charts/ExhaustionDialog.h src/Charts/GcOverlayWidget.h
  src/Charts/GcPane.h src/Charts/GoldenCheetah.h src/Charts/HistogramWindow.h
  src/Charts/HrPwPlot.h src/Charts/HrPwWindow.h src/Charts/IndendPlotMarker.h src/Charts/IntervalSummaryWindow.h src/Charts/LogTimeScaleDraw.h
  src/Charts/LTMCanvasPicker.h src/Charts/LTMChartParser.h src/Charts/LTMOutliers.h src/Charts/LTMPlot.h src/Charts/LTMPopup.h
  src/Charts/LTMSettings.h src/Charts/LTMTool.h src/Charts/LTMTrend2.h src/Charts/LTMTrend.h src/Charts/LTMWindow.h
  src/Charts/MetadataWindow.h src/Charts/MUPlot.h src/Charts/MUPool.h src/Charts/MUWidget.h src/Charts/PfPvPlot.h src/Charts/PfPvWindow.h
  src/Charts/PowerHist.h src/Charts/ReferenceLineDialog.h src/Charts/RideEditor.h src/Charts/RideMapWindow.h
  src/Charts/ScatterPlot.h src/Charts/ScatterWindow.h src/Charts/SmallPlot.h src/Charts/TreeMapPlot.h
  src/Charts/TreeMapWindow.h src/Charts/ZoneScaleDraw.h
  )

# cloud services
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Cloud/CalendarDownload.h src/Cloud/CloudService.h
  src/Cloud/LocalFileStore.h src/Cloud/OAuthDialog.h src/Cloud/TodaysPlanBodyMeasures.h
  src/Cloud/WithingsDownload.h src/Cloud/Strava.h src/Cloud/CyclingAnalytics.h src/Cloud/RideWithGPS.h
  src/Cloud/TrainingsTageBuch.h src/Cloud/Selfloops.h src/Cloud/Velohero.h src/Cloud/SportsPlusHealth.h
  src/Cloud/AddCloudWizard.h src/Cloud/Withings.h src/Cloud/MeasuresDownload.h src/Cloud/Xert.h
  )

# core data
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Core/Athlete.h src/Core/Context.h src/Core/DataFilter.h src/Core/FreeSearch.h src/Core/GcCalendarModel.h src/Core/GcUpgrade.h
  src/Core/IdleTimer.h src/Core/IntervalItem.h src/Core/NamedSearch.h src/Core/RideCache.h src/Core/RideCacheModel.h src/Core/RideDB.h
  src/Core/RideItem.h src/Core/Route.h src/Core/RouteParser.h src/Core/Season.h src/Core/SeasonParser.h src/Core/Secrets.h src/Core/Settings.h
  src/Core/Specification.h src/Core/TimeUtils.h src/Core/Units.h src/Core/UserData.h src/Core/Utils.h
  src/Core/Measures.h src/Core/Quadtree.h
  )

# device and file IO or edit
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Gui/AboutDialog.h src/Gui/AddIntervalDialog.h src/Gui/AnalysisSidebar.h src/Gui/ChooseCyclistDialog.h src/Gui/ColorButton.h
  src/Gui/Colors.h src/Gui/CompareDateRange.h src/Gui/CompareInterval.h src/Gui/ComparePane.h src/Gui/ConfigDialog.h src/Gui/DiarySidebar.h
  src/Gui/DragBar.h src/Gui/EstimateCPDialog.h src/Gui/GcCrashDialog.h src/Gui/GcSideBarItem.h src/Gui/GcToolBar.h src/Gui/GcWindowLayout.h
  src/Gui/GcWindowRegistry.h src/Gui/GenerateHeatMapDialog.h src/Gui/GProgressDialog.h src/Gui/HelpWhatsThis.h src/Gui/HelpWindow.h
  src/Gui/IntervalTreeView.h src/Gui/LTMSidebar.h src/Gui/MainWindow.h src/Gui/NewCyclistDialog.h src/Gui/Pages.h src/Gui/RideNavigator.h src/Gui/RideNavigatorProxy.h
  src/Gui/SaveDialogs.h src/Gui/SearchBox.h src/Gui/SearchFilterBox.h src/Gui/SolveCPDialog.h src/Gui/AthleteTab.h src/Gui/AbstractView.h src/Gui/ToolsRhoEstimator.h
  src/Gui/Views.h src/Gui/BatchExportDialog.h src/Gui/DownloadRideDialog.h src/Gui/ManualRideDialog.h src/Gui/NewMainWindow.h src/Gui/NewSideBar.h
  src/Gui/MergeActivityWizard.h src/Gui/RideImportWizard.h src/Gui/SplitActivityWizard.h src/Gui/SolverDisplay.h src/Gui/MetricSelect.h
  src/Gui/AddChartWizard.h src/Gui/NavigationModel.h src/Gui/AthleteView.h src/Gui/AthleteConfigDialog.h src/Gui/AthletePages.h src/Gui/Perspective.h
  src/Gui/PerspectiveDialog.h
  )

# GUI components
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Gui/AboutDialog.h src/Gui/AddIntervalDialog.h src/Gui/AnalysisSidebar.h src/Gui/ChooseCyclistDialog.h src/Gui/ColorButton.h
  src/Gui/Colors.h src/Gui/CompareDateRange.h src/Gui/CompareInterval.h src/Gui/ComparePane.h src/Gui/ConfigDialog.h src/Gui/DiarySidebar.h
  src/Gui/DragBar.h src/Gui/EstimateCPDialog.h src/Gui/GcCrashDialog.h src/Gui/GcSideBarItem.h src/Gui/GcToolBar.h src/Gui/GcWindowLayout.h
  src/Gui/GcWindowRegistry.h src/Gui/GenerateHeatMapDialog.h src/Gui/GProgressDialog.h src/Gui/HelpWhatsThis.h src/Gui/HelpWindow.h
  src/Gui/IntervalTreeView.h src/Gui/LTMSidebar.h src/Gui/MainWindow.h src/Gui/NewCyclistDialog.h src/Gui/Pages.h src/Gui/RideNavigator.h src/Gui/RideNavigatorProxy.h
  src/Gui/SaveDialogs.h src/Gui/SearchBox.h src/Gui/SearchFilterBox.h src/Gui/SolveCPDialog.h src/Gui/AthleteTab.h src/Gui/AbstractView.h src/Gui/ToolsRhoEstimator.h
  src/Gui/Views.h src/Gui/BatchExportDialog.h src/Gui/DownloadRideDialog.h src/Gui/ManualRideDialog.h src/Gui/NewMainWindow.h src/Gui/NewSideBar.h
  src/Gui/MergeActivityWizard.h src/Gui/RideImportWizard.h src/Gui/SplitActivityWizard.h src/Gui/SolverDisplay.h src/Gui/MetricSelect.h
  src/Gui/AddChartWizard.h src/Gui/NavigationModel.h src/Gui/AthleteView.h src/Gui/AthleteConfigDialog.h src/Gui/AthletePages.h src/Gui/Perspective.h
  src/Gui/PerspectiveDialog.h
  )

# metrics and models
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Metrics/Banister.h src/Metrics/CPSolver.h src/Metrics/Estimator.h src/Metrics/ExtendedCriticalPower.h src/Metrics/HrZones.h src/Metrics/PaceZones.h
  src/Metrics/PDModel.h src/Metrics/PMCData.h src/Metrics/PowerProfile.h src/Metrics/RideMetadata.h src/Metrics/RideMetric.h src/Metrics/SpecialFields.h
  src/Metrics/Statistic.h src/Metrics/UserMetricParser.h src/Metrics/UserMetricSettings.h src/Metrics/VDOTCalculator.h src/Metrics/WPrime.h src/Metrics/Zones.h
  )

## Planning and Compliance
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Planning/PlanningWindow.h
  )

# contrib
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  contrib/qtsolutions/codeeditor/codeeditor.h contrib/qtsolutions/json/mvjson.h
  contrib/qtsolutions/qwtcurve/qwt_plot_gapped_curve.h  contrib/qxt/src/qxtspanslider.h
  contrib/qxt/src/qxtspanslider_p.h contrib/qxt/src/qxtstringspinbox.h contrib/qzip/zipreader.h
  contrib/qzip/zipwriter.h contrib/lmfit/lmcurve.h  contrib/lmfit/lmcurve_tyd.h
  contrib/lmfit/lmmin.h  contrib/lmfit/lmstruct.h
  contrib/levmar/compiler.h  contrib/levmar/levmar.h  contrib/levmar/lm.h  contrib/levmar/misc.h
  contrib/boost/GeometricTools_BSplineCurve.h
  contrib/kmeans/kmeans_dataset.h contrib/kmeans/kmeans_general_functions.h contrib/kmeans/hamerly_kmeans.h
  contrib/kmeans/kmeans.h contrib/kmeans/original_space_kmeans.h contrib/kmeans/triangle_inequality_base_kmeans.h
  contrib/voronoi/Voronoi.h
  )

# Train View
set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Train/AddDeviceWizard.h src/Train/CalibrationData.h src/Train/ComputrainerController.h src/Train/Computrainer.h src/Train/DeviceConfiguration.h
  src/Train/DeviceTypes.h src/Train/DialWindow.h src/Train/ErgDBDownloadDialog.h src/Train/ErgDB.h src/Train/ErgFile.h src/Train/ErgFilePlot.h
  src/Train/Library.h src/Train/LibraryParser.h src/Train/MeterWidget.h src/Train/NullController.h src/Train/RealtimeController.h
  src/Train/RealtimeData.h src/Train/RealtimePlot.h src/Train/RealtimePlotWindow.h src/Train/RemoteControl.h src/Train/SpinScanPlot.h
  src/Train/SpinScanPlotWindow.h src/Train/SpinScanPolarPlot.h src/Train/GarminServiceHelper.h src/Train/PhysicsUtility.h src/Train/BicycleSim.h
  )

set (GC_HEADER_FILES ${GC_HEADER_FILES} # Qt 5
  src/Train/TodaysPlanWorkoutDownload.h
  )

set (GC_HEADER_FILES ${GC_HEADER_FILES}
  src/Train/TrainBottom.h src/Train/TrainDB.h src/Train/TrainSidebar.h
  src/Train/VideoLayoutParser.h src/Train/VideoSyncFile.h src/Train/WorkoutPlotWindow.h src/Train/WebPageWindow.h
  src/Train/WorkoutWidget.h src/Train/WorkoutWidgetItems.h src/Train/WorkoutWindow.h src/Train/WorkoutWizard.h src/Train/ZwoParser.h

  )

###=============
### SOURCE FILES
###=============

## ANT+
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/ANT/ANTChannel.cpp src/ANT/ANT.cpp src/ANT/ANTlocalController.cpp src/ANT/ANTLogger.cpp src/ANT/ANTMessage.cpp
  )

## Charts and related
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Charts/Aerolab.cpp src/Charts/AerolabWindow.cpp src/Charts/AllPlot.cpp src/Charts/AllPlotInterval.cpp src/Charts/AllPlotSlopeCurve.cpp
  src/Charts/AllPlotWindow.cpp src/Charts/BlankState.cpp src/Charts/ChartBar.cpp src/Charts/ChartSettings.cpp
  src/Charts/CPPlot.cpp src/Charts/CpPlotCurve.cpp src/Charts/CriticalPowerWindow.cpp src/Charts/ExhaustionDialog.cpp src/Charts/GcOverlayWidget.cpp src/Charts/GcPane.cpp
  src/Charts/GoldenCheetah.cpp src/Charts/HistogramWindow.cpp src/Charts/HrPwPlot.cpp
  src/Charts/HrPwWindow.cpp src/Charts/IndendPlotMarker.cpp src/Charts/IntervalSummaryWindow.cpp src/Charts/LogTimeScaleDraw.cpp
  src/Charts/LTMCanvasPicker.cpp src/Charts/LTMChartParser.cpp src/Charts/LTMOutliers.cpp src/Charts/LTMPlot.cpp src/Charts/LTMPopup.cpp
  src/Charts/LTMSettings.cpp src/Charts/LTMTool.cpp src/Charts/LTMTrend.cpp src/Charts/LTMWindow.cpp
  src/Charts/MetadataWindow.cpp src/Charts/MUPlot.cpp src/Charts/MUWidget.cpp src/Charts/PfPvPlot.cpp src/Charts/PfPvWindow.cpp
  src/Charts/PowerHist.cpp src/Charts/ReferenceLineDialog.cpp src/Charts/RideEditor.cpp src/Charts/RideMapWindow.cpp
  src/Charts/ScatterPlot.cpp src/Charts/ScatterWindow.cpp src/Charts/SmallPlot.cpp src/Charts/TreeMapPlot.cpp
  src/Charts/TreeMapWindow.cpp
  )

## Cloud Services / Web resources
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Cloud/CalendarDownload.cpp src/Cloud/CloudService.cpp
  src/Cloud/LocalFileStore.cpp src/Cloud/OAuthDialog.cpp src/Cloud/TodaysPlanBodyMeasures.cpp
  src/Cloud/WithingsDownload.cpp src/Cloud/Strava.cpp src/Cloud/CyclingAnalytics.cpp src/Cloud/RideWithGPS.cpp
  src/Cloud/TrainingsTageBuch.cpp src/Cloud/Selfloops.cpp src/Cloud/Velohero.cpp src/Cloud/SportsPlusHealth.cpp
  src/Cloud/AddCloudWizard.cpp src/Cloud/Withings.cpp src/Cloud/MeasuresDownload.cpp src/Cloud/Xert.cpp
  )

## Core Data Structures
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Core/Athlete.cpp src/Core/Context.cpp src/Core/DataFilter.cpp src/Core/FreeSearch.cpp src/Core/GcUpgrade.cpp src/Core/IdleTimer.cpp
  src/Core/IntervalItem.cpp src/Core/main.cpp src/Core/NamedSearch.cpp src/Core/RideCache.cpp src/Core/RideCacheModel.cpp src/Core/RideItem.cpp
  src/Core/Route.cpp src/Core/RouteParser.cpp src/Core/Season.cpp src/Core/SeasonParser.cpp src/Core/Settings.cpp src/Core/Specification.cpp
  src/Core/TimeUtils.cpp src/Core/Units.cpp src/Core/UserData.cpp src/Core/Utils.cpp
  src/Core/Measures.cpp src/Core/Quadtree.cpp
  )

## File and Device IO and Editing
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/FileIO/ArchiveFile.cpp src/FileIO/AthleteBackup.cpp src/FileIO/Bin2RideFile.cpp src/FileIO/BinRideFile.cpp
  src/FileIO/CommPort.cpp
  src/FileIO/Computrainer3dpFile.cpp src/FileIO/CsvRideFile.cpp src/FileIO/DataProcessor.cpp src/FileIO/Device.cpp
  src/FileIO/FitlogParser.cpp src/FileIO/FitlogRideFile.cpp src/FileIO/FitRideFile.cpp src/FileIO/FixAeroPod.cpp src/FileIO/FixDeriveDistance.cpp
  src/FileIO/FixDeriveHeadwind.cpp src/FileIO/FixDerivePower.cpp src/FileIO/FixDeriveTorque.cpp src/FileIO/FixElevation.cpp src/FileIO/FixLapSwim.cpp
  src/FileIO/FixFreewheeling.cpp src/FileIO/FixGaps.cpp src/FileIO/FixGPS.cpp src/FileIO/FixRunningCadence.cpp src/FileIO/FixRunningPower.cpp
  src/FileIO/FixHRSpikes.cpp src/FileIO/FixMoxy.cpp src/FileIO/FixPower.cpp src/FileIO/FixSmO2.cpp src/FileIO/FixSpeed.cpp src/FileIO/FixSpikes.cpp
  src/FileIO/FixTorque.cpp src/FileIO/GcRideFile.cpp src/FileIO/GpxParser.cpp src/FileIO/GpxRideFile.cpp src/FileIO/JouleDevice.cpp src/FileIO/LapsEditor.cpp
  src/FileIO/MacroDevice.cpp src/FileIO/ManualRideFile.cpp src/FileIO/MoxyDevice.cpp
  src/FileIO/PolarRideFile.cpp src/FileIO/PowerTapDevice.cpp src/FileIO/PowerTapUtil.cpp src/FileIO/PwxRideFile.cpp src/FileIO/QuarqParser.cpp
  src/FileIO/QuarqRideFile.cpp src/FileIO/RawRideFile.cpp src/FileIO/RideAutoImportConfig.cpp
  src/FileIO/RideFileCache.cpp src/FileIO/RideFileCommand.cpp src/FileIO/RideFile.cpp src/FileIO/RideFileTableModel.cpp
  src/FileIO/Serial.cpp src/FileIO/SlfParser.cpp src/FileIO/SlfRideFile.cpp src/FileIO/SmfParser.cpp src/FileIO/SmfRideFile.cpp src/FileIO/SmlParser.cpp
  src/FileIO/SmlRideFile.cpp src/FileIO/Snippets.cpp src/FileIO/SrdRideFile.cpp src/FileIO/SrmRideFile.cpp src/FileIO/SyncRideFile.cpp
  src/FileIO/TacxCafRideFile.cpp src/FileIO/TcxParser.cpp src/FileIO/TcxRideFile.cpp src/FileIO/TxtRideFile.cpp src/FileIO/WkoRideFile.cpp
  src/FileIO/XDataDialog.cpp src/FileIO/XDataTableModel.cpp src/FileIO/FilterHRV.cpp src/FileIO/MeasuresCsvImport.cpp
  src/FileIO/LocationInterpolation.cpp src/FileIO/TTSReader.cpp src/FileIO/EpmRideFile.cpp src/FileIO/EpmParser.cpp
  )

## GUI Elements and Dialogs
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Gui/AboutDialog.cpp src/Gui/AddIntervalDialog.cpp src/Gui/AnalysisSidebar.cpp src/Gui/ChooseCyclistDialog.cpp src/Gui/ColorButton.cpp
  src/Gui/Colors.cpp src/Gui/CompareDateRange.cpp src/Gui/CompareInterval.cpp src/Gui/ComparePane.cpp src/Gui/ConfigDialog.cpp src/Gui/DiarySidebar.cpp
  src/Gui/DragBar.cpp src/Gui/EstimateCPDialog.cpp src/Gui/GcCrashDialog.cpp src/Gui/GcSideBarItem.cpp src/Gui/GcToolBar.cpp src/Gui/GcWindowLayout.cpp
  src/Gui/GcWindowRegistry.cpp src/Gui/GenerateHeatMapDialog.cpp src/Gui/GProgressDialog.cpp src/Gui/HelpWhatsThis.cpp src/Gui/HelpWindow.cpp
  src/Gui/IntervalTreeView.cpp src/Gui/LTMSidebar.cpp src/Gui/MainWindow.cpp src/Gui/NewCyclistDialog.cpp src/Gui/Pages.cpp
  src/Gui/RideNavigator.cpp src/Gui/SaveDialogs.cpp
  src/Gui/SearchBox.cpp src/Gui/SearchFilterBox.cpp src/Gui/SolveCPDialog.cpp src/Gui/AthleteTab.cpp src/Gui/AbstractView.cpp
  src/Gui/ToolsRhoEstimator.cpp src/Gui/Views.cpp
  src/Gui/BatchExportDialog.cpp src/Gui/DownloadRideDialog.cpp src/Gui/ManualRideDialog.cpp src/Gui/EditUserMetricDialog.cpp
  src/Gui/NewMainWindow.cpp src/Gui/NewSideBar.cpp
  src/Gui/MergeActivityWizard.cpp src/Gui/RideImportWizard.cpp src/Gui/SplitActivityWizard.cpp src/Gui/SolverDisplay.cpp src/Gui/MetricSelect.cpp
  src/Gui/AddChartWizard.cpp src/Gui/NavigationModel.cpp src/Gui/AthleteView.cpp src/Gui/AthleteConfigDialog.cpp src/Gui/AthletePages.cpp src/Gui/Perspective.cpp
  src/Gui/PerspectiveDialog.cpp
  )

## Models and Metrics
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Metrics/aBikeScore.cpp src/Metrics/aCoggan.cpp src/Metrics/AerobicDecoupling.cpp src/Metrics/Banister.cpp src/Metrics/BasicRideMetrics.cpp
  src/Metrics/BikeScore.cpp src/Metrics/Coggan.cpp src/Metrics/CPSolver.cpp src/Metrics/DanielsPoints.cpp src/Metrics/Estimator.cpp
  src/Metrics/ExtendedCriticalPower.cpp src/Metrics/GOVSS.cpp src/Metrics/HrTimeInZone.cpp src/Metrics/HrZones.cpp src/Metrics/LeftRightBalance.cpp
  src/Metrics/PaceTimeInZone.cpp src/Metrics/PaceZones.cpp src/Metrics/PDModel.cpp src/Metrics/PeakPace.cpp src/Metrics/PeakPower.cpp src/Metrics/PeakHr.cpp
  src/Metrics/PMCData.cpp src/Metrics/PowerProfile.cpp src/Metrics/RideMetadata.cpp src/Metrics/RideMetric.cpp src/Metrics/RunMetrics.cpp
  src/Metrics/SwimMetrics.cpp src/Metrics/SpecialFields.cpp src/Metrics/Statistic.cpp src/Metrics/SustainMetric.cpp src/Metrics/SwimScore.cpp
  src/Metrics/TimeInZone.cpp src/Metrics/TRIMPPoints.cpp src/Metrics/UserMetric.cpp src/Metrics/UserMetricParser.cpp src/Metrics/VDOTCalculator.cpp
  src/Metrics/VDOT.cpp src/Metrics/WattsPerKilogram.cpp src/Metrics/WPrime.cpp src/Metrics/Zones.cpp src/Metrics/HrvMetrics.cpp src/Metrics/BlinnSolver.cpp
  src/Metrics/RowMetrics.cpp src/Metrics/FastKmeans.cpp
  )

## Planning and Compliance
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Planning/PlanningWindow.cpp
  )

## Contributed solutions
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  contrib/qtsolutions/codeeditor/codeeditor.cpp contrib/qtsolutions/json/mvjson.cpp
  contrib/qtsolutions/qwtcurve/qwt_plot_gapped_curve.cpp
  contrib/qxt/src/qxtspanslider.cpp contrib/qxt/src/qxtstringspinbox.cpp contrib/qzip/zip.cpp
  contrib/lmfit/lmcurve.c contrib/lmfit/lmmin.c
  #contrib/levmar/Axb.c contrib/levmar/lm_core.c contrib/levmar/lmbc_core.c
  #contrib/levmar/lmblec_core.c contrib/levmar/lmbleic_core.c contrib/levmar/lmlec.c contrib/levmar/misc.c
  #contrib/levmar/Axb_core.c contrib/levmar/lm.c contrib/levmar/lmbc.c contrib/levmar/lmblec.c contrib/levmar/lmbleic.c
  #contrib/levmar/lmlec_core.c contrib/levmar/misc_core.c
  contrib/kmeans/kmeans_dataset.cpp contrib/kmeans/kmeans_general_functions.cpp contrib/kmeans/hamerly_kmeans.cpp
  contrib/kmeans/kmeans.cpp contrib/kmeans/original_space_kmeans.cpp contrib/kmeans/triangle_inequality_base_kmeans.cpp
  contrib/voronoi/Voronoi.cpp

  #levmar/lm_core.c levmar/lmbc_core.c #
  #levmar/lmblec_core.c levmar/lmbleic_core.c
  #levmar/Axb_core.c
  #levmar/lmlec_core.c levmar/misc_core.c
  )
# NB. The _core,c files are not used in levmar's CMakeLists.txt file.

## Train View Components
set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Train/AddDeviceWizard.cpp src/Train/CalibrationData.cpp src/Train/ComputrainerController.cpp src/Train/Computrainer.cpp src/Train/DeviceConfiguration.cpp
  src/Train/DeviceTypes.cpp src/Train/DialWindow.cpp src/Train/ErgDB.cpp src/Train/ErgDBDownloadDialog.cpp src/Train/ErgFile.cpp src/Train/ErgFilePlot.cpp
  src/Train/Library.cpp src/Train/LibraryParser.cpp src/Train/MeterWidget.cpp src/Train/NullController.cpp src/Train/RealtimeController.cpp
  src/Train/RealtimeData.cpp src/Train/RealtimePlot.cpp src/Train/RealtimePlotWindow.cpp src/Train/RemoteControl.cpp src/Train/SpinScanPlot.cpp
  src/Train/SpinScanPlotWindow.cpp src/Train/SpinScanPolarPlot.cpp src/Train/GarminServiceHelper.cpp src/Train/PhysicsUtility.cpp src/Train/BicycleSim.cpp
  src/Train/PolynomialRegression.cpp
  )

set (GC_SOURCE_FILES ${GC_SOURCE_FILES} # Qt 5
  src/Train/TodaysPlanWorkoutDownload.cpp
  )

set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
  src/Train/TrainBottom.cpp src/Train/TrainDB.cpp src/Train/TrainSidebar.cpp
  src/Train/VideoLayoutParser.cpp src/Train/VideoSyncFile.cpp src/Train/WorkoutPlotWindow.cpp src/Train/WebPageWindow.cpp
  src/Train/WorkoutWidget.cpp src/Train/WorkoutWidgetItems.cpp src/Train/WorkoutWindow.cpp src/Train/WorkoutWizard.cpp src/Train/ZwoParser.cpp
  src/Train/LiveMapWebPageWindow.cpp
  )

## Crash Handling
if (MSVC)
  set (GC_SOURCE_FILES ${GC_SOURCE_FILES}
    src/Core/WindowsCrashHandler.cpp
    )
endif (MSVC)

#qt5_wrap_ui (GC_FORMS_HEADER_FILES ${GC_FORMS})
#qt5_wrap_cpp (GC_HEADER_FILES_MOC ${GC_HEADER_FILES})
#set (GC_FORMS_HEADER_FILES ${GC_FORMS}) # We use AUTOMUIC ON
set (GC_HEADER_FILES_MOC ${GC_HEADER_FILES}) # We use AUTOMOC ON
set (GC_RCC_FILES ${GC_RESOURCES})	       # We use AUTORCC ON
set (QWT_HEADER_FILES_MOC ${QWT_HEADER_FILES}) # We use AUTOMOC ON
#set (QXT_HEADER_FILES_MOC ${QXT_HEADER_FILES}) # We use AUTOMOC ON

include_directories (
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    #${Qt5LinguistTools_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5Sql_INCLUDE_DIRS}
    ${Qt5SerialPort_INCLUDE_DIRS}
    if (NOT NOWEBKIT)
      ${Qt5WebKitWidgets_INCLUDE_DIRS} # TODO make optional.
    else (NOT NOWEBKIT)
      ${Qt5WebEngine_INCLUDE_DIRS}
      ${Qt5WebEngineCore_INCLUDE_DIRS}
      ${Qt5WebEngineWidgets_INCLUDE_DIRS}
      ${Qt5WebChannel_INCLUDE_DIRS}
      ${Qt5Positioning_INCLUDE_DIRS}
    endif (NOT NOWEBKIT)
    ${Qt5Bluetooth_INCLUDE_DIRS}     # TODO make optional.
    ${Qt5Charts_INCLUDE_DIRS}	     # TODO make optional.
    ${Qt5OpenGL_INCLUDE_DIRS}	     # TODO make optional.
    ${Qt5PrintSupport_INCLUDE_DIRS}
  endif (USE_QT5)

  ${GC_INCLUDE_DIRS}
  ${QWT_INCLUDE_DIR}
  ${QXT_INCLUDE_DIR}
  ${GC_OPT_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR} # May be needed to find generated (ui_*.h) files (see https://anteru.net/2009/09/07/582/).
  )

set (GC_QT_LIBS "Qt5::Widgets")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Core")
#set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::LinguistTools")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Xml")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Sql")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Network")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Svg")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Concurrent")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::SerialPort")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Multimedia")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::MultimediaWidgets")
#if (NOT NOWEBKIT)
#  set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::WebKitWidgets") # TODO make optional.
#else (NOT NOWEBKIT)
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::WebEngine")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::WebEngineCore")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::WebEngineWidgets")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::WebChannel")
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Positioning")
#endif (NOT NOWEBKIT)
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Bluetooth") # TODO make optional.
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::Charts") # TODO make optional.
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::OpenGL") # TODO make optional.
set (GC_QT_LIBS ${GC_QT_LIBS} "Qt5::PrintSupport")

add_executable(GoldenCheetah
  ${GC_HEADER_FILES_MOC}
  ${GC_RCC_FILES}
  ${GC_SOURCE_FILES}
  ${QWT_HEADER_FILES_MOC}
  ${QWT_SOURCE_FILES}
  ${QXT_HEADER_FILES_MOC}
  #${QXT_SOURCE_FILES}
  )

# Deps.
target_link_libraries (GoldenCheetah
  ${GC_QT_LIBS}
  ${ZLIB_LIBRARIES}		# TODO add this only when needed.
  ${GSL_LIBRARIES}
  ${GC_OPT_LIBS}
  #${QWT_LIBRARIES}
  )

#qt5_use_modules(GoldenCheetah Core SerialPort)

# Install of the app.
install (TARGETS GoldenCheetah
  RUNTIME DESTINATION bin
  )

###====================
### MISCELLANEOUS FILES
###====================

#OTHER_FILES +=   Resources/python/library.py Python/SIP/goldencheetah.sip
