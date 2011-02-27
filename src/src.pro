# To build, see the instructions in gcconfig.pri.in.

include( gcconfig.pri )

TEMPLATE = app
TARGET = GoldenCheetah
DEPENDPATH += .

!isEmpty( BOOST_INCLUDE ) { INCLUDEPATH += $${BOOST_INCLUDE} }
INCLUDEPATH += ../qwt/src ../qxt/src
QT += xml sql network webkit phonon
LIBS += ../qwt/lib/libqwt.a
LIBS += -lm

!isEmpty( LIBOAUTH_INSTALL ) {
INCLUDEPATH += $${LIBOAUTH_INSTALL}/include
LIBS +=  $${LIBOAUTH_INSTALL}/lib/liboauth.a
LIBS += $${LIBCRYPTO_INSTALL}
LIBS += $${LIBZ_INSTALL}
LIBS += $${LIBCURL_INSTALL}
DEFINES += GC_HAVE_LIBOAUTH
SOURCES += TwitterDialog.cpp
HEADERS += TwitterDialog.h
}

!isEmpty( D2XX_INCLUDE ) {
  INCLUDEPATH += $${D2XX_INCLUDE}
  HEADERS += D2XX.h
  SOURCES += D2XX.cpp
}

!isEmpty( SRMIO_INSTALL ) {
    !isEmpty( SRMIO_INCLUDE ) { INCLUDEPATH += $${SRMIO_INCLUDE} }
    LIBS += $${SRMIO_LIB}
    HEADERS += SrmDevice.h
    SOURCES += SrmDevice.cpp
}

!isEmpty( QWT3D_INSTALL ) {
  INCLUDEPATH += $${QWT3D_INSTALL}/include
  LIBS += $${QWT3D_INSTALL}/lib/libqwtplot3d.a
  CONFIG += qwt3d
}
isEmpty( QWT3D_INSTALL ):linux-g++:exists( /usr/include/qwtplot3d-qt4 ):exists( /usr/lib/libqwtplot3d-qt4.so ) {
  INCLUDEPATH += /usr/include/qwtplot3d-qt4
  LIBS += /usr/lib/libqwtplot3d-qt4.so
  CONFIG += qwt3d
}
qwt3d {
  QT += opengl
  HEADERS += ModelPlot.h ModelWindow.h
  SOURCES += ModelPlot.cpp ModelWindow.cpp
  DEFINES += GC_HAVE_QWTPLOT3D
}

!isEmpty( KML_INSTALL) {
    KML_INCLUDE = $${KML_INSTALL}/include
    KML_LIBS = $${KML_INSTALL}/lib/libkmldom.a \
               $${KML_INSTALL}/lib/libkmlconvenience.a \
               $${KML_INSTALL}/lib/libkmlengine.a \
               $${KML_INSTALL}/lib/libkmlbase.a \

    LIBS += $${KML_LIBS} $${KML_LIBS}
    DEFINES += GC_HAVE_KML
    SOURCES += KmlRideFile.cpp
    HEADERS += KmlRideFile.h
}

!isEmpty( ICAL_INSTALL) {
    HEADERS += ICalendar.h DiaryWindow.h GcCalendarModel.h CalDAV.h
    SOURCES += ICalendar.cpp DiaryWindow.cpp CalDAV.cpp
    ICAL_INCLUDE = $${ICAL_INSTALL}/include
    ICAL_LIBS = $${ICAL_INSTALL}/lib/libical.a
    DEFINES += GC_HAVE_ICAL
    INCLUDEPATH += $${ICAL_INCLUDE}
    LIBS += $${ICAL_LIBS}
}

macx {
    LIBS += -lobjc -framework Carbon -framework AppKit
    HEADERS += QtMacSegmentedButton.h
    SOURCES += QtMacSegmentedButton.mm
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

include( ../qtsolutions/soap/qtsoap.pri )
HEADERS += TPUpload.h TPUploadDialog.h TPDownload.h TPDownloadDialog.h
SOURCES += TPUpload.cpp TPUploadDialog.cpp TPDownload.cpp TPDownloadDialog.cpp
DEFINES += GC_HAVE_SOAP

HEADERS += ../qxt/src/qxtspanslider.h \
           ../qxt/src/qxtspanslider_p.h \
           ../qxt/src/qxtscheduleview.h \
           .././qxt/src/qxtscheduleview_p.h \
           ../qxt/src/qxtscheduleheaderwidget.h \
           ../qxt/src/qxtscheduleviewheadermodel_p.h \
           ../qxt/src/qxtscheduleitemdelegate.h \
           ../qxt/src/qxtstyleoptionscheduleviewitem.h

HEADERS += \
        Aerolab.h \
        AerolabWindow.h \
        AthleteTool.h \
        AllPlot.h \
        AllPlotWindow.h \
        ANT.h \
        ANTMessages.h \
        ANTlocalController.h \
        ANTplusController.h \
        BestIntervalDialog.h \
        BinRideFile.h \
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
        DownloadRideDialog.h \
        ErgFile.h \
        ErgFilePlot.h \
        FitRideFile.h \
        GcPane.h \
        GcRideFile.h \
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
        MainWindow.h \
        ManualRideDialog.h \
        ManualRideFile.h \
        MetricAggregator.h \
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
        RealtimeWindow.h \
        RealtimeController.h \
        ComputrainerController.h \
        RealtimePlot.h \
        RideCalendar.h \
        RideEditor.h \
        RideFile.h \
        RideFileCommand.h \
        RideFileTableModel.h \
        RideImportWizard.h \
        RideItem.h \
        RideMetadata.h \
        RideMetric.h \
        RideNavigator.h \
        RideNavigatorProxy.h \
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
        SplitRideDialog.h \
        SrdRideFile.h \
        SrmRideFile.h \
        StressCalculator.h \
        SummaryMetrics.h \
        SummaryWindow.h \
        TcxParser.h \
        TcxRideFile.h \
        TxtRideFile.h \
        TimeUtils.h \
        ToolsDialog.h \
        TrainTabs.h \
        TrainTool.h \
        TrainWindow.h \
        TreeMapWindow.h \
        TreeMapPlot.h \
        Units.h \
        ViewSelection.h \
        WeeklySummaryWindow.h \
        WeeklyViewItemDelegate.h \
        WithingsDownload.h \
        WkoRideFile.h \
        WorkoutWizard.h \
        Zones.h \
        ZoneScaleDraw.h

YACCSOURCES = JsonRideFile.y WithingsParser.y
LEXSOURCES = JsonRideFile.l WithingsParser.l

#-t turns on debug, use with caution
#QMAKE_YACCFLAGS = -t -d

SOURCES += \
        AerobicDecoupling.cpp \
        Aerolab.cpp \
        AerolabWindow.cpp \
        AllPlot.cpp \
        AllPlotWindow.cpp \
        AthleteTool.cpp \
        ANT.cpp \
        ANTlocalController.cpp \
        ANTplusController.cpp \
        BasicRideMetrics.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        BinRideFile.cpp \
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
        DownloadRideDialog.cpp \
        ErgFile.cpp \
        ErgFilePlot.cpp \
        FitRideFile.cpp \
        FixGaps.cpp \
        FixGPS.cpp \
        FixSpikes.cpp \
        FixTorque.cpp \
        GcPane.cpp \
        GcRideFile.cpp \
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
        MainWindow.cpp \
        ManualRideDialog.cpp \
        ManualRideFile.cpp \
        MetricAggregator.cpp \
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
        RealtimeWindow.cpp \
        RealtimePlot.cpp \
        RideCalendar.cpp \
        RideEditor.cpp \
        RideFile.cpp \
        RideFileCommand.cpp \
        RideFileTableModel.cpp \
        RideImportWizard.cpp \
        RideItem.cpp \
        RideMetadata.cpp \
        RideMetric.cpp \
        RideNavigator.cpp \
        RideSummaryWindow.cpp \
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
        SplitRideDialog.cpp \
        SrdRideFile.cpp \
        SrmRideFile.cpp \
        StressCalculator.cpp \
        SummaryMetrics.cpp \
        SummaryWindow.cpp \
        TacxCafRideFile.cpp \
        TcxParser.cpp \
        TcxRideFile.cpp \
        TxtRideFile.cpp \
        TimeInZone.cpp \
        TimeUtils.cpp \
        ToolsDialog.cpp \
        TrainTabs.cpp \
        TrainTool.cpp \
        TrainWindow.cpp \
        TreeMapWindow.cpp \
        TreeMapPlot.cpp \
        TRIMPPoints.cpp \
        ViewSelection.cpp \
        WattsPerKilogram.cpp \
        WithingsDownload.cpp \
        WeeklySummaryWindow.cpp \
        WkoRideFile.cpp \
        WorkoutWizard.cpp \
        Zones.cpp \
        main.cpp \

RESOURCES = application.qrc

TRANSLATIONS = translations/gc_fr.ts \
               translations/gc_ja.ts \
               translations/gc_it.ts \
               translations/gc_pt-br.ts \
               translations/gc_de.ts \
               translations/gc_cs.ts \
               translations/gc_ru.ts

