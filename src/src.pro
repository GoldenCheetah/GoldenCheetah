# To build, see the instructions in gcconfig.pri.in.

include( gcconfig.pri )

#
# What we are making and core dependencies
#
TEMPLATE = app
TARGET = GoldenCheetah
!isEmpty( APP_NAME ) { TARGET = $${APP_NAME} }
DEPENDPATH += .
QMAKE_INFO_PLIST = ./mac/Info.plist.app

## qwt and libz
INCLUDEPATH += ../qwt/src ../qxt/src $${LIBZ_INCLUDE} ../qtsolutions/json
LIBS += ../qwt/lib/libqwt.a
LIBS += -lm $${LIBZ_LIBS}

#
# We support 4.8.4 or higher
#            5.2.0 or higher
#
## common modules
QT += xml sql network script svg

lessThan(QT_MAJOR_VERSION, 5) {

    ## QT4 specific modules
    QT += webkit

} else {

    ## QT5 specific modules
    QT += webkitwidgets widgets concurrent
    macx {
        QT += macextras
    } else {
        QT += multimedia multimediawidgets
    }
}

# if we are building in debug mode
# then set MACRO -DGC_DEBUG so we can
# add / turnoff code for debugging purposes
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -DGC_DEBUG
}

!isEmpty( LIBOAUTH_INSTALL ) {
    isEmpty( LIBOAUTH_INCLUDE ) { LIBOAUTH_INCLUDE += $${LIBOAUTH_INSTALL}/include }
    isEmpty( LIBOAUTH_LIBS ) {
        LIBOAUTH_LIBS = $${LIBOAUTH_INSTALL}/lib/liboauth.a \
                        -lcurl -lcrypto -lz
    }
    INCLUDEPATH += $${LIBOAUTH_INCLUDE}
    LIBS        += $${LIBOAUTH_LIBS}
    DEFINES     += GC_HAVE_LIBOAUTH
    SOURCES     += OAuthDialog.cpp TwitterDialog.cpp
    HEADERS     += OAuthDialog.h TwitterDialog.h
}

!isEmpty( D2XX_INCLUDE ) {
    INCLUDEPATH += $${D2XX_INCLUDE}
    !isEmpty( D2XX_LIBS ) { LIBS += $${D2XX_LIBS} }
    HEADERS     += D2XX.h
    SOURCES     += D2XX.cpp
    DEFINES     += GC_HAVE_D2XX
}

!isEmpty( SRMIO_INSTALL ) {
    isEmpty( SRMIO_INCLUDE ) { SRMIO_INCLUDE = $${SRMIO_INSTALL}/include }
    isEmpty( SRMIO_LIBS )    { SRMIO_LIBS    = $${SRMIO_INSTALL}/lib/libsrmio.a }
    INCLUDEPATH += $${SRMIO_INCLUDE}
    LIBS        += $${SRMIO_LIBS}
    HEADERS     += SrmDevice.h
    SOURCES     += SrmDevice.cpp
    DEFINES     += GC_HAVE_SRMIO
}

!isEmpty( QWT3D_INSTALL ) {
    isEmpty( QWT3D_INCLUDE ) { QWT3D_INCLUDE = $${QWT3D_INSTALL}/include }
    isEmpty( QWT3D_LIBS )    { QWT3D_LIBS    = $${QWT3D_INSTALL}/lib/libqwtplot3d.a }
    INCLUDEPATH += $${QWT3D_INCLUDE}
    LIBS        += $${QWT3D_LIBS}
    unix:!macx { LIBS += -lGLU }
    QT          += opengl
    DEFINES     += GC_HAVE_QWTPLOT3D
    HEADERS     += ModelPlot.h ModelWindow.h
    SOURCES     += ModelPlot.cpp ModelWindow.cpp
}

!isEmpty( KML_INSTALL) {
    isEmpty( KML_INCLUDE ) { KML_INCLUDE = $${KML_INSTALL}/include }
    isEmpty( KML_LIBS )    { 
        KML_LIBS    = $${KML_INSTALL}/lib/libkmldom.a \
                      $${KML_INSTALL}/lib/libkmlconvenience.a \
                      $${KML_INSTALL}/lib/libkmlengine.a \
                      $${KML_INSTALL}/lib/libkmlbase.a
    }
    INCLUDEPATH += $${KML_INCLUDE} $${BOOST_INCLUDE}
    LIBS        += $${KML_LIBS}
    DEFINES     += GC_HAVE_KML
    SOURCES     += KmlRideFile.cpp
    HEADERS     += KmlRideFile.h
}

!isEmpty( ICAL_INSTALL ) {
    isEmpty( ICAL_INCLUDE ) { ICAL_INCLUDE = $${ICAL_INSTALL}/include }
    isEmpty( ICAL_LIBS )    { ICAL_LIBS    = $${ICAL_INSTALL}/lib/libical.a }
    INCLUDEPATH += $${ICAL_INCLUDE}
    LIBS        += $${ICAL_LIBS}
    DEFINES     += GC_HAVE_ICAL
    HEADERS     += ICalendar.h DiaryWindow.h CalDAV.h
    SOURCES     += ICalendar.cpp DiaryWindow.cpp CalDAV.cpp
}

# are we supporting USB2 devices
!isEmpty( LIBUSB_INSTALL ) {
    isEmpty( LIBUSB_INCLUDE ) { LIBUSB_INCLUDE = $${LIBUSB_INSTALL}/include }
    isEmpty( LIBUSB_LIBS )    {
        unix  { LIBUSB_LIBS = $${LIBUSB_INSTALL}/lib/libusb.a }
        win32 { LIBUSB_LIBS = $${LIBUSB_INSTALL}/lib/gcc/libusb.a }
    }
    INCLUDEPATH += $${LIBUSB_INCLUDE}
    LIBS        += $${LIBUSB_LIBS}
    DEFINES     += GC_HAVE_LIBUSB
    SOURCES     += LibUsb.cpp EzUsb.c Fortius.cpp FortiusController.cpp
    HEADERS     += LibUsb.h EzUsb.h Fortius.cpp FortiusController.h
}

# are we supporting video playback?
# only on Linux and Windows, since we use QTKit on Mac
!isEmpty( VLC_INSTALL ) {
    macx {
        # we do not use VLC on Mac we use Quicktime
        # so ignore this setting on a Mac build
    } else {
        isEmpty( VLC_INCLUDE ) { VLC_INCLUDE = $${VLC_INSTALL}/include }
        isEmpty( VLC_LIBS )    {
            win32 {
                VLC_LIBS = $${VLC_INSTALL}/lib/libvlc.dll.a \
    	                   $${VLC_INSTALL}/lib/libvlccore.dll.a
            } else {
                VLC_LIBS += -lvlc -lvlccore
            }
        }
        INCLUDEPATH += $${VLC_INCLUDE}
        LIBS        += $${VLC_LIBS}
        DEFINES     += GC_HAVE_VLC
    }
}

!isEmpty( CLUCENE_LIBS ) {
    INCLUDEPATH += $${CLUCENE_INCLUDE}
    LIBS        += $${CLUCENE_LIBS}
    DEFINES     += GC_HAVE_LUCENE
    HEADERS     += Lucene.h DataFilter.h SearchBox.h NamedSearch.h SearchFilterBox.h
    SOURCES     += Lucene.cpp DataFilter.cpp SearchBox.cpp NamedSearch.cpp SearchFilterBox.cpp
    YACCSOURCES += DataFilter.y
    LEXSOURCES  += DataFilter.l
}

# Mac specific build for
# Segmented mac style button
# Video playback using Quicktime Framework
# search box for title bar
macx {
    LIBS    += -lobjc -framework IOKit -framework AppKit -framework QTKit
    HEADERS +=  QtMacVideoWindow.h \
                QtMacSegmentedButton.h \
                QtMacButton.h 

    OBJECTIVE_SOURCES +=    QtMacVideoWindow.mm \
                            QtMacSegmentedButton.mm \
                            QtMacButton.mm 

    # on a mac we need to install the Wahoo API for BTLE/Kickr support
    # This requires **v3.0 (beta)** of the WF API which is not yet
    # in general release 
    !isEmpty(HAVE_WFAPI) {

        DEFINES += GC_HAVE_WFAPI
        LIBS += -framework WFConnector 
        LIBS += -framework IOBluetooth -framework Foundation
        LIBS += -lstdc++ -all_load

        # We have an abstraction layer for the Wahoo Fitness API
        # At present this only works on Mac -- since support for 
        # BTLE on Linux and Windows is emerging and there is no
        # Linux or Windows support for the WF API from Wahoo (yet)
        HEADERS += WFApi.h Kickr.h KickrController.h
        SOURCES += Kickr.cpp KickrController.cpp
        HEADERS += BT40.h BT40Controller.h
        SOURCES += BT40.cpp BT40Controller.cpp
        OBJECTIVE_SOURCES += WFApi.mm

    }

} else {

    # not a mac? then F12 to toggle full screen using
    # standard QT showFullScreen / showNormal
    HEADERS += QTFullScreen.h
    SOURCES += QTFullScreen.cpp

    #qt segmented control for toolbar (non-Mac)
    HEADERS += ../qtsolutions/segmentcontrol/qtsegmentcontrol.h
    SOURCES += ../qtsolutions/segmentcontrol/qtsegmentcontrol.cpp

    # we now have videowindow, it will do nothing
    HEADERS     += VideoWindow.h
    SOURCES     += VideoWindow.cpp
}

!win32 {
    RC_FILE = images/gc.icns
}

win32 {

    INCLUDEPATH += ./win32 $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib
    LIBS += -lws2_32
    //QMAKE_LFLAGS = -Wl,--enable-runtime-pseudo-reloc \
    //               -Wl,--script,win32/i386pe.x-no-rdata,--enable-auto-import
    //QMAKE_CXXFLAGS += -fdata-sections
    RC_FILE = windowsico.rc

    # are we supporting USB1 devices on Windows?
    !isEmpty( USBXPRESS_INSTALL ) {
        isEmpty( USBXPRESS_INCLUDE ) { USBXPRESS_INCLUDE = $${USBXPRESS_INSTALL} }
        isEmpty( USBXPRESS_LIBS )    { USBXPRESS_LIBS    = $${USBXPRESS_INSTALL}/x86/SiUSBXp.lib }
        INCLUDEPATH += $${USBXPRESS_INCLUDE}
        LIBS        += $${USBXPRESS_LIBS}
        DEFINES     += GC_HAVE_USBXPRESS
        SOURCES += USBXpress.cpp
        HEADERS += USBXpress.h
    }
}

# local qxt widgets - rather than add another dependency on libqxt
DEFINES += QXT_STATIC
SOURCES += ../qxt/src/qxtspanslider.cpp \
           ../qxt/src/qxtstringspinbox.cpp

HEADERS += ../qxt/src/qxtspanslider.h \
           ../qxt/src/qxtspanslider_p.h \
           ../qxt/src/qxtstringspinbox.h

isEmpty( QTSOAP_INSTALL ) {
    include( ../qtsolutions/soap/qtsoap.pri )
} else {
    include( $${QTSOAP_INSTALL} )
}
HEADERS += TPUpload.h TPUploadDialog.h TPDownload.h TPDownloadDialog.h
SOURCES += TPUpload.cpp TPUploadDialog.cpp TPDownload.cpp TPDownloadDialog.cpp
DEFINES += GC_HAVE_SOAP

HEADERS += \
        AboutDialog.h \
        AddDeviceWizard.h \
        AddIntervalDialog.h \
        Aerolab.h \
        AerolabWindow.h \
        AllPlot.h \
        AllPlotWindow.h \
        AllPlotSlopeCurve.h \
        AnalysisSidebar.h \
        ANT.h \
        ANTChannel.h \
        ANTLogger.h \
        ANTMessage.h \
        ANTMessages.h \
        ANTlocalController.h \
        Athlete.h \
        BatchExportDialog.h \
        BestIntervalDialog.h \
        BinRideFile.h \
        Bin2RideFile.h \
        BingMap.h \
        BlankState.h \
        CalendarDownload.h \
        ChartBar.h \
        ChartSettings.h \
        ChooseCyclistDialog.h \
        Colors.h \
        ColorButton.h \
        CommPort.h \
        CompareDateRange.h \
        CompareInterval.h \
        ComparePane.h \
        Computrainer.h \
        Computrainer3dpFile.h \
        ConfigDialog.h \
        Context.h \
        CpPlotCurve.h \
        CPPlot.h \
        CriticalPowerWindow.h \
        CsvRideFile.h \
        DataProcessor.h \
        DBAccess.h \
        DaysScaleDraw.h \
        Device.h \
        DeviceTypes.h \
        DeviceConfiguration.h \
        DialWindow.h \
        DiarySidebar.h \
        DragBar.h \
        DownloadRideDialog.h \
        ErgFile.h \
        ErgDB.h \
        ErgDBDownloadDialog.h \
        ErgFilePlot.h \
        ExtendedCriticalPower.h \
        FitlogRideFile.h \
        FitlogParser.h \
        FitRideFile.h \ 
        GenerateHeatMapDialog.h \
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
        GoldenCheetah.h \
        GoogleMapControl.h \
        GProgressDialog.h \
        GpxParser.h \
        GpxRideFile.h \
        HelpWindow.h \
        HistogramWindow.h \
        HomeWindow.h \
        HrZones.h \
        HrPwPlot.h \
        HrPwWindow.h \
        IntervalItem.h \
        IntervalSidebar.h \
        IntervalSummaryWindow.h \
        IntervalTreeView.h \
        JouleDevice.h \
        JsonRideFile.h \
        Library.h \
        LibraryParser.h \
        LogTimeScaleDraw.h \
        LTMCanvasPicker.h \
        LTMChartParser.h \
        LTMOutliers.h \
        LTMPlot.h \
        LTMPopup.h \
        LTMSidebar.h \
        LTMSettings.h \
        LTMTool.h \
        LTMTrend.h \
        LTMTrend2.h \
        LTMWindow.h \
        MacroDevice.h \
        MainWindow.h \
        ManualRideDialog.h \
        ManualRideFile.h \
        MergeActivityWizard.h \
        MetadataWindow.h \
        MetricAggregator.h \
        MoxyDevice.h \
        MUPlot.h \
        MUPool.h \
        MUWidget.h \
        NewCyclistDialog.h \
        NullController.h \
        PaceZones.h \
        Pages.h \
        PDModel.h \
        PfPvPlot.h \
        PfPvWindow.h \
        PolarRideFile.h \
        PowerHist.h \
        PowerTapDevice.h \
        PowerTapUtil.h \
        PwxRideFile.h \
        QuarqParser.h \
        QuarqRideFile.h \
        RawRideFile.h \
        RealtimeData.h \
        RealtimePlotWindow.h \
        RealtimeController.h \
        ReferenceLineDialog.h \
        ComputrainerController.h \
        RealtimePlot.h \
        RideAutoImportConfig.h \
        RideCache.h \
        RideEditor.h \
        RideFile.h \
        RideFileCache.h \
        RideFileCommand.h \
        RideFileTableModel.h \
        RideImportWizard.h \
        RideItem.h \
        RideMetadata.h \
        RideMetric.h \
        RideNavigator.h \
        RideNavigatorProxy.h \
        RideWindow.h \
        IntervalNavigator.h \
        IntervalNavigatorProxy.h \
        SaveDialogs.h \
        SmallPlot.h \
        RideSummaryWindow.h \
        Route.h \
        RouteItem.h \
        RouteParser.h \
        RouteWindow.h \
        ScatterPlot.h \
        ScatterWindow.h \
        Season.h \
        SeasonParser.h \
        Serial.h \
        Settings.h \
        ShareDialog.h \
        SpecialFields.h \
        SpinScanPlot.h \
        SpinScanPolarPlot.h \
        SpinScanPlotWindow.h \
        SplitActivityWizard.h \
        SlfParser.h \
        SlfRideFile.h \
        SmfParser.h \
        SmfRideFile.h \
        SrdRideFile.h \
        SrmRideFile.h \
        StressCalculator.h \
        SummaryMetrics.h \
        SummaryWindow.h \
        SyncRideFile.h \
        Tab.h \
        TabView.h \
        TcxParser.h \
        TcxRideFile.h \
        TxtRideFile.h \
        TimeUtils.h \
        ToolsDialog.h \
        ToolsRhoEstimator.h \
        TrainDB.h \
        TrainSidebar.h \
        TreeMapWindow.h \
        TreeMapPlot.h \
        TrainingstagebuchUploader.h \
        Units.h \
        VeloHeroUploader.h \
        Views.h \
        WithingsDownload.h \
        WkoRideFile.h \
        WorkoutPlotWindow.h \
        WorkoutWizard.h \
        WPrime.h \
        ZeoDownload.h \
        Zones.h \
        ZoneScaleDraw.h \
        ../qtsolutions/json/mvjson.h

YACCSOURCES += JsonRideFile.y WithingsParser.y
LEXSOURCES  += JsonRideFile.l WithingsParser.l

#-t turns on debug, use with caution
#QMAKE_YACCFLAGS = -t -d

SOURCES += \
        AboutDialog.cpp \
        AddDeviceWizard.cpp \
        AddIntervalDialog.cpp \
        AerobicDecoupling.cpp \
        Aerolab.cpp \
        AerolabWindow.cpp \
        AllPlot.cpp \
        AllPlotWindow.cpp \
        AllPlotSlopeCurve.cpp \
        AnalysisSidebar.cpp \
        ANT.cpp \
        ANTChannel.cpp \
        ANTLogger.cpp \
        ANTMessage.cpp \
        ANTlocalController.cpp \
        Athlete.cpp \
        BasicRideMetrics.cpp \
        BatchExportDialog.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        BinRideFile.cpp \
        Bin2RideFile.cpp \
        BingMap.cpp \
        BlankState.cpp \
        CalendarDownload.cpp \
        ChartBar.cpp \
        ChartSettings.cpp \
        ChooseCyclistDialog.cpp \
        Coggan.cpp \
        Colors.cpp \
        ColorButton.cpp \
        CommPort.cpp \
        CompareDateRange.cpp \
        CompareInterval.cpp \
        ComparePane.cpp \
        Computrainer.cpp \
        Computrainer3dpFile.cpp \
        ConfigDialog.cpp \
        Context.cpp \
        CpPlotCurve.cpp \
        CPPlot.cpp \
        CriticalPowerWindow.cpp \
        CsvRideFile.cpp \
        DanielsPoints.cpp \
        DataProcessor.cpp \
        DBAccess.cpp \
        Device.cpp \
        DeviceTypes.cpp \
        DeviceConfiguration.cpp \
        DialWindow.cpp \
        DiarySidebar.cpp \
        DownloadRideDialog.cpp \
        DragBar.cpp \
        ErgDB.cpp \
        ErgDBDownloadDialog.cpp \
        ErgFile.cpp \
        ErgFilePlot.cpp \
        ExtendedCriticalPower.cpp \
        FitlogRideFile.cpp \
        FitlogParser.cpp \
        FitRideFile.cpp \
        FixDerivePower.cpp \
        FixDeriveTorque.cpp \
        FixElevation.cpp \
        FixGaps.cpp \
        FixGPS.cpp \
        FixPower.cpp \
        FixSpikes.cpp \
        FixTorque.cpp \
        FixHRSpikes.cpp \
        GenerateHeatMapDialog.cpp \
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
        GoldenCheetah.cpp \
        GoogleMapControl.cpp \
        GOVSS.cpp \
        GProgressDialog.cpp \
        GpxParser.cpp \
        GpxRideFile.cpp \
        HelpWindow.cpp \
        HistogramWindow.cpp \
        HomeWindow.cpp \
        HrTimeInZone.cpp \
        HrZones.cpp \
        HrPwPlot.cpp \
        HrPwWindow.cpp \
        IntervalItem.cpp \
        IntervalSidebar.cpp \
        IntervalSummaryWindow.cpp \
        IntervalTreeView.cpp \
        JouleDevice.cpp \
        LeftRightBalance.cpp \
        Library.cpp \
        LibraryParser.cpp \
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
        MainWindow.cpp \
        ManualRideDialog.cpp \
        ManualRideFile.cpp \
        MergeActivityWizard.cpp \
        MetadataWindow.cpp \
        MetricAggregator.cpp \
        MoxyDevice.cpp \
        MUPlot.cpp \
        MUWidget.cpp \
        NewCyclistDialog.cpp \
        NullController.cpp \
        PaceTimeInZone.cpp \
        PaceZones.cpp \
        Pages.cpp \
        PDModel.cpp \
        PeakPower.cpp \
        PfPvPlot.cpp \
        PfPvWindow.cpp \
        PolarRideFile.cpp \
        PowerHist.cpp \
        PowerTapDevice.cpp \
        PowerTapUtil.cpp \
        PwxRideFile.cpp \
        QuarqParser.cpp \
        QuarqRideFile.cpp \
        RawRideFile.cpp \
        RealtimeData.cpp \
        RealtimeController.cpp \
        ComputrainerController.cpp \
        RealtimePlot.cpp \
        RealtimePlotWindow.cpp \
        ReferenceLineDialog.cpp \
        RideAutoImportConfig.cpp \
        RideCache.cpp \
        RideEditor.cpp \
        RideFile.cpp \
        RideFileCache.cpp \
        RideFileCommand.cpp \
        RideFileTableModel.cpp \
        RideImportWizard.cpp \
        RideItem.cpp \
        RideMetadata.cpp \
        RideMetric.cpp \
        RideNavigator.cpp \
        RideSummaryWindow.cpp \
        RideWindow.cpp \
        IntervalNavigator.cpp \
        Route.cpp \
        RouteItem.cpp \
        RouteParser.cpp \
        RouteWindow.cpp \
        SaveDialogs.cpp \
        ScatterPlot.cpp \
        ScatterWindow.cpp \
        Season.cpp \
        SeasonParser.cpp \
        Serial.cpp \
        Settings.cpp \
        ShareDialog.cpp \
        SmallPlot.cpp \
        SpecialFields.cpp \
        SpinScanPlot.cpp \
        SpinScanPolarPlot.cpp \
        SpinScanPlotWindow.cpp \
        SplitActivityWizard.cpp \
        SlfParser.cpp \
        SlfRideFile.cpp \
        SmfParser.cpp \
        SmfRideFile.cpp \
        SrdRideFile.cpp \
        SrmRideFile.cpp \
        StressCalculator.cpp \
        SummaryMetrics.cpp \
        SummaryWindow.cpp \
        SyncRideFile.cpp \
        Tab.cpp \
        TabView.cpp \
        TacxCafRideFile.cpp \
        TcxParser.cpp \
        TcxRideFile.cpp \
        TxtRideFile.cpp \
        TimeInZone.cpp \
        TimeUtils.cpp \
        ToolsDialog.cpp \
        ToolsRhoEstimator.cpp \
        TrainDB.cpp \
        TrainSidebar.cpp \
        TreeMapWindow.cpp \
        TreeMapPlot.cpp \
        TrainingstagebuchUploader.cpp \
        TRIMPPoints.cpp \
        Units.cpp \
        VeloHeroUploader.cpp \
        Views.cpp \
        WattsPerKilogram.cpp \
        WithingsDownload.cpp \
        WkoRideFile.cpp \
        WorkoutPlotWindow.cpp \
        WorkoutWizard.cpp \
        WPrime.cpp \
        ZeoDownload.cpp \
        Zones.cpp \
        main.cpp \
        ../qtsolutions/json/mvjson.cpp

RESOURCES = application.qrc \
            RideWindow.qrc

TRANSLATIONS = translations/gc_fr.ts \
               translations/gc_ja.ts \
               translations/gc_it.ts \
               translations/gc_pt-br.ts \
               translations/gc_de.ts \
               translations/gc_cs.ts \
               translations/gc_es.ts \
               translations/gc_pt.ts \
               translations/gc_ru.ts

!isEmpty(TRANSLATIONS) {

   isEmpty(QMAKE_LRELEASE) {
     win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
     unix:!macx {QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease-qt4 }
     else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
   }

   isEmpty(TS_DIR):TS_DIR = translations
   TSQM.name = lrelease ${QMAKE_FILE_IN}
   TSQM.input = TRANSLATIONS
   TSQM.output = $$TS_DIR/${QMAKE_FILE_BASE}.qm
   TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN}
   TSQM.CONFIG = no_link
   QMAKE_EXTRA_COMPILERS += TSQM
   PRE_TARGETDEPS += compiler_TSQM_make_all

} else:message(No translation files in project)

OTHER_FILES += \
    web/Rider.js \
    web/ride.js \
    web/jquery-1.6.4.min.js \
    web/MapWindow.html \
    web/StreetViewWindow.html \
    web/Window.css
