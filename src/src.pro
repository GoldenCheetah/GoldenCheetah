###############################################################################
#                                                                             #
#              ***  SECTION ONE - CORE CONFIGURATION  ***                     #
#                                                                             #
#                                                                             #
###############################################################################


###==========================
### IMPORT USER CONFIGURATION
###==========================

# You must configure your settings by copying from gcconfig.pri.in.
# The file contains instructions on settings to make
include(gcconfig.pri)

# You can also define your own local source to add to build
HEADERS += $${LOCALHEADERS}
SOURCE += $${LOCALSOURCE}


###=====================
### GOLDENCHEETAH TARGET
###=====================

DEPENDPATH += .
TEMPLATE = app
TARGET = GoldenCheetah

!isEmpty(APP_NAME) { TARGET = $${APP_NAME} }
CONFIG(debug, debug|release) { QMAKE_CXXFLAGS += -DGC_DEBUG }


###======================================================
### QT MODULES [we officially support QT4.8.6+ or QT5.6+]
###======================================================

# always
QT += xml sql network script svg concurrent

lessThan(QT_MAJOR_VERSION, 5) {

    ## QT4 specific modules
    QT += webkit

} else {

    ## QT5 specific modules
    QT += webkitwidgets widgets concurrent serialport
    macx {
        QT += macextras webenginewidgets
    } else {
        QT += multimedia multimediawidgets
    }
}


###=======================================================================
### DISTRIBUTED SOURCE [Snaffled in sources to avoid further dependencies]
###=======================================================================

# qwt, qxt, libz, json and qwtcurve
INCLUDEPATH += ../qwt/src ../qxt/src ../qtsolutions/json ../qtsolutions/qwtcurve
DEFINES += QXT_STATIC

# local qtsoap handling
include(../qtsolutions/soap/qtsoap.pri)
DEFINES += GC_HAVE_SOAP

# to make sure we are toolchain neutral we NEVER refer to a lib
# via file extensions .lib or .a in src.pro unless the section is
# platform specific. Instead we use directives -Ldir and -llib
win32 {

    #QWT is configured to build 2 libs (release/debug) on win32 (see qwtbuild.pri)
    CONFIG(release, debug|release){
    LIBS += -L../qwt/lib -lqwt
    }
    CONFIG(debug, debug|release) {
    LIBS += -L../qwt/lib -lqwtd
    }

} else {
    #QWT is configured to build 1 lib for all other OS (see qwtbuild.pri)
    LIBS += -L../qwt/lib -lqwt
}

# compress and math libs must be defined in gcconfig.pri
# if they're not part of the QT include
INCLUDEPATH += $${LIBZ_INCLUDE}
LIBS += $${LIBZ_LIBS}

###===============================
### PLATFORM SPECIFIC DEPENDENCIES
###===============================

# Microsoft Visual Studion toolchain dependencies
*msvc2015 {

    # we need windows kit 8.2 or higher with MSVC, offer default location
    isEmpty(WINKIT_INSTALL) WINKIT_INSTALL= "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x64"
    LIBS += -L$${WINKIT_INSTALL} -lGdi32 -lUser32

} else {
    # gnu toolchain wants math libs
    LIBS += -lm
}

# windows icon and use QT zlib, not sure why different but keep for now
win32 {

    RC_FILE = windowsico.rc
    INCLUDEPATH += ./win32 $${QT_INSTALL_PREFIX}/src/3rdparty/zlib
    LIBS += -lws2_32

} else {

    RC_FILE = images/gc.icns
}

macx {

    # we have our own plist
    QMAKE_INFO_PLIST = ./mac/Info.plist.app

    # on mac we use native buttons and video, but have native fullscreen support
    LIBS    += -lobjc -framework IOKit -framework AppKit -framework QTKit
    HEADERS += \
            QtMacVideoWindow.h \
            QtMacSegmentedButton.h \
            QtMacButton.h

    OBJECTIVE_SOURCES += \
            QtMacVideoWindow.mm \
            QtMacSegmentedButton.mm \
            QtMacButton.mm

} else {

    # not on mac we need our own full screen support and segment control button
    HEADERS += QTFullScreen.h
    SOURCES += QTFullScreen.cpp

    HEADERS += ../qtsolutions/segmentcontrol/qtsegmentcontrol.h
    SOURCES += ../qtsolutions/segmentcontrol/qtsegmentcontrol.cpp

    # we now have videowindow, it will do nothing
    HEADERS += VideoWindow.h
    SOURCES += VideoWindow.cpp
}


###=================
### LANGUAGE SUPPORT
###=================

TRANSLATIONS = translations/gc_fr.ts \
               translations/gc_ja.ts \
               translations/gc_it.ts \
               translations/gc_pt-br.ts \
               translations/gc_de.ts \
               translations/gc_cs.ts \
               translations/gc_es.ts \
               translations/gc_pt.ts \
               translations/gc_ru.ts \
               translations/gc_zh-tw.ts

# need lrelease to generate qm files
isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    unix:!macx {QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease-qt4 }
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

# how to run lrelease
isEmpty(TS_DIR):TS_DIR = translations
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$TS_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM
PRE_TARGETDEPS += compiler_TSQM_make_all


###==========
### RESOURCES
###==========

RESOURCES = application.qrc \
            RideWindow.qrc



###############################################################################
#                                                                             #
#         ***  SECTION TWO - OPTIONAL LIBRARIES AND FEATURES  ***             #
#                                                                             #
#                                                                             #
###############################################################################



###====================
### OPTIONAL => KQOAUTH
###====================

unix:!macx {

    # build from version in repo for Linux builds since
    # kqoauth is not packaged for the Debian and this makes
    # life much easier for the package maintainer
    INCLUDEPATH += ../kqoauth
    LIBS        += ../kqoauth/libkqoauth.a
    DEFINES     += GC_HAVE_KQOAUTH

} else {

    !isEmpty(KQOAUTH_INSTALL) {

        # we will work out the rest if you tell us where it is installed
        isEmpty(KQOAUTH_INCLUDE) { KQOAUTH_INCLUDE = $${KQOAUTH_INSTALL}/src }
        isEmpty(KQOAUTH_LIBS)    { KQOAUTH_LIBS    = -L$${KQOAUTH_INSTALL}/lib -lkqoauth }

        INCLUDEPATH += $${KQOAUTH_INCLUDE}
        LIBS        += $${KQOAUTH_LIBS}
        DEFINES     += GC_HAVE_KQOAUTH
    }
}

# if we have it we can add twitter support
contains(DEFINES, "GC_HAVE_KQOAUTH") {
        SOURCES     += TwitterDialog.cpp
        HEADERS     += TwitterDialog.h
}


###=======================================================
### OPTIONAL => D2XX FOR FTDI DRIVERS ON WINDOWS PLATFORMS
###=======================================================

!isEmpty(D2XX_INCLUDE) {

    DEFINES     += GC_HAVE_D2XX
    INCLUDEPATH += $${D2XX_INCLUDE}

    !isEmpty(D2XX_LIBS) { LIBS += $${D2XX_LIBS} }
    unix                { LIBS += -ldl }

    HEADERS     += D2XX.h
    SOURCES     += D2XX.cpp
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
    HEADERS     += SrmDevice.h
    SOURCES     += SrmDevice.cpp
}


###==================
### OPTIONAL => QWT3D
###==================

!isEmpty(QWT3D_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(QWT3D_INCLUDE) { QWT3D_INCLUDE = $${QWT3D_INSTALL}/include }
    isEmpty(QWT3D_LIBS)    { QWT3D_LIBS    = -L$${QWT3D_INSTALL}/lib -lqwtplot3d }

    DEFINES     += GC_HAVE_QWTPLOT3D
    INCLUDEPATH += $${QWT3D_INCLUDE}
    LIBS        += $${QWT3D_LIBS}

    # additional dependencies for 3d
    unix:!macx { LIBS += -lGLU }
    QT          += opengl

    # add 3d plot
    HEADERS     += ModelPlot.h ModelWindow.h
    SOURCES     += ModelPlot.cpp ModelWindow.cpp
}


###=====================================
### OPTIONAL => GOOGLE KML IMPORT EXPORT
###=====================================

!isEmpty(KML_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(KML_INCLUDE) { KML_INCLUDE = $${KML_INSTALL}/include }
    isEmpty(KML_LIBS)    { KML_LIBS    = -L$${KML_INSTALL}/lib/ \
                                         -lkmldom -lkmlconvenience -lkmlengine -lkmlbase
    }

    # on MS VS the linker wants /LTCG for libkmldom due to
    # "MSIL .netmodule or module compiled with /GL found"
    *msvc2015 { QMAKE_LFLAGS +=  /LTCG }

    DEFINES     += GC_HAVE_KML
    INCLUDEPATH += $${KML_INCLUDE}  $${BOOST_INCLUDE}
    LIBS        += $${KML_LIBS}

    # add kml file i/o
    SOURCES     += KmlRideFile.cpp
    HEADERS     += KmlRideFile.h
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

    # add caldav and diary functions
    HEADERS     += ICalendar.h DiaryWindow.h CalDAV.h
    SOURCES     += ICalendar.cpp DiaryWindow.cpp CalDAV.cpp
}


###===================
### OPTIONAL => LIBUSB
###===================

!isEmpty(LIBUSB_INSTALL) {

    # we will work out the rest if you tell use where it is installed
    isEmpty(LIBUSB_INCLUDE) { LIBUSB_INCLUDE = $${LIBUSB_INSTALL}/include }
    isEmpty(LIBUSB_LIBS)    {
        # needs fixing for msvc toolchain
        unix  { LIBUSB_LIBS = -L$${LIBUSB_INSTALL}/lib -lusb }
        win32 { LIBUSB_LIBS = -L$${LIBUSB_INSTALL}/lib/gcc -lusb }
    }

    DEFINES     += GC_HAVE_LIBUSB
    INCLUDEPATH += $${LIBUSB_INCLUDE}
    LIBS        += $${LIBUSB_LIBS}

    # lots of dependents
    SOURCES     += LibUsb.cpp EzUsb.c Fortius.cpp FortiusController.cpp
    HEADERS     += LibUsb.h EzUsb.h Fortius.cpp FortiusController.h
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

    SOURCES += USBXpress.cpp
    HEADERS += USBXpress.h
}


###=============================================================
### OPTIONAL => VLC [Windows and Unix. OSX uses QuickTime Video]
###=============================================================

!isEmpty(VLC_INSTALL) {

    # not on a mac as they use quicktime video
    !macx {

        # we will work out the rest if you tell use where it is installed
        isEmpty(VLC_INCLUDE) { VLC_INCLUDE = $${VLC_INSTALL}/include }
        isEmpty(VLC_LIBS)    { VLC_LIBS    = -L$${VLC_INSTALL}/lib -lvlc -lvlccore }

        DEFINES     += GC_HAVE_VLC
        INCLUDEPATH += $${VLC_INCLUDE}
        LIBS        += $${VLC_LIBS}
    }
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
### OPTIONAL => HTTP API WEB SERVICES
###==================================

!isEmpty (HTPATH) {

    INCLUDEPATH += $$HTPATH
    DEPENDPATH += $$HTPATH

    DEFINES += GC_WANT_HTTP

    HEADERS +=  APIWebService.h
    SOURCES +=  APIWebService.cpp

    HEADERS +=  $$HTPATH/httpglobal.h \
                $$HTPATH/httplistener.h \
                $$HTPATH/httpconnectionhandler.h \
                $$HTPATH/httpconnectionhandlerpool.h \
                $$HTPATH/httprequest.h \
                $$HTPATH/httpresponse.h \
                $$HTPATH/httpcookie.h \
                $$HTPATH/httprequesthandler.h \
                $$HTPATH/httpsession.h \
                $$HTPATH/httpsessionstore.h \
                $$HTPATH/staticfilecontroller.h
    SOURCES +=  $$HTPATH/httpglobal.cpp \
                $$HTPATH/httplistener.cpp \
                $$HTPATH/httpconnectionhandler.cpp \
                $$HTPATH/httpconnectionhandlerpool.cpp \
                $$HTPATH/httprequest.cpp \
                $$HTPATH/httpresponse.cpp \
                $$HTPATH/httpcookie.cpp \
                $$HTPATH/httprequesthandler.cpp \
                $$HTPATH/httpsession.cpp \
                $$HTPATH/httpsessionstore.cpp \
                $$HTPATH/staticfilecontroller.cpp
}


###=====================================================
### OPTIONAL => CLOUD DB [Google App Engine Integration]
###=====================================================

##----------------------------------------------##
## CloudDB is only supported on QT5.5 or higher ##
##----------------------------------------------##

notsupported = "INFO: CloudDB requires version QT >= 5.5, no support for"
notsupported += $${QT_VERSION}

equals(CloudDB, active) {

    greaterThan(QT_MAJOR_VERSION, 4) {

        greaterThan(QT_MINOR_VERSION, 4) {

            HEADERS += CloudDBChart.h CloudDBCommon.h \
                       CloudDBCurator.h CloudDBStatus.h
            SOURCES += CloudDBChart.cpp CloudDBCommon.cpp \
                       CloudDBCurator.cpp CloudDBStatus.cpp
            DEFINES += GC_HAS_CLOUD_DB

        } else {

            # QT5 but not 5.5 or higher
            message($$notsupported)
        }

    } else {

        # QT4 not supported
        message($$notsupported)
    }
}



###############################################################################
#                                                                             #
#         ***  SECTION THREE - GOLDENCHEETAH SOURCE FILES  ***                #
#                                                                             #
#                                                                             #
###############################################################################



###===========================================
### FEATURES ENABLED WHEN HAVE QT5 [or higher]
###===========================================

greaterThan(QT_MAJOR_VERSION, 4) {

    # Features that only work with QT5 or higher
    SOURCES += Dropbox.cpp
    HEADERS += Dropbox.h
    SOURCES += GoogleDrive.cpp
    HEADERS += GoogleDrive.h
    SOURCES += Monark.cpp MonarkController.cpp MonarkConnection.cpp
    HEADERS += Monark.h MonarkController.h MonarkConnection.h
}


###=====================
### LEX AND YACC SOURCES
###=====================

YACCSOURCES += DataFilter.y \
               JsonRideFile.y \
               WithingsParser.y \
               RideDB.y

LEXSOURCES  += DataFilter.l \
               JsonRideFile.l \
               WithingsParser.l \
               RideDB.l


###=========================================
### HEADER FILES [scanned by qmake, for moc]
###=========================================

HEADERS  += \
        AboutDialog.h \
        AddDeviceWizard.h \
        AddIntervalDialog.h \
        Aerolab.h \
        AerolabWindow.h \
        AllPlot.h \
        AllPlotInterval.h \
        AllPlotSlopeCurve.h \
        AllPlotWindow.h \
        AnalysisSidebar.h \
        ANTChannel.h \
        ANT.h \
        ANTlocalController.h \
        ANTLogger.h \
        ANTMessage.h \
        ANTMessages.h \
        AthleteBackup.h \
        Athlete.h \
        BatchExportDialog.h \
        BestIntervalDialog.h \
        Bin2RideFile.h \
        BingMap.h \
        BinRideFile.h \
        BlankState.h \
        CalendarDownload.h \
        ChartBar.h \
        ChartSettings.h \
        ChooseCyclistDialog.h \
        ../qtsolutions/codeeditor/codeeditor.h \
        ColorButton.h \
        Colors.h \
        CommPort.h \
        CompareDateRange.h \
        CompareInterval.h \
        ComparePane.h \
        Computrainer3dpFile.h \
        ComputrainerController.h \
        Computrainer.h \
        ConfigDialog.h \
        Context.h \
        CpPlotCurve.h \
        CPPlot.h \
        CriticalPowerWindow.h \
        CsvRideFile.h \
        DataFilter.h \
        DataProcessor.h \
        DaysScaleDraw.h \
        DeviceConfiguration.h \
        Device.h \
        DeviceTypes.h \
        DialWindow.h \
        DiarySidebar.h \
        DownloadRideDialog.h \
        DragBar.h \
        ErgDBDownloadDialog.h \
        ErgDB.h \
        ErgFile.h \
        ErgFilePlot.h \
        ExtendedCriticalPower.h \
        FileStore.h \
        FitlogParser.h \
        FitlogRideFile.h \
        FitRideFile.h \
        FreeSearch.h \
        GcCalendarModel.h \
        GcCrashDialog.h \
        GcOverlayWidget.h \
        GcPane.h \
        GcRideFile.h \
        GcScopeBar.h \
        GcSideBarItem.h \
        GcToolBar.h \
        GcUpgrade.h \
        GcWindowLayout.h \
        GcWindowRegistry.h \
        GenerateHeatMapDialog.h \
        GoldenCheetah.h \
        GoogleMapControl.h \
        GProgressDialog.h \
        GpxParser.h \
        GpxRideFile.h \
        HelpWhatsThis.h \
        HelpWindow.h \
        HistogramWindow.h \
        HomeWindow.h \
        HrPwPlot.h \
        HrPwWindow.h \
        HrZones.h \
        IndendPlotMarker.h \
        IntervalItem.h \
        IntervalSummaryWindow.h \
        IntervalTreeView.h \
        JouleDevice.h \
        JsonRideFile.h \
        LapsEditor.h \
        Library.h \
        LibraryParser.h \
        LocalFileStore.h \
        LogTimeScaleDraw.h \
        LTMCanvasPicker.h \
        LTMChartParser.h \
        LTMOutliers.h \
        LTMPlot.h \
        LTMPopup.h \
        LTMSettings.h \
        LTMSidebar.h \
        LTMTool.h \
        LTMTrend2.h \
        LTMTrend.h \
        LTMWindow.h \
        MacroDevice.h \
        MainWindow.h \
        ManualRideDialog.h \
        ManualRideFile.h \
        MergeActivityWizard.h \
        MetadataWindow.h \
        MeterWidget.h \
        MoxyDevice.h \
        MUPlot.h \
        MUPool.h \
        MUWidget.h \
        NamedSearch.h \
        NewCyclistDialog.h \
        NullController.h \
        OAuthDialog.h \
        PaceZones.h \
        Pages.h \
        PDModel.h \
        PfPvPlot.h \
        PfPvWindow.h \
        PMCData.h \
        PolarRideFile.h \
        PowerHist.h \
        PowerTapDevice.h \
        PowerTapUtil.h \
        PwxRideFile.h \
        ../qtsolutions/json/mvjson.h \
        ../qtsolutions/qwtcurve/qwt_plot_gapped_curve.h \
        QuarqParser.h \
        QuarqRideFile.h \
        ../qxt/src/qxtspanslider.h \
        ../qxt/src/qxtspanslider_p.h \
        ../qxt/src/qxtstringspinbox.h \
        ../qzip/zipreader.h \
        ../qzip/zipwriter.h \
        RawRideFile.h \
        RealtimeController.h \
        RealtimeData.h \
        RealtimePlot.h \
        RealtimePlotWindow.h \
        ReferenceLineDialog.h \
        RemoteControl.h \
        RideAutoImportConfig.h \
        RideCache.h \
        RideCacheModel.h \
        RideEditor.h \
        RideFileCache.h \
        RideFileCommand.h \
        RideFile.h \
        RideFileTableModel.h \
        RideImportWizard.h \
        RideItem.h \
        RideMetadata.h \
        RideMetric.h \
        RideNavigator.h \
        RideNavigatorProxy.h \
        RideSummaryWindow.h \
        RideWindow.h \
        Route.h \
        RouteParser.h \
        SaveDialogs.h \
        ScatterPlot.h \
        ScatterWindow.h \
        SearchBox.h \
        SearchFilterBox.h \
        Season.h \
        SeasonParser.h \
        Secrets.h \
        Serial.h \
        Settings.h \
        ShareDialog.h \
        SlfParser.h \
        SlfRideFile.h \
        SmallPlot.h \
        SmfParser.h \
        SmfRideFile.h \
        SmlParser.h \
        SmlRideFile.h \
        SpecialFields.h \
        Specification.h \
        SpinScanPlot.h \
        SpinScanPlotWindow.h \
        SpinScanPolarPlot.h \
        SplitActivityWizard.h \
        SportPlusHealthUploader.h \
        SrdRideFile.h \
        SrmRideFile.h \
        Statistic.h \
        SummaryWindow.h \
        SyncRideFile.h \
        Tab.h \
        TabView.h \
        TcxParser.h \
        TcxRideFile.h \
        TimeUtils.h \
        ToolsDialog.h \
        ToolsRhoEstimator.h \
        TPDownloadDialog.h \
        TPDownload.h \
        TPUploadDialog.h \
        TPUpload.h \
        TrainDB.h \
        TrainingstagebuchUploader.h \
        TrainSidebar.h \
        TreeMapPlot.h \
        TreeMapWindow.h \
        TxtRideFile.h \
        Units.h \
        UserData.h \
        UserMetricParser.h \
        UserMetricSettings.h \
        VDOTCalculator.h \
        VeloHeroUploader.h \
        VideoLayoutParser.h \
        VideoSyncFile.h \
        Views.h \
        WithingsDownload.h \
        WkoRideFile.h \
        WorkoutPlotWindow.h \
        WorkoutWidget.h \
        WorkoutWidgetItems.h \
        WorkoutWindow.h \
        WorkoutWizard.h \
        WPrime.h \
        ZoneScaleDraw.h \
        Zones.h \
        ZwoParser.h


###=============
### SOURCE FILES
###=============

SOURCES += \
        aBikeScore.cpp \
        AboutDialog.cpp \
        aCoggan.cpp \
        AddDeviceWizard.cpp \
        AddIntervalDialog.cpp \
        AerobicDecoupling.cpp \
        Aerolab.cpp \
        AerolabWindow.cpp \
        AllPlot.cpp \
        AllPlotInterval.cpp \
        AllPlotSlopeCurve.cpp \
        AllPlotWindow.cpp \
        AnalysisSidebar.cpp \
        ANTChannel.cpp \
        ANT.cpp \
        ANTlocalController.cpp \
        ANTLogger.cpp \
        ANTMessage.cpp \
        AthleteBackup.cpp \
        Athlete.cpp \
        BasicRideMetrics.cpp \
        BatchExportDialog.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        Bin2RideFile.cpp \
        BingMap.cpp \
        BinRideFile.cpp \
        BlankState.cpp \
        CalendarDownload.cpp \
        ChartBar.cpp \
        ChartSettings.cpp \
        ChooseCyclistDialog.cpp \
        ../qtsolutions/codeeditor/codeeditor.cpp \
        Coggan.cpp \
        ColorButton.cpp \
        Colors.cpp \
        CommPort.cpp \
        CompareDateRange.cpp \
        CompareInterval.cpp \
        ComparePane.cpp \
        Computrainer3dpFile.cpp \
        ComputrainerController.cpp \
        Computrainer.cpp \
        ConfigDialog.cpp \
        Context.cpp \
        CPPlot.cpp \
        CpPlotCurve.cpp \
        CriticalPowerWindow.cpp \
        CsvRideFile.cpp \
        DanielsPoints.cpp \
        DataFilter.cpp \
        DataProcessor.cpp \
        DeviceConfiguration.cpp \
        Device.cpp \
        DeviceTypes.cpp \
        DialWindow.cpp \
        DiarySidebar.cpp \
        DownloadRideDialog.cpp \
        DragBar.cpp \
        EditUserMetricDialog.cpp \
        ErgDB.cpp \
        ErgDBDownloadDialog.cpp \
        ErgFile.cpp \
        ErgFilePlot.cpp \
        ExtendedCriticalPower.cpp \
        FileStore.cpp \
        FitlogParser.cpp \
        FitlogRideFile.cpp \
        FitRideFile.cpp \
        FixDeriveDistance.cpp \
        FixDerivePower.cpp \
        FixDeriveTorque.cpp \
        FixElevation.cpp \
        FixFreewheeling.cpp \
        FixGaps.cpp \
        FixGPS.cpp \
        FixHRSpikes.cpp \
        FixMoxy.cpp \
        FixPower.cpp \
        FixSmO2.cpp \
        FixSpeed.cpp \
        FixSpikes.cpp \
        FixTorque.cpp \
        FreeSearch.cpp \
        GcCrashDialog.cpp \
        GcOverlayWidget.cpp \
        GcPane.cpp \
        GcRideFile.cpp \
        GcScopeBar.cpp \
        GcSideBarItem.cpp \
        GcToolBar.cpp \
        GcUpgrade.cpp \
        GcWindowLayout.cpp \
        GcWindowRegistry.cpp \
        GenerateHeatMapDialog.cpp \
        GoldenCheetah.cpp \
        GoogleMapControl.cpp \
        GOVSS.cpp \
        GProgressDialog.cpp \
        GpxParser.cpp \
        GpxRideFile.cpp \
        HelpWhatsThis.cpp \
        HelpWindow.cpp \
        HistogramWindow.cpp \
        HomeWindow.cpp \
        HrPwPlot.cpp \
        HrPwWindow.cpp \
        HrTimeInZone.cpp \
        HrZones.cpp \
        IndendPlotMarker.cpp \
        IntervalItem.cpp \
        IntervalSummaryWindow.cpp \
        IntervalTreeView.cpp \
        JouleDevice.cpp \
        LapsEditor.cpp \
        LeftRightBalance.cpp \
        Library.cpp \
        LibraryParser.cpp \
        LocalFileStore.cpp \
        LogTimeScaleDraw.cpp \
        LTMCanvasPicker.cpp \
        LTMChartParser.cpp \
        LTMOutliers.cpp \
        LTMPlot.cpp \
        LTMPopup.cpp \
        LTMSettings.cpp \
        LTMSidebar.cpp \
        LTMTool.cpp \
        LTMTrend.cpp \
        LTMWindow.cpp \
        MacroDevice.cpp \
        main.cpp \
        MainWindow.cpp \
        ManualRideDialog.cpp \
        ManualRideFile.cpp \
        MergeActivityWizard.cpp \
        MetadataWindow.cpp \
        MeterWidget.cpp \
        MoxyDevice.cpp \
        MUPlot.cpp \
        MUWidget.cpp \
        NamedSearch.cpp \
        NewCyclistDialog.cpp \
        NullController.cpp \
        OAuthDialog.cpp \
        PaceTimeInZone.cpp \
        PaceZones.cpp \
        Pages.cpp \
        PDModel.cpp \
        PeakPace.cpp \
        PeakPower.cpp \
        PfPvPlot.cpp \
        PfPvWindow.cpp \
        PMCData.cpp \
        PolarRideFile.cpp \
        PowerHist.cpp \
        PowerTapDevice.cpp \
        PowerTapUtil.cpp \
        PwxRideFile.cpp \
        ../qtsolutions/json/mvjson.cpp \
        ../qtsolutions/qwtcurve/qwt_plot_gapped_curve.cpp \
        QuarqParser.cpp \
        QuarqRideFile.cpp \
        ../qxt/src/qxtspanslider.cpp \
        ../qxt/src/qxtstringspinbox.cpp \
        ../qzip/zip.cpp \
        RawRideFile.cpp \
        RealtimeController.cpp \
        RealtimeData.cpp \
        RealtimePlot.cpp \
        RealtimePlotWindow.cpp \
        ReferenceLineDialog.cpp \
        RemoteControl.cpp \
        RideAutoImportConfig.cpp \
        RideCache.cpp \
        RideCacheModel.cpp \
        RideEditor.cpp \
        RideFileCache.cpp \
        RideFileCommand.cpp \
        RideFile.cpp \
        RideFileTableModel.cpp \
        RideImportWizard.cpp \
        RideItem.cpp \
        RideMetadata.cpp \
        RideMetric.cpp \
        RideNavigator.cpp \
        RideSummaryWindow.cpp \
        RideWindow.cpp \
        Route.cpp \
        RouteParser.cpp \
        SaveDialogs.cpp \
        ScatterPlot.cpp \
        ScatterWindow.cpp \
        SearchBox.cpp \
        SearchFilterBox.cpp \
        Season.cpp \
        SeasonParser.cpp \
        Serial.cpp \
        Settings.cpp \
        ShareDialog.cpp \
        SlfParser.cpp \
        SlfRideFile.cpp \
        SmallPlot.cpp \
        SmfParser.cpp \
        SmfRideFile.cpp \
        SmlParser.cpp \
        SmlRideFile.cpp \
        SpecialFields.cpp \
        Specification.cpp \
        SpinScanPlot.cpp \
        SpinScanPlotWindow.cpp \
        SpinScanPolarPlot.cpp \
        SplitActivityWizard.cpp \
        SportPlusHealthUploader.cpp \
        SrdRideFile.cpp \
        SrmRideFile.cpp \
        Statistic.cpp \
        SummaryWindow.cpp \
        SustainMetric.cpp \
        SwimScore.cpp \
        SyncRideFile.cpp \
        Tab.cpp \
        TabView.cpp \
        TacxCafRideFile.cpp \
        TcxParser.cpp \
        TcxRideFile.cpp \
        TimeInZone.cpp \
        TimeUtils.cpp \
        ToolsDialog.cpp \
        ToolsRhoEstimator.cpp \
        TPDownload.cpp \
        TPDownloadDialog.cpp \
        TPUpload.cpp \
        TPUploadDialog.cpp \
        TrainDB.cpp \
        TrainingstagebuchUploader.cpp \
        TrainSidebar.cpp \
        TreeMapPlot.cpp \
        TreeMapWindow.cpp \
        TRIMPPoints.cpp \
        TxtRideFile.cpp \
        Units.cpp \
        UserData.cpp \
        UserMetric.cpp \
        UserMetricParser.cpp \
        VDOTCalculator.cpp \
        VDOT.cpp \
        VeloHeroUploader.cpp \
        VideoLayoutParser.cpp \
        VideoSyncFile.cpp \
        Views.cpp \
        WattsPerKilogram.cpp \
        WithingsDownload.cpp \
        WkoRideFile.cpp \
        WorkoutWidget.cpp \
        WorkoutWidgetItems.cpp \
        WorkoutPlotWindow.cpp \
        WorkoutWindow.cpp \
        WorkoutWizard.cpp \
        WPrime.cpp \
        Zones.cpp \
        ZwoParser.cpp


###======================================
### PENDING SOURCE FILES [not active yet]
###======================================

DEFERRES += RouteWindow.h \
            RouteWindow.cpp \
            RouteItem.h \
            RouteItem.cpp

###====================
### MISCELLANEOUS FILES
###====================

OTHER_FILES += \
    web/Rider.js \
    web/ride.js \
    web/jquery-1.6.4.min.js \
    web/MapWindow.html \
    web/StreetViewWindow.html \
    web/Window.css
