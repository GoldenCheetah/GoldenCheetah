# To build, see the instructions in gcconfig.pri.in.

include( gcconfig.pri )

TEMPLATE = app
TARGET = GoldenCheetah
DEPENDPATH += .
!isEmpty( BOOST_INCLUDE ) { INCLUDEPATH += $${BOOST_INCLUDE} }
INCLUDEPATH += ../qwt/src
QT += xml sql
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
        BestIntervalDialog.h \
        ChooseCyclistDialog.h \
        CommPort.h \
        ConfigDialog.h \
        CpintPlot.h \
        CriticalPowerWindow.h \
        CsvRideFile.h \
        DBAccess.h \
        DatePickerDialog.h \
        DaysScaleDraw.h \
        Device.h \
        DownloadRideDialog.h \
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
        QuarqParser.h \
        QuarqRideFile.h \
        RawRideFile.h \
        RideCalendar.h \
        RideFile.h \
        RideImportWizard.h \
        RideItem.h \
        RideMetric.h \
        Season.h \
        SeasonParser.h \
        Settings.h \
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
        XmlRideFile.h \
        Zones.h \
        srm.h

SOURCES += \
        AllPlot.cpp \
        AllPlotWindow.cpp \
        BasicRideMetrics.cpp \
        BestIntervalDialog.cpp \
        BikeScore.cpp \
        ChooseCyclistDialog.cpp \
        CommPort.cpp \
        ConfigDialog.cpp \
        CpintPlot.cpp \
        CriticalPowerWindow.cpp \
        CsvRideFile.cpp \
        DBAccess.cpp \
        DatePickerDialog.cpp \
        Device.cpp \
        DownloadRideDialog.cpp \
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
        QuarqParser.cpp \
        QuarqRideFile.cpp \
        RawRideFile.cpp \
        RideCalendar.cpp \
        RideFile.cpp \
        RideImportWizard.cpp \
        RideItem.cpp \
        RideMetric.cpp \
        Season.cpp \
        SeasonParser.cpp \
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
        XmlRideFile.cpp \
        Zones.cpp \
        main.cpp \
        srm.cpp

RESOURCES = application.qrc

