###############################################################################
#                                                                             #
#              ***  SECTION ONE - CORE CONFIGURATION  ***                     #
#                                                                             #
#                                                                             #
###############################################################################

!versionAtLeast(QT_VERSION, 6.5.3):error("Use at least Qt version 6.5.3")

###==========================
### IMPORT USER CONFIGURATION
###==========================

# You must configure your settings by copying from gcconfig.pri.in.
# The file contains instructions on settings to make
include(gcconfig.pri)

# You can also define your own local source to add to build
HEADERS += $${LOCALHEADERS}
SOURCES += $${LOCALSOURCES}

###=====================
### GOLDENCHEETAH TARGET
###=====================

DEPENDPATH += .
TEMPLATE = app
TARGET = GoldenCheetah

!isEmpty(APP_NAME) { TARGET = $${APP_NAME} }
CONFIG(debug, debug|release) { QMAKE_CXXFLAGS += -DGC_DEBUG }


###========================================================================
### QT6.5.3 officially supported which mandates c++17 support in toolchain
###========================================================================

# always
QT += xml sql network svg  widgets concurrent serialport multimedia multimediawidgets \
      webenginecore webenginewidgets webchannel positioning webenginequick core5compat
CONFIG += c++20

###=======================================================================
### Directory Structure - Split into subdirs to be more manageable
###=======================================================================
INCLUDEPATH += ./ANT ./Train ./FileIO ./Cloud ./Charts ./Metrics ./Gui ./Core ./Planning
QMAKE_CFLAGS_ISYSTEM =


###=======================================================================
### DISTRIBUTED SOURCE [Snaffled in sources to avoid further dependencies]
###=======================================================================

# qwt, qxt, libz, json, lmfit and qwtcurve
INCLUDEPATH +=  ../qwt/src \
                ../contrib/qxt/src \
                ../contrib/qtsolutions/json \
                ../contrib/qtsolutions/qwtcurve \
                ../contrib/qtsolutions/flowlayout \
                ../contrib/lmfit \
                ../contrib/boost \
                ../contrib/kmeans \
                ../contrib/voronoi

DEFINES += QXT_STATIC

# to make sure we are toolchain neutral we NEVER refer to a lib
# via file extensions .lib or .a in src.pro unless the section is
# platform specific. Instead we use directives -Ldir and -llib
win32 {
    #QWT is configured to build 2 libs (release/debug) on win32 (see qwtbuild.pri)
    CONFIG(release, debug|release){
    LIBS += -L$${PWD}/../qwt/lib -lqwt
    }
    CONFIG(debug, debug|release) {
    LIBS += -L$${PWD}/../qwt/lib -lqwtd
    }

} else {
    #QWT is configured to build 1 lib for all other OS (see qwtbuild.pri)
    LIBS += -L$${PWD}/../qwt/lib -lqwt
}

# compress and math libs must be defined in gcconfig.pri
# if they're not part of the QT include
INCLUDEPATH += $${LIBZ_INCLUDE}
LIBS += $${LIBZ_LIBS}

# GNU Scientific Library
INCLUDEPATH += $${GSL_INCLUDES}
LIBS += $${GSL_LIBS}

###===============================
### PLATFORM SPECIFIC DEPENDENCIES
###===============================

# Microsoft Visual Studion toolchain dependencies
win32-msvc* {
    # we need windows kit 8.2 or higher with MSVC, offer default location
    isEmpty(WINKIT_INSTALL) WINKIT_INSTALL= "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x64"
    LIBS += -L$${WINKIT_INSTALL} -lGdi32 -lUser32
    CONFIG += force_debug_info
} else {
    # gnu toolchain wants math libs
    LIBS += -lm

    unix:!macx {
        # Linux gcc 5 grumbles about unused static globals and leads
        # to a gazillion warnings that are harmless so lets remove them
        QMAKE_CXXFLAGS += -Wno-unused-variable

        # Linux Flex compiler grumbles about unsigned comparisons
        QMAKE_CXXFLAGS += -Wno-sign-compare
    }
}

# windows icon and use QT zlib, not sure why different but keep for now
win32 {

    RC_FILE = Resources/win32/windowsico.rc
    INCLUDEPATH += Resources/win32 $${QT_INSTALL_PREFIX}/src/3rdparty/zlib
    LIBS += -lws2_32

} else {

    RC_FILE = Resources/images/gc.icns
}

macx {

    # we have our own plist
    QMAKE_INFO_PLIST = ./Resources/mac/Info.plist.app

    # on mac we use native buttons and video, but have native fullscreen support
    LIBS    += -lobjc -framework IOKit -framework AppKit

    # GC_VIDEO_QT6 will enable Qt6 video support,
    # GC_VIDEO_VLC will enable VLC video support,
    # otherwise we have a blank videowindow, it will do nothing
    HEADERS += Train/VideoWindow.h
    SOURCES += Train/VideoWindow.cpp
} else {
    # not on mac we need our own full screen support and segment control button
    HEADERS += Gui/QTFullScreen.h
    SOURCES += Gui/QTFullScreen.cpp

    HEADERS += Train/VideoWindow.h
    SOURCES += Train/VideoWindow.cpp
}



# X11
if (defined(GC_WANT_X11)) {
    LIBS += -lX11
}

###=================
### LANGUAGE SUPPORT
###=================

TRANSLATIONS = Resources/translations/gc_fr.ts \
               Resources/translations/gc_ja.ts \
               Resources/translations/gc_it.ts \
               Resources/translations/gc_pt-br.ts \
               Resources/translations/gc_de.ts \
               Resources/translations/gc_cs.ts \
               Resources/translations/gc_es.ts \
               Resources/translations/gc_pt.ts \
               Resources/translations/gc_ru.ts \
               Resources/translations/gc_zh-cn.ts \
               Resources/translations/gc_zh-tw.ts \
               Resources/translations/gc_nl.ts \
               Resources/translations/gc_sv.ts

# need lrelease to generate qm files
isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

# how to run lrelease
isEmpty(TS_DIR):TS_DIR = $${PWD}/Resources/translations
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$TS_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm $$TS_DIR/${QMAKE_FILE_BASE}.qm
TSQM.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += TSQM

###==========
### RESOURCES
###==========

RESOURCES = $${PWD}/Resources/application.qrc



###############################################################################
#                                                                             #
#         ***  SECTION TWO - OPTIONAL LIBRARIES AND FEATURES  ***             #
#                                                                             #
#                                                                             #
###############################################################################


###=========================
### OPTIONAL => Embed Python
###=========================

contains(DEFINES, "GC_WANT_PYTHON") {
    message("Enabling Python support")
    INCLUDEPATH += $$replace(PYTHONINCLUDES, ^-I, )
    LIBS += $${PYTHONLIBS}

    # add Python subdirectory to include path
    INCLUDEPATH += ./Python
    DEFINES += SIP_STATIC_MODULE

    ## Python integration & SIP files
    HEADERS += $$files(Python/SIP/sip*.h) Python/SIP/Bindings.h
    SOURCES += $$files(Python/SIP/sip*.c)
    SOURCES += $$files(Python/SIP/sip*.cpp) Python/SIP/Bindings.cpp

    ## Python Embedding & Charts
    HEADERS += Python/PythonEmbed.h Python/PythonSyntax.h Charts/PythonChart.h
    SOURCES += Python/PythonEmbed.cpp Python/PythonSyntax.cpp Charts/PythonChart.cpp

    ## Python data processors
    HEADERS += FileIO/FixPyScriptsDialog.h FileIO/FixPySettings.h FileIO/FixPyRunner.h \
                FileIO/FixPyScript.h FileIO/FixPyDataProcessor.h
    SOURCES += FileIO/FixPyScriptsDialog.cpp FileIO/FixPySettings.cpp FileIO/FixPyRunner.cpp \
                FileIO/FixPyDataProcessor.cpp
}

###====================
### OPTIONAL => Embed R
###====================

contains(DEFINES, "GC_WANT_R") {

    # Only supports Linux and OSX until RInside and Rcpp support MSVC
    # This is not likely to be very soon, they are heavily dependant on GCC
    # see: http://dirk.eddelbuettel.com/blog/2011/03/25/#rinside_and_qt
    isEmpty(R_HOME){ R_HOME = $$system(R RHOME) }

    # add R subdirectory to include path
    INCLUDEPATH += ./R

    ## include headers and libraries for R
    win32  { QMAKE_CXXFLAGS += -I$$R_HOME/include
             DEFINES += Win32 }
    else   { QMAKE_CXXFLAGS += $$system($$R_HOME/bin/R CMD config --cppflags) }

    ## R has lots of compatibility headers for S and legacy R code we don't want
    DEFINES += STRICT_R_HEADERS

    ## R integration
    HEADERS += R/REmbed.h R/RTool.h R/RGraphicsDevice.h R/RSyntax.h R/RLibrary.h
    SOURCES += R/REmbed.cpp R/RTool.cpp R/RGraphicsDevice.cpp R/RSyntax.cpp R/RLibrary.cpp

    ## R based charts
    HEADERS += Charts/RChart.h Charts/RCanvas.h
    SOURCES += Charts/RChart.cpp Charts/RCanvas.cpp

    ## For hardware accelerated scene rendering
    QT += opengl
}

###=======================================================
### OPTIONAL => D2XX FOR FTDI DRIVERS ON WINDOWS PLATFORMS
###=======================================================

!isEmpty(D2XX_INCLUDE) {

    DEFINES     += GC_HAVE_D2XX
    INCLUDEPATH += $${D2XX_INCLUDE}

    !isEmpty(D2XX_LIBS) { LIBS += $${D2XX_LIBS} }
    unix                { LIBS += -ldl }

    HEADERS     += FileIO/D2XX.h
    SOURCES     += FileIO/D2XX.cpp
}


###==================
### OPTIONAL => SRMIO
###==================

!isEmpty(SRMIO_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(SRMIO_INCLUDE) { SRMIO_INCLUDE = $${SRMIO_INSTALL}/include }
    isEmpty(SRMIO_LIBS)    { SRMIO_LIBS    = -L$${SRMIO_INSTALL}/lib -lsrmio }

    DEFINES     += GC_HAVE_SRMIO
    INCLUDEPATH += $${SRMIO_INCLUDE}
    LIBS        += $${SRMIO_LIBS}

    # add support for srm downloads
    HEADERS     += FileIO/SrmDevice.h
    SOURCES     += FileIO/SrmDevice.cpp
}


###=================
### OPTIONAL => ICAL
###=================

!isEmpty(ICAL_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(ICAL_INCLUDE) { ICAL_INCLUDE = $${ICAL_INSTALL}/include }
    isEmpty(ICAL_LIBS)    { ICAL_LIBS    = -L$${ICAL_INSTALL}/lib -lical }

    DEFINES     += GC_HAVE_ICAL
    INCLUDEPATH += $${ICAL_INCLUDE}
    LIBS        += $${ICAL_LIBS}

    # add caldav and calendar functions
    HEADERS     += Core/ICalendar.h Cloud/CalDAV.h Cloud/CalDAVCloud.h
    SOURCES     += Core/ICalendar.cpp Cloud/CalDAV.cpp Cloud/CalDAVCloud.cpp
}


###===================
### OPTIONAL => LIBUSB
###===================

!isEmpty(LIBUSB_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(LIBUSB_INCLUDE) { LIBUSB_INCLUDE = $${LIBUSB_INSTALL}/include }
    isEmpty(LIBUSB_LIBS)    {
        # needs fixing for msvc toolchain
        _LIBUSB_GCC_LIBNAME = usb
        isEqual(LIBUSB_USE_V_1, true) _LIBUSB_GCC_LIBNAME = usb-1.0

        unix  { LIBUSB_LIBS = -L$${LIBUSB_INSTALL}/lib -l$${_LIBUSB_GCC_LIBNAME} }
        win32 { LIBUSB_LIBS = -L$${LIBUSB_INSTALL}/lib/gcc -l$${_LIBUSB_GCC_LIBNAME} }
    }

    DEFINES     += GC_HAVE_LIBUSB
    INCLUDEPATH += $${LIBUSB_INCLUDE}
    LIBS        += $${LIBUSB_LIBS}

    # lots of dependents
    SOURCES     += Train/LibUsb.cpp Train/Fortius.cpp Train/FortiusController.cpp \
                   Train/Imagic.cpp Train/ImagicController.cpp
    HEADERS     += Train/LibUsb.h Train/Fortius.cpp Train/FortiusController.h \
                   Train/Imagic.h Train/ImagicController.h

    !isEqual(LIBUSB_USE_V_1, true) {
        SOURCES += Train/EzUsb.c
        HEADERS += Train/EzUsb.h
    }

    isEqual(LIBUSB_USE_V_1, true) {
        DEFINES += LIBUSB_V_1
        SOURCES += Train/EzUsb-1.0.c
        HEADERS += Train/EzUsb-1.0.h
    }
}


###===================================================
### OPTIONAL => USBXPRESS [Windows only for ANT+ USB1]
###===================================================

# are we supporting USB1 devices on Windows?
!isEmpty(USBXPRESS_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(USBXPRESS_INCLUDE) { USBXPRESS_INCLUDE = $${USBXPRESS_INSTALL} }

    # this is windows only !
    isEmpty(USBXPRESS_LIBS)    { USBXPRESS_LIBS    = $${USBXPRESS_INSTALL}/x86/SiUSBXp.lib }

    DEFINES     += GC_HAVE_USBXPRESS
    INCLUDEPATH += $${USBXPRESS_INCLUDE}
    LIBS        += $${USBXPRESS_LIBS}

    SOURCES += Train/USBXpress.cpp
    HEADERS += Train/USBXpress.h
}


###=============================================================
### OPTIONAL => VLC [Windows, Unix and OSX]
###=============================================================

!isEmpty(VLC_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(VLC_INCLUDE) { VLC_INCLUDE = $${VLC_INSTALL}/include }
    isEmpty(VLC_LIBS)    { VLC_LIBS    = -L$${VLC_INSTALL}/lib -lvlc }

    DEFINES     += GC_HAVE_VLC
    INCLUDEPATH += $${VLC_INCLUDE}
    LIBS        += $${VLC_LIBS}
}


###=======================
### OPTIONAL => SAMPLERATE
###=======================

!isEmpty(SAMPLERATE_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(SAMPLERATE_INCLUDE) { SAMPLERATE_INCLUDE = $${SAMPLERATE_INSTALL}/include }
    isEmpty(SAMPLERATE_LIBS)    { SAMPLERATE_LIBS    = -L$${SAMPLERATE_INSTALL}/lib -lsamplerate }

    DEFINES     += GC_HAVE_SAMPLERATE
    INCLUDEPATH += $${SAMPLERATE_INCLUDE}
    LIBS        += $${SAMPLERATE_LIBS}
}


###==================================
### HTTP API WEB SERVICES
###==================================

HTPATH = ../contrib/httpserver
INCLUDEPATH += $$HTPATH
DEPENDPATH += $$HTPATH

DEFINES += GC_WANT_HTTP

HEADERS +=  Core/APIWebService.h
SOURCES +=  Core/APIWebService.cpp

HEADERS +=  $$HTPATH/httpglobal.h \
            $$HTPATH/httplistener.h \
            $$HTPATH/httpconnectionhandler.h \
            $$HTPATH/httpconnectionhandlerpool.h \
            $$HTPATH/httprequest.h \
            $$HTPATH/httpresponse.h \
            $$HTPATH/httpcookie.h \
            $$HTPATH/httprequesthandler.h \
            $$HTPATH/staticfilecontroller.h
SOURCES +=  $$HTPATH/httpglobal.cpp \
            $$HTPATH/httplistener.cpp \
            $$HTPATH/httpconnectionhandler.cpp \
            $$HTPATH/httpconnectionhandlerpool.cpp \
            $$HTPATH/httprequest.cpp \
            $$HTPATH/httpresponse.cpp \
            $$HTPATH/httpcookie.cpp \
            $$HTPATH/httprequesthandler.cpp \
            $$HTPATH/staticfilecontroller.cpp


###=====================================================
### OPTIONAL => CLOUD DB [Google App Engine Integration]
###=====================================================

##------------------------------------------------##
## CloudDB is only supported on QT6.5.3 or higher ##
##------------------------------------------------##

notsupported = "INFO: CloudDB requires version QT >= 6.5.3, no support for"
notsupported += $${QT_VERSION}

equals(CloudDB, active) {

            HEADERS += Cloud/CloudDBChart.h Cloud/CloudDBCommon.h \
                       Cloud/CloudDBCurator.h Cloud/CloudDBStatus.h \
                       Cloud/CloudDBVersion.h Cloud/CloudDBTelemetry.h \
                       Cloud/CloudDBUserMetric.h
            SOURCES += Cloud/CloudDBChart.cpp Cloud/CloudDBCommon.cpp \
                       Cloud/CloudDBCurator.cpp Cloud/CloudDBStatus.cpp \
                       Cloud/CloudDBVersion.cpp Cloud/CloudDBTelemetry.cpp \
                       Cloud/CloudDBUserMetric.cpp

            DEFINES += GC_HAS_CLOUD_DB
}



###############################################################################
#                                                                             #
#         ***  SECTION THREE - GOLDENCHEETAH SOURCE FILES  ***                #
#                                                                             #
#                                                                             #
###############################################################################

SOURCES += Cloud/Dropbox.cpp
HEADERS += Cloud/Dropbox.h
SOURCES += Cloud/OpenData.cpp
HEADERS += Cloud/OpenData.h

SOURCES += Cloud/SixCycle.cpp
HEADERS += Cloud/SixCycle.h
SOURCES += Cloud/PolarFlow.cpp
HEADERS += Cloud/PolarFlow.h
SOURCES += Cloud/SportTracks.cpp
HEADERS += Cloud/SportTracks.h
SOURCES += Cloud/Nolio.cpp
HEADERS += Cloud/Nolio.h

SOURCES += Train/MonarkController.cpp Train/MonarkConnection.cpp
HEADERS += Train/MonarkController.h Train/MonarkConnection.h
SOURCES += Train/Kettler.cpp Train/KettlerController.cpp Train/KettlerConnection.cpp
HEADERS += Train/Kettler.h Train/KettlerController.h Train/KettlerConnection.h
SOURCES += Train/KettlerRacer.cpp Train/KettlerRacerController.cpp Train/KettlerRacerConnection.cpp
HEADERS += Train/KettlerRacer.h Train/KettlerRacerController.h Train/KettlerRacerConnection.h
SOURCES += Train/Ergofit.cpp Train/ErgofitController.cpp Train/ErgofitConnection.cpp
HEADERS += Train/Ergofit.h Train/ErgofitController.h Train/ErgofitConnection.h
SOURCES += Train/DaumController.cpp Train/Daum.cpp
HEADERS += Train/DaumController.h Train/Daum.h
SOURCES += Train/KurtInRide.cpp Train/KurtSmartControl.cpp
HEADERS += Train/KurtInRide.h Train/KurtSmartControl.h

QT += bluetooth
HEADERS += Train/BT40Controller.h Train/BT40Device.h
SOURCES += Train/BT40Controller.cpp Train/BT40Device.cpp
HEADERS += Train/VMProConfigurator.h Train/VMProWidget.h
SOURCES += Train/VMProConfigurator.cpp Train/VMProWidget.cpp
SOURCES += Train/Ftms.cpp
HEADERS += Train/Ftms.h

QT += charts opengl

# Dashboard uses qt charts, so needs at least Qt 5.7
DEFINES += GC_HAVE_OVERVIEW
HEADERS += Gui/ChartSpace.h Charts/OverviewItems.h Charts/Overview.h
SOURCES += Gui/ChartSpace.cpp Charts/OverviewItems.cpp Charts/Overview.cpp

# generic chart
DEFINES += GC_HAVE_GENERIC
HEADERS += Charts/UserChartWindow.h Charts/UserChartOverviewItem.h Charts/UserChart.h Charts/UserChartData.h \
           Charts/GenericChart.h Charts/GenericPlot.h Charts/GenericSelectTool.h Charts/GenericLegend.h \
	   Charts/GenericAnnotations.h Charts/SeriesIterator.h

SOURCES += Charts/UserChartWindow.cpp Charts/UserChartOverviewItem.cpp Charts/UserChart.cpp Charts/UserChartData.cpp \
           Charts/GenericChart.cpp Charts/GenericPlot.cpp Charts/GenericSelectTool.cpp Charts/GenericLegend.cpp \
	   Charts/GenericAnnotations.cpp

###=====================
### LEX AND YACC SOURCES
###=====================

YACCSOURCES += Core/DataFilter.y \
               FileIO/JsonRideFile.y \
               Core/RideDB.y \
               Train/WorkoutFilter.y \
               Train/TrainerDayAPIQuery.y

LEXSOURCES  += Core/DataFilter.l \
               FileIO/JsonRideFile.l \
               Core/RideDB.l \
               Train/WorkoutFilter.l \
               Train/TrainerDayAPIQuery.l

# Fix parallel build races (YACC headers must exist before LEX runs)
compiler_lex_make_all.depends += compiler_yacc_decl_make_all


###=========================================
### HEADER FILES [scanned by qmake, for moc]
###=========================================

# ANT+
HEADERS  += ANT/ANTChannel.h ANT/ANT.h ANT/ANTlocalController.h ANT/ANTLogger.h ANT/ANTMessage.h ANT/ANTMessages.h

# Charts and associated widgets
HEADERS += Charts/Aerolab.h Charts/AerolabWindow.h Charts/AllPlot.h Charts/AllPlotInterval.h Charts/AllPlotSlopeCurve.h \
           Charts/AllPlotWindow.h Charts/BlankState.h Charts/ChartBar.h Charts/ChartSettings.h \
           Charts/CpPlotCurve.h Charts/CPPlot.h Charts/CriticalPowerWindow.h Charts/DaysScaleDraw.h Charts/ExhaustionDialog.h Charts/GcOverlayWidget.h \
           Charts/GcPane.h Charts/GoldenCheetah.h Charts/HistogramWindow.h \
           Charts/HrPwPlot.h Charts/HrPwWindow.h Charts/IndendPlotMarker.h Charts/IntervalSummaryWindow.h Charts/LogTimeScaleDraw.h \
           Charts/LTMCanvasPicker.h Charts/LTMChartParser.h Charts/LTMOutliers.h Charts/LTMPlot.h Charts/LTMPopup.h \
           Charts/LTMSettings.h Charts/LTMTool.h Charts/LTMTrend2.h Charts/LTMTrend.h Charts/LTMWindow.h \
           Charts/MetadataWindow.h Charts/MUPlot.h Charts/MUPool.h Charts/MUWidget.h Charts/PfPvPlot.h Charts/PfPvWindow.h \
           Charts/PowerHist.h Charts/ReferenceLineDialog.h Charts/RideEditor.h Charts/RideMapWindow.h \
           Charts/ScatterPlot.h Charts/ScatterWindow.h Charts/SmallPlot.h Charts/TreeMapPlot.h \
           Charts/TreeMapWindow.h Charts/ZoneScaleDraw.h Charts/CalendarWindow.h Charts/AgendaWindow.h

# cloud services
HEADERS += Cloud/CalendarDownload.h Cloud/CloudService.h \
           Cloud/LocalFileStore.h Cloud/OAuthDialog.h \
           Cloud/WithingsDownload.h Cloud/Strava.h Cloud/CyclingAnalytics.h Cloud/RideWithGPS.h \
           Cloud/TrainingsTageBuch.h Cloud/Selfloops.h Cloud/SportsPlusHealth.h \
           Cloud/AddCloudWizard.h Cloud/Withings.h Cloud/MeasuresDownload.h Cloud/Xert.h \
           Cloud/Azum.h

# core data
HEADERS += Core/Athlete.h Core/Context.h Core/DataFilter.h Core/FreeSearch.h Core/GcCalendarModel.h Core/GcUpgrade.h \
           Core/IdleTimer.h Core/IntervalItem.h Core/NamedSearch.h Core/RideCache.h Core/RideCacheModel.h Core/RideDB.h \
           Core/RideItem.h Core/Route.h Core/RouteParser.h Core/Season.h Core/SeasonDialogs.h Core/Seasons.h Core/Secrets.h Core/Settings.h \
           Core/Specification.h Core/TimeUtils.h Core/Units.h Core/UserData.h Core/Utils.h \
           Core/Measures.h Core/Quadtree.h Core/SplineLookup.h

# device and file IO or edit
HEADERS += FileIO/ArchiveFile.h FileIO/AthleteBackup.h  FileIO/Bin2RideFile.h FileIO/BinRideFile.h \
           FileIO/CommPort.h \
           FileIO/Computrainer3dpFile.h FileIO/CsvRideFile.h FileIO/DataProcessor.h FileIO/Device.h  \
           FileIO/FitlogParser.h FileIO/FitlogRideFile.h FileIO/FitRideFile.h FileIO/GcRideFile.h FileIO/GpxParser.h \
           FileIO/GpxRideFile.h FileIO/JouleDevice.h FileIO/JsonRideFile.h FileIO/LapsEditor.h FileIO/MacroDevice.h \
           FileIO/ManualRideFile.h FileIO/MoxyDevice.h FileIO/PolarRideFile.h \
           FileIO/PowerTapDevice.h FileIO/PowerTapUtil.h FileIO/PwxRideFile.h FileIO/QuarqParser.h FileIO/QuarqRideFile.h \
           FileIO/RawRideFile.h FileIO/RideAutoImportConfig.h FileIO/RideFileCache.h \
           FileIO/RideFileCommand.h FileIO/RideFile.h FileIO/RideFileTableModel.h  FileIO/Serial.h \
           FileIO/SlfParser.h FileIO/SlfRideFile.h FileIO/SmfParser.h FileIO/SmfRideFile.h FileIO/SmlParser.h \
           FileIO/SmlRideFile.h FileIO/SrdRideFile.h FileIO/SrmRideFile.h FileIO/SyncRideFile.h FileIO/TcxParser.h \
           FileIO/TcxRideFile.h FileIO/TxtRideFile.h FileIO/WkoRideFile.h FileIO/XDataDialog.h FileIO/XDataTableModel.h \
           FileIO/FilterHRV.h FileIO/MeasuresCsvImport.h FileIO/LocationInterpolation.h FileIO/TTSReader.h \
           FileIO/EpmParser.h FileIO/EpmRideFile.h

# GUI components
HEADERS += Gui/AboutDialog.h Gui/AddIntervalDialog.h Gui/AnalysisSidebar.h Gui/ChooseCyclistDialog.h Gui/ColorButton.h \
           Gui/Colors.h Gui/CompareDateRange.h Gui/CompareInterval.h Gui/ComparePane.h Gui/ConfigDialog.h Gui/MiniCalendar.h \
           Gui/DragBar.h Gui/EstimateCPDialog.h Gui/GcCrashDialog.h Gui/GcSideBarItem.h Gui/GcToolBar.h Gui/GcWindowLayout.h \
           Gui/GcWindowRegistry.h Gui/GenerateHeatMapDialog.h Gui/HelpWhatsThis.h Gui/HelpWindow.h \
           Gui/IntervalTreeView.h Gui/LTMSidebar.h Gui/MainWindow.h Gui/NewAthleteWizard.h Gui/Pages.h Gui/RideNavigator.h Gui/RideNavigatorProxy.h \
           Gui/SaveDialogs.h Gui/SearchBox.h Gui/SearchFilterBox.h Gui/SolveCPDialog.h Gui/AthleteTab.h Gui/AbstractView.h Gui/ToolsRhoEstimator.h \
           Gui/Views.h Gui/BatchProcessingDialog.h Gui/DownloadRideDialog.h Gui/ManualActivityWizard.h Gui/NewSideBar.h \
           Gui/MergeActivityWizard.h Gui/RideImportWizard.h Gui/SplitActivityWizard.h Gui/SolverDisplay.h Gui/MetricSelect.h \
           Gui/AddTileWizard.h Gui/NavigationModel.h Gui/AthleteView.h Gui/AthleteConfigDialog.h Gui/AthletePages.h Gui/Perspective.h \
           Gui/PerspectiveDialog.h Gui/SplashScreen.h Gui/StyledItemDelegates.h Gui/MetadataDialog.h Gui/ActionButtonBox.h \
           Gui/MetricOverrideDialog.h Gui/RepeatScheduleWizard.h \
           Gui/Calendar.h Gui/Agenda.h Gui/CalendarData.h Gui/CalendarItemDelegates.h \
           Gui/IconManager.h Gui/FilterSimilarDialog.h

# metrics and models
HEADERS += Metrics/Banister.h Metrics/CPSolver.h Metrics/Estimator.h Metrics/ExtendedCriticalPower.h Metrics/HrZones.h Metrics/PaceZones.h \
           Metrics/PDModel.h Metrics/PMCData.h Metrics/PowerProfile.h Metrics/RideMetadata.h Metrics/RideMetric.h Metrics/SpecialFields.h \
           Metrics/Statistic.h Metrics/UserMetricParser.h Metrics/UserMetricSettings.h Metrics/VDOTCalculator.h Metrics/WPrime.h Metrics/Zones.h \
           Metrics/BlinnSolver.h Metrics/FastKmeans.h

## Planning and Compliance
HEADERS += Planning/PlanningWindow.h

# contrib
HEADERS += ../contrib/qtsolutions/codeeditor/codeeditor.h ../contrib/qtsolutions/json/mvjson.h \
           ../contrib/qtsolutions/flowlayout/flowlayout.h \
           ../contrib/qtsolutions/qwtcurve/qwt_plot_gapped_curve.h  ../contrib/qxt/src/qxtspanslider.h \
           ../contrib/qxt/src/qxtspanslider_p.h ../contrib/qxt/src/qxtstringspinbox.h ../contrib/qzip/zipreader.h \
           ../contrib/qzip/zipwriter.h ../contrib/lmfit/lmcurve.h  ../contrib/lmfit/lmcurve_tyd.h \
           ../contrib/lmfit/lmmin.h  ../contrib/lmfit/lmstruct.h \
           ../contrib/boost/GeometricTools_BSplineCurve.h \
           ../contrib/kmeans/kmeans_dataset.h ../contrib/kmeans/kmeans_general_functions.h ../contrib/kmeans/hamerly_kmeans.h \
           ../contrib/kmeans/kmeans.h ../contrib/kmeans/original_space_kmeans.h ../contrib/kmeans/triangle_inequality_base_kmeans.h \
           ../contrib/voronoi/Voronoi.h


# Train View
HEADERS += Train/AddDeviceWizard.h Train/CalibrationData.h Train/ComputrainerController.h Train/Computrainer.h Train/DeviceConfiguration.h \
           Train/DeviceTypes.h Train/DialWindow.h Train/TrainerDayDownloadDialog.h Train/TrainerDay.h Train/ErgFile.h Train/ErgFilePlot.h \
           Train/Library.h Train/LibraryParser.h Train/MeterWidget.h Train/NullController.h Train/RealtimeController.h \
           Train/RealtimeData.h Train/RealtimePlot.h Train/RealtimePlotWindow.h Train/RemoteControl.h Train/SpinScanPlot.h \
           Train/SpinScanPlotWindow.h Train/SpinScanPolarPlot.h Train/GarminServiceHelper.h Train/PhysicsUtility.h Train/BicycleSim.h \
           Train/PolynomialRegression.h Train/MultiRegressionizer.h Train/StravaRoutesDownload.h \
           Train/VideoSyncFileBase.h Train/ErgFileBase.h \
           Train/ModelFilter.h Train/MultiFilterProxyModel.h Train/WorkoutFilter.h Train/FilterEditor.h \
           Train/WorkoutFilterBox.h Train/TagBar.h Train/Taggable.h Train/TagStore.h Train/TagWidget.h \
           Train/TrainerDayAPIQuery.h Train/TrainerDayAPIDialog.h Train/ElevationChartWindow.h

HEADERS += Train/TrainBottom.h Train/TrainDB.h Train/TrainSidebar.h \
           Train/VideoLayoutParser.h Train/VideoSyncFile.h Train/WorkoutPlotWindow.h Train/WebPageWindow.h \
           Train/WorkoutWidget.h Train/WorkoutWidgetItems.h Train/WorkoutWindow.h Train/WorkoutWizard.h Train/ZwoParser.h \
           Train/LiveMapWebPageWindow.h Train/ScalingLabel.h \
           Train/InfoWidget.h Train/PowerInfoWidget.h Train/PowerZonesWidget.h Train/RatingWidget.h \
           Train/ErgOverview.h Train/Shy.h \
           Train/WorkoutTagWrapper.h \
           Train/MenuProvider.h Train/WorkoutMenuProvider.h


###=============
### SOURCE FILES
###=============

## ANT+
SOURCES += ANT/ANTChannel.cpp ANT/ANT.cpp ANT/ANTlocalController.cpp ANT/ANTLogger.cpp ANT/ANTMessage.cpp

## Charts and related
SOURCES += Charts/Aerolab.cpp Charts/AerolabWindow.cpp Charts/AllPlot.cpp Charts/AllPlotInterval.cpp Charts/AllPlotSlopeCurve.cpp \
           Charts/AllPlotWindow.cpp Charts/BlankState.cpp Charts/ChartBar.cpp Charts/ChartSettings.cpp \
           Charts/CPPlot.cpp Charts/CpPlotCurve.cpp Charts/CriticalPowerWindow.cpp Charts/ExhaustionDialog.cpp Charts/GcOverlayWidget.cpp Charts/GcPane.cpp \
           Charts/GoldenCheetah.cpp Charts/HistogramWindow.cpp Charts/HrPwPlot.cpp \
           Charts/HrPwWindow.cpp Charts/IndendPlotMarker.cpp Charts/IntervalSummaryWindow.cpp Charts/LogTimeScaleDraw.cpp \
           Charts/LTMCanvasPicker.cpp Charts/LTMChartParser.cpp Charts/LTMOutliers.cpp Charts/LTMPlot.cpp Charts/LTMPopup.cpp \
           Charts/LTMSettings.cpp Charts/LTMTool.cpp Charts/LTMTrend.cpp Charts/LTMWindow.cpp \
           Charts/MetadataWindow.cpp Charts/MUPlot.cpp Charts/MUWidget.cpp Charts/PfPvPlot.cpp Charts/PfPvWindow.cpp \
           Charts/PowerHist.cpp Charts/ReferenceLineDialog.cpp Charts/RideEditor.cpp Charts/RideMapWindow.cpp \
           Charts/ScatterPlot.cpp Charts/ScatterWindow.cpp Charts/SmallPlot.cpp Charts/TreeMapPlot.cpp \
           Charts/TreeMapWindow.cpp Charts/CalendarWindow.cpp Charts/AgendaWindow.cpp

## Cloud Services / Web resources
SOURCES += Cloud/CalendarDownload.cpp Cloud/CloudService.cpp \
           Cloud/LocalFileStore.cpp Cloud/OAuthDialog.cpp \
           Cloud/WithingsDownload.cpp Cloud/Strava.cpp Cloud/CyclingAnalytics.cpp Cloud/RideWithGPS.cpp \
           Cloud/TrainingsTageBuch.cpp Cloud/Selfloops.cpp Cloud/SportsPlusHealth.cpp \
           Cloud/AddCloudWizard.cpp Cloud/Withings.cpp Cloud/MeasuresDownload.cpp Cloud/Xert.cpp \
           Cloud/Azum.cpp

## Core Data Structures
SOURCES += Core/Athlete.cpp Core/Context.cpp Core/DataFilter.cpp Core/FreeSearch.cpp Core/GcUpgrade.cpp Core/IdleTimer.cpp \
           Core/IntervalItem.cpp Core/main.cpp Core/NamedSearch.cpp Core/RideCache.cpp Core/RideCacheModel.cpp Core/RideItem.cpp \
           Core/Route.cpp Core/RouteParser.cpp Core/Season.cpp Core/SeasonDialogs.cpp Core/Seasons.cpp Core/Settings.cpp Core/Specification.cpp \
           Core/TimeUtils.cpp Core/Units.cpp Core/UserData.cpp Core/Utils.cpp \
           Core/Measures.cpp Core/Quadtree.cpp Core/SplineLookup.cpp

## File and Device IO and Editing
SOURCES += FileIO/ArchiveFile.cpp FileIO/AthleteBackup.cpp FileIO/Bin2RideFile.cpp FileIO/BinRideFile.cpp \
           FileIO/CommPort.cpp \
           FileIO/Computrainer3dpFile.cpp FileIO/CsvRideFile.cpp FileIO/DataProcessor.cpp FileIO/Device.cpp \
           FileIO/FitlogParser.cpp FileIO/FitlogRideFile.cpp FileIO/FitRideFile.cpp FileIO/FixAeroPod.cpp FileIO/FixDeriveDistance.cpp \
           FileIO/FixDeriveHeadwind.cpp FileIO/FixDerivePower.cpp FileIO/FixDeriveTorque.cpp FileIO/FixElevation.cpp FileIO/FixLapSwim.cpp \
           FileIO/FixFreewheeling.cpp FileIO/FixGaps.cpp FileIO/FixGPS.cpp FileIO/FixRunningCadence.cpp FileIO/FixRunningPower.cpp \
           FileIO/FixHRSpikes.cpp FileIO/FixMoxy.cpp FileIO/FixPower.cpp FileIO/FixSmO2.cpp FileIO/FixSpeed.cpp FileIO/FixSpikes.cpp \
           FileIO/FixTorque.cpp FileIO/GcRideFile.cpp FileIO/GpxParser.cpp FileIO/GpxRideFile.cpp FileIO/JouleDevice.cpp FileIO/LapsEditor.cpp \
           FileIO/MacroDevice.cpp FileIO/ManualRideFile.cpp FileIO/MoxyDevice.cpp \
           FileIO/PolarRideFile.cpp FileIO/PowerTapDevice.cpp FileIO/PowerTapUtil.cpp FileIO/PwxRideFile.cpp FileIO/QuarqParser.cpp \
           FileIO/QuarqRideFile.cpp FileIO/RawRideFile.cpp FileIO/RideAutoImportConfig.cpp \
           FileIO/RideFileCache.cpp FileIO/RideFileCommand.cpp FileIO/RideFile.cpp FileIO/RideFileTableModel.cpp \
           FileIO/Serial.cpp FileIO/SlfParser.cpp FileIO/SlfRideFile.cpp FileIO/SmfParser.cpp FileIO/SmfRideFile.cpp FileIO/SmlParser.cpp \
           FileIO/SmlRideFile.cpp FileIO/Snippets.cpp FileIO/SrdRideFile.cpp FileIO/SrmRideFile.cpp FileIO/SyncRideFile.cpp \
           FileIO/TacxCafRideFile.cpp FileIO/TcxParser.cpp FileIO/TcxRideFile.cpp FileIO/TxtRideFile.cpp FileIO/WkoRideFile.cpp \
           FileIO/XDataDialog.cpp FileIO/XDataTableModel.cpp FileIO/FilterHRV.cpp FileIO/MeasuresCsvImport.cpp \
           FileIO/LocationInterpolation.cpp FileIO/TTSReader.cpp FileIO/EpmRideFile.cpp FileIO/EpmParser.cpp

## GUI Elements and Dialogs
SOURCES += Gui/AboutDialog.cpp Gui/AddIntervalDialog.cpp Gui/AnalysisSidebar.cpp Gui/ChooseCyclistDialog.cpp Gui/ColorButton.cpp \
           Gui/Colors.cpp Gui/CompareDateRange.cpp Gui/CompareInterval.cpp Gui/ComparePane.cpp Gui/ConfigDialog.cpp Gui/MiniCalendar.cpp \
           Gui/DragBar.cpp Gui/EstimateCPDialog.cpp Gui/GcCrashDialog.cpp Gui/GcSideBarItem.cpp Gui/GcToolBar.cpp Gui/GcWindowLayout.cpp \
           Gui/GcWindowRegistry.cpp Gui/GenerateHeatMapDialog.cpp Gui/HelpWhatsThis.cpp Gui/HelpWindow.cpp \
           Gui/IntervalTreeView.cpp Gui/LTMSidebar.cpp Gui/MainWindow.cpp Gui/NewAthleteWizard.cpp Gui/Pages.cpp Gui/RideNavigator.cpp Gui/SaveDialogs.cpp \
           Gui/SearchBox.cpp Gui/SearchFilterBox.cpp Gui/SolveCPDialog.cpp Gui/AthleteTab.cpp Gui/AbstractView.cpp Gui/ToolsRhoEstimator.cpp Gui/Views.cpp \
           Gui/BatchProcessingDialog.cpp Gui/DownloadRideDialog.cpp Gui/ManualActivityWizard.cpp Gui/EditUserMetricDialog.cpp Gui/NewSideBar.cpp \
           Gui/MergeActivityWizard.cpp Gui/RideImportWizard.cpp Gui/SplitActivityWizard.cpp Gui/SolverDisplay.cpp Gui/MetricSelect.cpp \
           Gui/AddTileWizard.cpp Gui/NavigationModel.cpp Gui/AthleteView.cpp Gui/AthleteConfigDialog.cpp Gui/AthletePages.cpp Gui/Perspective.cpp \
           Gui/PerspectiveDialog.cpp Gui/SplashScreen.cpp Gui/StyledItemDelegates.cpp Gui/MetadataDialog.cpp Gui/ActionButtonBox.cpp \
           Gui/MetricOverrideDialog.cpp Gui/RepeatScheduleWizard.cpp \
           Gui/Calendar.cpp Gui/Agenda.cpp Gui/CalendarData.cpp Gui/CalendarItemDelegates.cpp \
           Gui/IconManager.cpp Gui/FilterSimilarDialog.cpp

## Models and Metrics
SOURCES += Metrics/aBikeScore.cpp Metrics/aCoggan.cpp Metrics/AerobicDecoupling.cpp Metrics/Banister.cpp Metrics/BasicRideMetrics.cpp \
           Metrics/BikeScore.cpp Metrics/Coggan.cpp Metrics/CPSolver.cpp Metrics/DanielsPoints.cpp Metrics/Estimator.cpp \
           Metrics/ExtendedCriticalPower.cpp Metrics/GOVSS.cpp Metrics/HrTimeInZone.cpp Metrics/HrZones.cpp Metrics/LeftRightBalance.cpp \
           Metrics/PaceTimeInZone.cpp Metrics/PaceZones.cpp Metrics/PDModel.cpp Metrics/PeakPace.cpp Metrics/PeakPower.cpp Metrics/PeakHr.cpp \
           Metrics/PMCData.cpp Metrics/PowerProfile.cpp Metrics/RideMetadata.cpp Metrics/RideMetric.cpp Metrics/RunMetrics.cpp \
           Metrics/SwimMetrics.cpp Metrics/SpecialFields.cpp Metrics/Statistic.cpp Metrics/SustainMetric.cpp Metrics/SwimScore.cpp \
           Metrics/TimeInZone.cpp Metrics/TRIMPPoints.cpp Metrics/UserMetric.cpp Metrics/UserMetricParser.cpp Metrics/VDOTCalculator.cpp \
           Metrics/VDOT.cpp Metrics/WattsPerKilogram.cpp Metrics/WPrime.cpp Metrics/Zones.cpp Metrics/HrvMetrics.cpp Metrics/BlinnSolver.cpp \
           Metrics/RowMetrics.cpp Metrics/FastKmeans.cpp

## Planning and Compliance
SOURCES += Planning/PlanningWindow.cpp

## Contributed solutions
SOURCES += ../contrib/qtsolutions/codeeditor/codeeditor.cpp ../contrib/qtsolutions/json/mvjson.cpp \
           ../contrib/qtsolutions/flowlayout/flowlayout.cpp \
           ../contrib/qtsolutions/qwtcurve/qwt_plot_gapped_curve.cpp \
           ../contrib/qxt/src/qxtspanslider.cpp ../contrib/qxt/src/qxtstringspinbox.cpp ../contrib/qzip/zip.cpp \
           ../contrib/lmfit/lmcurve.c ../contrib/lmfit/lmmin.c \
           ../contrib/kmeans/kmeans_dataset.cpp ../contrib/kmeans/kmeans_general_functions.cpp ../contrib/kmeans/hamerly_kmeans.cpp \
           ../contrib/kmeans/kmeans.cpp ../contrib/kmeans/original_space_kmeans.cpp ../contrib/kmeans/triangle_inequality_base_kmeans.cpp \
           ../contrib/voronoi/Voronoi.cpp


## Train View Components
SOURCES += Train/AddDeviceWizard.cpp Train/CalibrationData.cpp Train/ComputrainerController.cpp Train/Computrainer.cpp Train/DeviceConfiguration.cpp \
           Train/DeviceTypes.cpp Train/DialWindow.cpp Train/TrainerDay.cpp Train/TrainerDayDownloadDialog.cpp Train/ErgFile.cpp Train/ErgFilePlot.cpp \
           Train/Library.cpp Train/LibraryParser.cpp Train/MeterWidget.cpp Train/NullController.cpp Train/RealtimeController.cpp \
           Train/RealtimeData.cpp Train/RealtimePlot.cpp Train/RealtimePlotWindow.cpp Train/RemoteControl.cpp Train/SpinScanPlot.cpp \
           Train/SpinScanPlotWindow.cpp Train/SpinScanPolarPlot.cpp Train/GarminServiceHelper.cpp Train/PhysicsUtility.cpp Train/BicycleSim.cpp \
           Train/PolynomialRegression.cpp Train/StravaRoutesDownload.cpp \
           Train/VideoSyncFileBase.cpp Train/ErgFileBase.cpp \
           Train/ModelFilter.cpp Train/MultiFilterProxyModel.cpp Train/WorkoutFilter.cpp Train/FilterEditor.cpp \
           Train/WorkoutFilterBox.cpp Train/TagBar.cpp Train/TagWidget.cpp \
           Train/TrainerDayAPIQuery.cpp Train/TrainerDayAPIDialog.cpp Train/ElevationChartWindow.cpp

SOURCES += Train/TrainBottom.cpp Train/TrainDB.cpp Train/TrainSidebar.cpp \
           Train/VideoLayoutParser.cpp Train/VideoSyncFile.cpp Train/WorkoutPlotWindow.cpp Train/WebPageWindow.cpp \
           Train/WorkoutWidget.cpp Train/WorkoutWidgetItems.cpp Train/WorkoutWindow.cpp Train/WorkoutWizard.cpp Train/ZwoParser.cpp \
           Train/LiveMapWebPageWindow.cpp Train/ScalingLabel.cpp \
           Train/InfoWidget.cpp Train/PowerInfoWidget.cpp Train/PowerZonesWidget.cpp Train/RatingWidget.cpp \
           Train/ErgOverview.cpp Train/Shy.cpp \
           Train/WorkoutTagWrapper.cpp \
           Train/WorkoutMenuProvider.cpp

## Crash Handling
win32-msvc* {
  SOURCES += Core/WindowsCrashHandler.cpp
}

###======================================
### PENDING SOURCE FILES [not active yet]
###======================================

DEFERRES += Core/RouteWindow.h Core/RouteWindow.cpp Core/RouteItem.h Core/RouteItem.cpp

###====================
### MISCELLANEOUS FILES
###====================

OTHER_FILES +=   Resources/python/library.py Python/SIP/goldencheetah.sip


###==========================
### PRECOMPILED HEADER
###==========================
macx {
    message("Disabling Precompiled Headers on macOS to avoid multi-arch Clang errors.")
} else {
    message("Enabling precompile_header")
    PRECOMPILED_HEADER = stable.h
    CONFIG += precompile_header
}

###============================================================================
### Disable Precompiled Header for C files
### The PCH contains C++ specific headers (Qt, STL) which causes compilation errors
### when the PCH is forced upon C files by gcc.
###============================================================================

for(src, SOURCES) {
    contains(src, .*\.c$) {
        eval($${src}.CONFIG -= precompile_header)
    }
}
