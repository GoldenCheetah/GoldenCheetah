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
INCLUDEPATH += ../qwt/src ../qxt/src $${LIBZ_INCLUDE} ../qtsolutions/json ../qtsolutions/qwtcurve
LIBS += ../qwt/lib/libqwt.a
LIBS += -lm $${LIBZ_LIBS}

#
# We support 4.8.4 or higher
#            5.2.0 or higher
#
## common modules
QT += xml sql network script svg concurrent

lessThan(QT_MAJOR_VERSION, 5) {

    ## QT4 specific modules
    QT += webkit

} else {

    ## QT5 specific modules
    QT += webkitwidgets widgets concurrent
    macx {
        QT += macextras webenginewidgets
    } else {
        QT += multimedia multimediawidgets
    }

    ## QT5 can support complex JSON documents
    SOURCES += Dropbox.cpp
    HEADERS += Dropbox.h
}

# if we are building in debug mode
# then set MACRO -DGC_DEBUG so we can
# add / turnoff code for debugging purposes
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -DGC_DEBUG
}


# KQOAuth .pro in default creates different libs for release and debug
!isEmpty( KQOAUTH_INSTALL ) {
    isEmpty( KQOAUTH_INCLUDE ) { KQOAUTH_INCLUDE += $${KQOAUTH_INSTALL}/src }
    isEmpty( KQOAUTH_LIBS ) {
        #KQOAUTH_LIBS = $${KQOAUTH_INSTALL}/lib/libkqoauth0.a
        KQOAUTH_LIBS = -lkqoauth
    }
    INCLUDEPATH += $${KQOAUTH_INCLUDE}
    LIBS        += $${KQOAUTH_LIBS}
    DEFINES     += GC_HAVE_KQOAUTH
    SOURCES     += TwitterDialog.cpp
    HEADERS     += TwitterDialog.h
}


!isEmpty( D2XX_INCLUDE ) {
    INCLUDEPATH += $${D2XX_INCLUDE}
    !isEmpty( D2XX_LIBS ) { LIBS += $${D2XX_LIBS} }
    HEADERS     += D2XX.h
    SOURCES     += D2XX.cpp
    DEFINES     += GC_HAVE_D2XX
    unix {
        LIBS    += -ldl
    }
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
    INCLUDEPATH += $${KML_INCLUDE}  $${BOOST_INCLUDE}
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

# are we supporting USB2 devices - LibUsb 0.1.12
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

# are we supporting USB2 devices - LibUsb 1.0.18+
!isEmpty( LIBUSB1_INSTALL ) {
    isEmpty( LIBUSB1_INCLUDE ) { LIBUSB1_INCLUDE = $${LIBUSB1_INSTALL}/include }
    isEmpty( LIBUSB1_LIBS )    {
        unix  { LIBUSB1_LIBS = $${LIBUSB1_INSTALL}/lib/libusb-1.0.a -ludev }
        win32 { LIBUSB1_LIBS = $${LIBUSB1_INSTALL}/lib/libusb-1.0.a -ludev }
    }
    INCLUDEPATH += $${LIBUSB1_INCLUDE}
    LIBS        += $${LIBUSB1_LIBS}
    DEFINES     += GC_HAVE_LIBUSB GC_HAVE_LIBUSB1
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

# libsamplerate available ?
!isEmpty( SAMPLERATE_INSTALL ) {
    isEmpty( SAMPLERATE_INCLUDE ) { SAMPLERATE_INCLUDE = $${SAMPLERATE_INSTALL}/include }
    isEmpty( SAMPLERATE_LIBS )    { SAMPLERATE_LIBS    = $${SAMPLERATE_INSTALL}/lib/libsamplerate.a }
    INCLUDEPATH += $${SAMPLERATE_INCLUDE}
    LIBS        += $${SAMPLERATE_LIBS}
    DEFINES     += GC_HAVE_SAMPLERATE
}

# FreeSearch replaces deprecated lucene
HEADERS     += DataFilter.h SearchBox.h NamedSearch.h SearchFilterBox.h FreeSearch.h
SOURCES     += DataFilter.cpp SearchBox.cpp NamedSearch.cpp SearchFilterBox.cpp FreeSearch.cpp
YACCSOURCES += DataFilter.y
LEXSOURCES  += DataFilter.l

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

# gapped curve
HEADERS += ../qtsolutions/qwtcurve/qwt_plot_gapped_curve.h
SOURCES +=  ../qtsolutions/qwtcurve/qwt_plot_gapped_curve.cpp

# web server to provide web-services for external integration with R
!isEmpty ( HTPATH ) {

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

# qzipreader/qzipwriter always
HEADERS += ../qzip/zipreader.h \
           ../qzip/zipwriter.h
SOURCES += ../qzip/zip.cpp

HEADERS += $${LOCALHEADERS}
SOURCE += $${LOCALSOURCE}
HEADERS += \
        AboutDialog.h \
        AddDeviceWizard.h \
        AddIntervalDialog.h \
        Aerolab.h \
        AerolabWindow.h \
        AllPlot.h \
        AllPlotInterval.h \
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
        AthleteBackup.h \
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
        FileStore.h \
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
        HelpWhatsThis.h \
        HistogramWindow.h \
        HomeWindow.h \
        HrZones.h \
        HrPwPlot.h \
        HrPwWindow.h \
        IndendPlotMarker.h \
        IntervalItem.h \
        IntervalSummaryWindow.h \
        IntervalTreeView.h \
        JouleDevice.h \
        JsonRideFile.h \
        LapsEditor.h \
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
        MeterWidget.h \
        MoxyDevice.h \
        MUPlot.h \
        MUPool.h \
        MUWidget.h \
        LocalFileStore.h \
        NewCyclistDialog.h \
        NullController.h \
        OAuthDialog.h \
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
        RideCacheModel.h \
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
        SaveDialogs.h \
        SmallPlot.h \
        RideSummaryWindow.h \
        Route.h \
        RouteParser.h \
        ScatterPlot.h \
        ScatterWindow.h \
        Season.h \
        SeasonParser.h \
        Secrets.h \
        Serial.h \
        Settings.h \
        ShareDialog.h \
        SpecialFields.h \
        Specification.h \
        SpinScanPlot.h \
        SpinScanPolarPlot.h \
        SpinScanPlotWindow.h \
        SplitActivityWizard.h \
        SlfParser.h \
        SlfRideFile.h \
        SmfParser.h \
        SmfRideFile.h \
        SmlParser.h \
        SmlRideFile.h \
        SrdRideFile.h \
        SrmRideFile.h \
        Statistic.h \
        PMCData.h \
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
        VDOTCalculator.h \
        TrainDB.h \
        TrainSidebar.h \
        TreeMapWindow.h \
        TreeMapPlot.h \
        TrainingstagebuchUploader.h \
        Units.h \
        UserData.h \
        VeloHeroUploader.h \
        VideoLayoutParser.h \
        VideoSyncFile.h \
        Views.h \
        WithingsDownload.h \
        WkoRideFile.h \
        WorkoutPlotWindow.h \
        WorkoutWizard.h \
        WPrime.h \
        Zones.h \
        ZoneScaleDraw.h \
        ../qtsolutions/json/mvjson.h

LEXSOURCES  += JsonRideFile.l WithingsParser.l RideDB.l
YACCSOURCES += JsonRideFile.y WithingsParser.y RideDB.y

#-t turns on debug, use with caution
#QMAKE_YACCFLAGS = -t -d

# code that is pending later releases and not compiled in currently
DEFERRES += RouteWindow.h \
            RouteWindow.cpp \
            RouteItem.h \
            RouteItem.cpp

SOURCES += \
        main.cpp \
        AboutDialog.cpp \
        AddDeviceWizard.cpp \
        AddIntervalDialog.cpp \
        AerobicDecoupling.cpp \
        Aerolab.cpp \
        AerolabWindow.cpp \
        AllPlot.cpp \
        AllPlotInterval.cpp \
        AllPlotWindow.cpp \
        AllPlotSlopeCurve.cpp \
        AnalysisSidebar.cpp \
        ANT.cpp \
        ANTChannel.cpp \
        ANTLogger.cpp \
        ANTMessage.cpp \
        ANTlocalController.cpp \
        Athlete.cpp \
        AthleteBackup.cpp \
        BasicRideMetrics.cpp \
        BatchExportDialog.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        aBikeScore.cpp \
        BinRideFile.cpp \
        Bin2RideFile.cpp \
        BingMap.cpp \
        BlankState.cpp \
        CalendarDownload.cpp \
        ChartBar.cpp \
        ChartSettings.cpp \
        ChooseCyclistDialog.cpp \
        Coggan.cpp \
        aCoggan.cpp \
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
        FileStore.cpp \
        FitlogRideFile.cpp \
        FitlogParser.cpp \
        FitRideFile.cpp \
        FixDeriveDistance.cpp \
        FixDerivePower.cpp \
        FixDeriveTorque.cpp \
        FixElevation.cpp \
        FixFreewheeling.cpp \
        FixGaps.cpp \
        FixGPS.cpp \
        FixMoxy.cpp \
        FixPower.cpp \
        FixSpeed.cpp \
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
        HelpWhatsThis.cpp \
        HistogramWindow.cpp \
        HomeWindow.cpp \
        HrTimeInZone.cpp \
        HrZones.cpp \
        HrPwPlot.cpp \
        HrPwWindow.cpp \
        IndendPlotMarker.cpp \
        IntervalItem.cpp \
        IntervalSummaryWindow.cpp \
        IntervalTreeView.cpp \
        JouleDevice.cpp \
        LapsEditor.cpp \
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
        MeterWidget.cpp \
        MoxyDevice.cpp \
        MUPlot.cpp \
        MUWidget.cpp \
        LocalFileStore.cpp \
        NewCyclistDialog.cpp \
        NullController.cpp \
        OAuthDialog.cpp \
        PaceTimeInZone.cpp \
        PaceZones.cpp \
        Pages.cpp \
        PDModel.cpp \
        PeakPower.cpp \
        PeakPace.cpp \
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
        RideCacheModel.cpp \
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
        Route.cpp \
        RouteParser.cpp \
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
        Specification.cpp \
        SpinScanPlot.cpp \
        SpinScanPolarPlot.cpp \
        SpinScanPlotWindow.cpp \
        SplitActivityWizard.cpp \
        SlfParser.cpp \
        SlfRideFile.cpp \
        SmfParser.cpp \
        SmfRideFile.cpp \
        SmlParser.cpp \
        SmlRideFile.cpp \
        SrdRideFile.cpp \
        SrmRideFile.cpp \
        Statistic.cpp \
        SustainMetric.cpp \
        PMCData.cpp \
        SummaryWindow.cpp \
        SyncRideFile.cpp \
        SwimScore.cpp \
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
        VDOT.cpp \
        VDOTCalculator.cpp \
        TrainDB.cpp \
        TrainSidebar.cpp \
        TreeMapWindow.cpp \
        TreeMapPlot.cpp \
        TrainingstagebuchUploader.cpp \
        TRIMPPoints.cpp \
        Units.cpp \
        UserData.cpp \
        VeloHeroUploader.cpp \
        VideoLayoutParser.cpp \
        VideoSyncFile.cpp \
        Views.cpp \
        WattsPerKilogram.cpp \
        WithingsDownload.cpp \
        WkoRideFile.cpp \
        WorkoutPlotWindow.cpp \
        WorkoutWizard.cpp \
        WPrime.cpp \
        Zones.cpp \
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
               translations/gc_ru.ts \
	       translations/gc_zh-tw.ts

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

