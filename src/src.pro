# To build, see the instructions in gcconfig.pri.in.

include( gcconfig.pri )

TEMPLATE = app
TARGET = GoldenCheetah
DEPENDPATH += .
!isEmpty( BOOST_INCLUDE ) { INCLUDEPATH += $${BOOST_INCLUDE} }
INCLUDEPATH += ../qwt/src
QT += xml sql network webkit
LIBS += ../qwt/lib/libqwt.a
LIBS += -lm

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

macx {
    LIBS += -framework Carbon
}

!win32 {
    RC_FILE = images/gc.icns
}
win32 {
    INCLUDEPATH += ./win32
    LIBS += -lws2_32
    QMAKE_LFLAGS = -Wl,--enable-runtime-pseudo-reloc \
        -Wl,--script,win32/i386pe.x-no-rdata
    //QMAKE_CXXFLAGS += -fdata-sections
    RC_FILE = windowsico.rc
}

HEADERS += \
        Aerolab.h \
        AerolabWindow.h \
        AllPlot.h \
        AllPlotWindow.h \
        ANTplusController.h \
        BestIntervalDialog.h \
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
        GcRideFile.h \
        GoogleMapControl.h \
        HistogramWindow.h \
        IntervalItem.h \
        LogTimeScaleDraw.h \
        LogTimeScaleEngine.h \
        LTMCanvasPicker.h \
        LTMChartParser.h \
        LTMPlot.h \
        LTMSettings.h \
        LTMTool.h \
        LTMTrend.h \
        LTMWindow.h \
        MainWindow.h \
        ManualRideDialog.h \
        ManualRideFile.h \
        MetricAggregator.h \
        Pages.h \
        PerfPlot.h \
        PerformanceManagerWindow.h \
        PfPvPlot.h \
        PfPvWindow.h \
        PolarRideFile.h \
        PowerHist.h \
        PowerTapDevice.h \
        PowerTapUtil.h \
        QuarqdClient.h \
        QuarqParser.h \
        QuarqRideFile.h \
        RawRideFile.h \
        RealtimeData.h \
        RealtimeWindow.h \
        RealtimeController.h \
        ComputrainerController.h \
        RealtimePlot.h \
        RideCalendar.h \
        RideFile.h \
        RideImportWizard.h \
        RideItem.h \
        RideMetadata.h \
        RideMetric.h \
        SaveDialogs.h \
        RideSummaryWindow.h \
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
        TcxParser.h \
        TcxRideFile.h \
        TimeUtils.h \
        ToolsDialog.h \
        TrainTabs.h \
        TrainTool.h \
        TrainWindow.h \
        Units.h \
        ViewSelection.h \
        WeeklySummaryWindow.h \
        WkoRideFile.h \
        Zones.h \

SOURCES += \
        AerobicDecoupling.cpp \
        Aerolab.cpp \
        AerolabWindow.cpp \
        AllPlot.cpp \
        AllPlotWindow.cpp \
        ANTplusController.cpp \
        BasicRideMetrics.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        DanielsPoints.cpp \
        ChooseCyclistDialog.cpp \
        Colors.cpp \
        ColorButton.cpp \
        CommPort.cpp \
        Computrainer.cpp \
        Computrainer3dpFile.cpp \
        ConfigDialog.cpp \
        CpintPlot.cpp \
        CriticalPowerWindow.cpp \
        CsvRideFile.cpp \
        DBAccess.cpp \
        DatePickerDialog.cpp \
        Device.cpp \
        DeviceTypes.cpp \
        DeviceConfiguration.cpp \
        DownloadRideDialog.cpp \
        ErgFile.cpp \
        ErgFilePlot.cpp \
        FitRideFile.cpp \
        GcRideFile.cpp \
        GoogleMapControl.cpp \
        HistogramWindow.cpp \
        IntervalItem.cpp \
        LogTimeScaleDraw.cpp \
        LogTimeScaleEngine.cpp \
        LTMCanvasPicker.cpp \
        LTMChartParser.cpp \
        LTMPlot.cpp \
        LTMSettings.cpp \
        LTMTool.cpp \
        LTMTrend.cpp \
        LTMWindow.cpp \
        MainWindow.cpp \
        ManualRideDialog.cpp \
        ManualRideFile.cpp \
        MetricAggregator.cpp \
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
        QuarqdClient.cpp \
        QuarqParser.cpp \
        QuarqRideFile.cpp \
        RawRideFile.cpp \
        RealtimeData.cpp \
        RealtimeController.cpp \
        ComputrainerController.cpp \
        RealtimeWindow.cpp \
        RealtimePlot.cpp \
        RideCalendar.cpp \
        RideFile.cpp \
        RideImportWizard.cpp \
        RideItem.cpp \
        RideMetadata.cpp \
        RideMetric.cpp \
        SaveDialogs.cpp \
        RideSummaryWindow.cpp \
        Season.cpp \
        SeasonParser.cpp \
        Serial.cpp \
        SimpleNetworkController.cpp \
        SimpleNetworkClient.cpp \
        SpecialFields.cpp \
        SplitRideDialog.cpp \
        SrdRideFile.cpp \
        SrmRideFile.cpp \
        StressCalculator.cpp \
        TcxParser.cpp \
        TcxRideFile.cpp \
        TimeInZone.cpp \
        TimeUtils.cpp \
        ToolsDialog.cpp \
        TrainTabs.cpp \
        TrainTool.cpp \
        TrainWindow.cpp \
        ViewSelection.cpp \
        WeeklySummaryWindow.cpp \
        WkoRideFile.cpp \
        Zones.cpp \
        main.cpp \

RESOURCES = application.qrc

TRANSLATIONS = translations/gc_fr.ts translations/gc_ja.ts

