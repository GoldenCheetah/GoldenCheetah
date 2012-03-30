# To build, see the instructions in gcconfig.pri.in.

include( gcconfig.pri )

TEMPLATE = app
TARGET = GoldenCheetah
!isEmpty( APP_NAME ) { TARGET = $${APP_NAME} }
DEPENDPATH += .

!isEmpty( BOOST_INCLUDE ) { INCLUDEPATH += $${BOOST_INCLUDE} }

INCLUDEPATH += ../qwt/src ../qxt/src
QT += xml sql network webkit script
LIBS += ../qwt/lib/libqwt.a
LIBS += -lm

!isEmpty( LIBOAUTH_INSTALL ) {
    isEmpty( LIBOAUTH_INCLUDE ) { LIBOAUTH_INCLUDE += $${LIBOAUTH_INSTALL}/include }
    isEmpty( LIBOAUTH_LIBS ) {
        LIBOAUTH_LIBS = $${LIBOAUTH_INSTALL}/lib/liboauth.a \
                        -lcurl -lcrypto -lz
    }
    INCLUDEPATH += $${LIBOAUTH_INCLUDE}
    LIBS        += $${LIBOAUTH_LIBS}
    DEFINES     += GC_HAVE_LIBOAUTH
    SOURCES     += TwitterDialog.cpp
    HEADERS     += TwitterDialog.h
}

!isEmpty( D2XX_INCLUDE ) {
    INCLUDEPATH += $${D2XX_INCLUDE}
    !isEmpty( D2XX_LIBS ) { LIBS += $${D2XX_LIBS} }
    HEADERS     += D2XX.h
    SOURCES     += D2XX.cpp
}

!isEmpty( SRMIO_INSTALL ) {
    isEmpty( SRMIO_INCLUDE ) { SRMIO_INCLUDE = $${SRMIO_INSTALL}/include }
    isEmpty( SRMIO_LIBS )    { SRMIO_LIBS    = $${SRMIO_INSTALL}/lib/libsrmio.a }
    INCLUDEPATH += $${SRMIO_INCLUDE}
    LIBS        += $${SRMIO_LIBS}
    HEADERS     += SrmDevice.h
    SOURCES     += SrmDevice.cpp
}

!isEmpty( QWT3D_INSTALL ) {
    isEmpty( QWT3D_INCLUDE ) { QWT3D_INCLUDE = $${QWT3D_INSTALL}/include }
    isEmpty( QWT3D_LIBS )    { QWT3D_LIBS    = $${QWT3D_INSTALL}/lib/libqwtplot3d.a }
    INCLUDEPATH += $${QWT3D_INCLUDE}
    LIBS        += $${QWT3D_LIBS}
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
    INCLUDEPATH += $${KML_INCLUDE}
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
        HEADERS     += VideoWindow.h
        SOURCES     += VideoWindow.cpp
    }
}

# Mac specific build for
# Segmented mac style button (but not used at present)
# Video playback using Quicktime Framework
# Lion fullscreen playback
macx {
    LIBS    += -lobjc -framework Carbon -framework IOKit -framework AppKit -framework QTKit
    HEADERS += QtMacVideoWindow.h LionFullScreen.h
    OBJECTIVE_SOURCES += QtMacVideoWindow.mm LionFullScreen.mm
} else {
    # not a mac? then F12 to toggle full screen using
    # standard QT showFullScreen / showNormal
    HEADERS += QTFullScreen.h
    SOURCES += QTFullScreen.cpp
}

!win32 {
    RC_FILE = images/gc.icns
}

win32 {
    INCLUDEPATH += ./win32 $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib
    LIBS += -lws2_32
    QMAKE_LFLAGS = -Wl,--enable-runtime-pseudo-reloc \
                   -Wl,--script,win32/i386pe.x-no-rdata,--enable-auto-import
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
           ../qxt/src/qxtscheduleview.cpp \
           ../qxt/src/qxtscheduleview_p.cpp \
           ../qxt/src/qxtscheduleheaderwidget.cpp \
           ../qxt/src/qxtscheduleviewheadermodel_p.cpp \
           ../qxt/src/qxtscheduleitemdelegate.cpp \
           ../qxt/src/qxtstyleoptionscheduleviewitem.cpp

isEmpty( QTSOAP_INSTALL ) {
    include( ../qtsolutions/soap/qtsoap.pri )
} else {
    include( $${QTSOAP_INSTALL} )
}
HEADERS += TPUpload.h TPUploadDialog.h TPDownload.h TPDownloadDialog.h
SOURCES += TPUpload.cpp TPUploadDialog.cpp TPDownload.cpp TPDownloadDialog.cpp
DEFINES += GC_HAVE_SOAP

HEADERS += ../qxt/src/qxtspanslider.h \
           ../qxt/src/qxtspanslider_p.h \
           ../qxt/src/qxtscheduleview.h \
           ../qxt/src/qxtscheduleview_p.h \
           ../qxt/src/qxtscheduleheaderwidget.h \
           ../qxt/src/qxtscheduleviewheadermodel_p.h \
           ../qxt/src/qxtscheduleitemdelegate.h \
           ../qxt/src/qxtstyleoptionscheduleviewitem.h

HEADERS += \
        AddDeviceWizard.h \
        AddIntervalDialog.h \
        Aerolab.h \
        AerolabWindow.h \
        AthleteTool.h \
        AllPlot.h \
        AllPlotWindow.h \
        ANT.h \
        ANTChannel.h \
        ANTMessage.h \
        ANTMessages.h \
        ANTlocalController.h \
        ANTplusController.h \
        BatchExportDialog.h \
        BestIntervalDialog.h \
        BinRideFile.h \
        BingMap.h \
        CalendarDownload.h \
        ChooseCyclistDialog.h \
        Colors.h \
        ColorButton.h \
        CommPort.h \
        Computrainer.h \
        Computrainer3dpFile.h \
        ConfigDialog.h \
        CpintPlot.h \
        CriticalPowerWindow.h \
        CsvRideFile.h \
        DataProcessor.h \
        DBAccess.h \
        DatePickerDialog.h \
        DaysScaleDraw.h \
        Device.h \
        DeviceTypes.h \
        DeviceConfiguration.h \
        DialWindow.h \
        DownloadRideDialog.h \
        ErgFile.h \
        ErgDB.h \
        ErgDBDownloadDialog.h \
        ErgFilePlot.h \
        FitlogRideFile.h \
        FitlogParser.h \
        FitRideFile.h \
        GcBubble.h \
        GcCalendar.h \
        GcCalendarModel.h \
        GcPane.h \
        GcRideFile.h \
        GcToolBar.h \
        GcWindowLayout.h \
        GcWindowRegistry.h \
        GcWindowTool.h \
        GoldenCheetah.h \
        GoldenClient.h \
        GoogleMapControl.h \
        GpxParser.h \
        GpxRideFile.h \
        HelpWindow.h \
        HistogramWindow.h \
        HomeWindow.h \
        HrZones.h \
        HrPwPlot.h \
        HrPwWindow.h \
        IntervalItem.h \
        IntervalSummaryWindow.h \
        IntervalTreeView.h \
        JsonRideFile.h \
        LogTimeScaleDraw.h \
        LogTimeScaleEngine.h \
        LTMCanvasPicker.h \
        LTMChartParser.h \
        LTMOutliers.h \
        LTMPlot.h \
        LTMPopup.h \
        LTMSettings.h \
        LTMTool.h \
        LTMTrend.h \
        LTMWindow.h \
        MacroDevice.h \
        MainWindow.h \
        ManualRideDialog.h \
        ManualRideFile.h \
        MetadataWindow.h \
        MetricAggregator.h \
        NewCyclistDialog.h \
        MultiWindow.h \
        NullController.h \
        Pages.h \
        PerfPlot.h \
        PerformanceManagerWindow.h \
        PfPvPlot.h \
        PfPvWindow.h \
        PolarRideFile.h \
        PowerHist.h \
        PowerTapDevice.h \
        PowerTapUtil.h \
        PwxRideFile.h \
        ProtocolHandler.h \
        QuarqdClient.h \
        QuarqParser.h \
        QuarqRideFile.h \
        QxtScheduleViewProxy.h \
        RawRideFile.h \
        RaceDispatcher.h \
        RealtimeData.h \
        RealtimePlotWindow.h \
        RealtimeController.h \
        ComputrainerController.h \
        RealtimePlot.h \
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
        RideWithGPSDialog.h \
        SaveDialogs.h \
        SmallPlot.h \
        RideSummaryWindow.h \
        ScatterPlot.h \
        ScatterWindow.h \
        Season.h \
        SeasonParser.h \
        Serial.h \
        Settings.h \
        SimpleNetworkController.h \
        SimpleNetworkClient.h \
        SpecialFields.h \
        SpinScanPlot.h \
        SpinScanPolarPlot.h \
        SpinScanPlotWindow.h \
        SplitRideDialog.h \
        SplitActivityWizard.h \
        SlfParser.h \
        SlfRideFile.h \
        SmfParser.h \
        SmfRideFile.h \
        SrdRideFile.h \
        SrmRideFile.h \
        StravaDialog.h \
        StressCalculator.h \
        SummaryMetrics.h \
        SummaryWindow.h \
        SyncRideFile.h \
        TcxParser.h \
        TcxRideFile.h \
        TxtRideFile.h \
        TimeUtils.h \
        ToolsDialog.h \
        ToolsRhoEstimator.h \
        TrainTool.h \
        TreeMapWindow.h \
        TreeMapPlot.h \
        Units.h \
        WeeklySummaryWindow.h \
        WeeklyViewItemDelegate.h \
        WithingsDownload.h \
        WkoRideFile.h \
        WorkoutPlotWindow.h \
        WorkoutWizard.h \
        Zones.h \
        ZoneScaleDraw.h

YACCSOURCES = JsonRideFile.y WithingsParser.y
LEXSOURCES  = JsonRideFile.l WithingsParser.l

#-t turns on debug, use with caution
#QMAKE_YACCFLAGS = -t -d

SOURCES += \
        AddDeviceWizard.cpp \
        AddIntervalDialog.cpp \
        AerobicDecoupling.cpp \
        Aerolab.cpp \
        AerolabWindow.cpp \
        AllPlot.cpp \
        AllPlotWindow.cpp \
        AthleteTool.cpp \
        ANT.cpp \
        ANTChannel.cpp \
        ANTMessage.cpp \
        ANTlocalController.cpp \
        ANTplusController.cpp \
        BasicRideMetrics.cpp \
        BatchExportDialog.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        BinRideFile.cpp \
        BingMap.cpp \
        CalendarDownload.cpp \
        ChooseCyclistDialog.cpp \
        Coggan.cpp \
        Colors.cpp \
        ColorButton.cpp \
        CommPort.cpp \
        Computrainer.cpp \
        Computrainer3dpFile.cpp \
        ConfigDialog.cpp \
        CpintPlot.cpp \
        CriticalPowerWindow.cpp \
        CsvRideFile.cpp \
        DanielsPoints.cpp \
        DataProcessor.cpp \
        DBAccess.cpp \
        DatePickerDialog.cpp \
        Device.cpp \
        DeviceTypes.cpp \
        DeviceConfiguration.cpp \
        DialWindow.cpp \
        DownloadRideDialog.cpp \
        ErgDB.cpp \
        ErgDBDownloadDialog.cpp \
        ErgFile.cpp \
        ErgFilePlot.cpp \
        FitlogRideFile.cpp \
        FitlogParser.cpp \
        FitRideFile.cpp \
        FixGaps.cpp \
        FixGPS.cpp \
        FixSpikes.cpp \
        FixTorque.cpp \
        GcBubble.cpp \
        GcCalendar.cpp \
        GcPane.cpp \
        GcRideFile.cpp \
        GcToolBar.cpp \
        GcWindowLayout.cpp \
        GcWindowRegistry.cpp \
        GcWindowTool.cpp \
        GoldenCheetah.cpp \
        GoldenClient.cpp \
        GoogleMapControl.cpp \
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
        IntervalSummaryWindow.cpp \
        IntervalTreeView.cpp \
        LogTimeScaleDraw.cpp \
        LogTimeScaleEngine.cpp \
        LTMCanvasPicker.cpp \
        LTMChartParser.cpp \
        LTMOutliers.cpp \
        LTMPlot.cpp \
        LTMPopup.cpp \
        LTMSettings.cpp \
        LTMTool.cpp \
        LTMTrend.cpp \
        LTMWindow.cpp \
        MacroDevice.cpp \
        MainWindow.cpp \
        ManualRideDialog.cpp \
        ManualRideFile.cpp \
        MetadataWindow.cpp \
        MetricAggregator.cpp \
        NewCyclistDialog.cpp \
        MultiWindow.cpp \
        NullController.cpp \
        Pages.cpp \
        PeakPower.cpp \
        PerfPlot.cpp \
        PerformanceManagerWindow.cpp \
        PfPvPlot.cpp \
        PfPvWindow.cpp \
        PolarRideFile.cpp \
        PowerHist.cpp \
        PowerTapDevice.cpp \
        PowerTapUtil.cpp \
        Protocolhandler.cpp \
        PwxRideFile.cpp \
        QuarqdClient.cpp \
        QuarqParser.cpp \
        QuarqRideFile.cpp \
        RaceDispatcher.cpp \
        RawRideFile.cpp \
        RealtimeData.cpp \
        RealtimeController.cpp \
        ComputrainerController.cpp \
        RealtimePlot.cpp \
        RealtimePlotWindow.cpp \
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
        RideWithGPSDialog.cpp \
        SaveDialogs.cpp \
        ScatterPlot.cpp \
        ScatterWindow.cpp \
        Season.cpp \
        SeasonParser.cpp \
        Serial.cpp \
        Settings.cpp \
        SimpleNetworkController.cpp \
        SimpleNetworkClient.cpp \
        SmallPlot.cpp \
        SpecialFields.cpp \
        SpinScanPlot.cpp \
        SpinScanPolarPlot.cpp \
        SpinScanPlotWindow.cpp \
        SplitRideDialog.cpp \
        SplitActivityWizard.cpp \
        SlfParser.cpp \
        SlfRideFile.cpp \
        SmfParser.cpp \
        SmfRideFile.cpp \
        SrdRideFile.cpp \
        SrmRideFile.cpp \
        StravaDialog.cpp \
        StressCalculator.cpp \
        SummaryMetrics.cpp \
        SummaryWindow.cpp \
        SyncRideFile.cpp \
        TacxCafRideFile.cpp \
        TcxParser.cpp \
        TcxRideFile.cpp \
        TxtRideFile.cpp \
        TimeInZone.cpp \
        TimeUtils.cpp \
        ToolsDialog.cpp \
        ToolsRhoEstimator.cpp \
        TrainTool.cpp \
        TreeMapWindow.cpp \
        TreeMapPlot.cpp \
        TRIMPPoints.cpp \
        WattsPerKilogram.cpp \
        WithingsDownload.cpp \
        WeeklySummaryWindow.cpp \
        WkoRideFile.cpp \
        WorkoutPlotWindow.cpp \
        WorkoutWizard.cpp \
        Zones.cpp \
        main.cpp \

RESOURCES = application.qrc \
            RideWindow.qrc

TRANSLATIONS = translations/gc_fr.ts \
               translations/gc_ja.ts \
               translations/gc_it.ts \
               translations/gc_pt-br.ts \
               translations/gc_de.ts \
               translations/gc_cs.ts \
               translations/gc_ru.ts

OTHER_FILES += \
    web/Rider.js \
    web/ride.js \
    web/jquery-1.6.4.min.js \
    web/MapWindow.html \
    web/StreetViewWindow.html \
    web/Window.css
