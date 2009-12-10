# To build, see the instructions in gcconfig.pri.in.

include( gcconfig.pri )

TEMPLATE = app
TARGET = GoldenCheetah
DEPENDPATH += .
!isEmpty( BOOST_INCLUDE ) { INCLUDEPATH += $${BOOST_INCLUDE} }
INCLUDEPATH += ../qwt/src
QT += xml sql network
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

macx {
    LIBS += -framework Carbon
}

!win32 {
    RC_FILE = images/gc.icns
    HEADERS += Serial.h
    SOURCES += Serial.cpp
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
        AllPlot.h \
        AllPlotWindow.h \
        ANTplusController.h \
        BestIntervalDialog.h \
        ChooseCyclistDialog.h \
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
        HistogramWindow.h \
        LogTimeScaleDraw.h \
        LogTimeScaleEngine.h \
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
        RideMetric.h \
        RideSummaryWindow.h \
        Season.h \
        SeasonParser.h \
        Settings.h \
        SimpleNetworkController.h \
        SimpleNetworkClient.h \
        SplitRideDialog.h \
        SrmRideFile.h \
        StressCalculator.h \
        SummaryMetrics.h \
        TcxParser.h \
        TcxRideFile.h \
        TimeUtils.h \
        ToolsDialog.h \
        Units.h \
        WeeklySummaryWindow.h \
        WkoRideFile.h \
        Zones.h \

SOURCES += \
        AerobicDecoupling.cpp \
        AllPlot.cpp \
        AllPlotWindow.cpp \
        ANTplusController.cpp \
        BasicRideMetrics.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        DanielsPoints.cpp \
        ChooseCyclistDialog.cpp \
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
        HistogramWindow.cpp \
        LogTimeScaleDraw.cpp \
        LogTimeScaleEngine.cpp \
        MainWindow.cpp \
        ManualRideDialog.cpp \
        ManualRideFile.cpp \
        MetricAggregator.cpp \
        Pages.cpp \
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
        RideMetric.cpp \
        RideSummaryWindow.cpp \
        Season.cpp \
        SeasonParser.cpp \
        SimpleNetworkController.cpp \
        SimpleNetworkClient.cpp \
        SplitRideDialog.cpp \
        SrmRideFile.cpp \
        StressCalculator.cpp \
        SummaryMetrics.cpp \
        TcxParser.cpp \
        TcxRideFile.cpp \
        TimeUtils.cpp \
        ToolsDialog.cpp \
        WeeklySummaryWindow.cpp \
        WkoRideFile.cpp \
        Zones.cpp \
        main.cpp \

RESOURCES = application.qrc

