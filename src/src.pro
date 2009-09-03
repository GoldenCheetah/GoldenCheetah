# To build, see the instructions in gcconfig.pri.in.

include( gcconfig.pri )

TEMPLATE = app
TARGET = GoldenCheetah
DEPENDPATH += .
!isEmpty( BOOST_INCLUDE ) { INCLUDEPATH += $${BOOST_INCLUDE} }
!isEmpty( QWT_INCLUDE ) { INCLUDEPATH += $${QWT_INCLUDE} }
QT += xml sql
LIBS += $${QWT_LIB}
LIBS += -lm

!win32 {
    QMAKE_CXXFLAGS += -DGC_BUILD_DATE="`date +'\"%a_%b_%d,_%Y\"'`"
    QMAKE_CXXFLAGS += -DGC_SVN_VERSION=\\\"`svnversion . | cut -f '2' -d ':'`\\\"
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

QMAKE_CXXFLAGS += -DGC_MAJOR_VER=1
QMAKE_CXXFLAGS += -DGC_MINOR_VER=1

macx {
    LIBS += -framework Carbon
}

!win32 {
    RC_FILE = images/gc.icns
    HEADERS += Serial.h
    SOURCES += Serial.cpp
}

HEADERS += \
        AllPlot.h \
        BestIntervalDialog.h \
        ChooseCyclistDialog.h \
        CpintPlot.h \
        CsvRideFile.h \
        WkoRideFile.h \
        DBAccess.h \
        Device.h \
        DownloadRideDialog.h \
        MainWindow.h \
        PfPvPlot.h \
        PolarRideFile.h \
        PowerHist.h \
        RawRideFile.h \
        RideFile.h \
        RideItem.h \
        RideMetric.h \
        SrmRideFile.h \
        TcxParser.h \
        TcxRideFile.h \
        QuarqParser.h \
        QuarqRideFile.h \
        TimeUtils.h \
        ConfigDialog.h \
        DatePickerDialog.h \
        CommPort.h \
        LogTimeScaleDraw.h \
        LogTimeScaleEngine.h \
        Pages.h \
        PowerTapDevice.h \
        PowerTapUtil.h \
        ToolsDialog.h \
        Zones.h \
        srm.h \
        MetricAggregator.h \
        Season.h \
        SummaryMetrics.h \
        SplitRideDialog.h \
        DaysScaleDraw.h \
        Settings.h \
        XmlRideFile.h \
        ManualRideFile.h \
        ManualRideDialog.h \
        RideCalendar.h

SOURCES += \
        AllPlot.cpp \
        BestIntervalDialog.cpp \
        ChooseCyclistDialog.cpp \
        CpintPlot.cpp \
        CsvRideFile.cpp \
        WkoRideFile.cpp \
        DBAccess.cpp \
        Device.cpp \
        DownloadRideDialog.cpp \
        MainWindow.cpp \
        PfPvPlot.cpp \
        PolarRideFile.cpp \
        PowerHist.cpp \
        RawRideFile.cpp \
        RideFile.cpp \
        RideItem.cpp \
        RideMetric.cpp \
        SrmRideFile.cpp \
        TcxParser.cpp \
        TcxRideFile.cpp \
        QuarqParser.cpp \
        QuarqRideFile.cpp \
        TimeUtils.cpp \
        BasicRideMetrics.cpp \
        BikeScore.cpp \
        ConfigDialog.cpp \
        DatePickerDialog.cpp \
        CommPort.cpp \
        LogTimeScaleDraw.cpp \
        LogTimeScaleEngine.cpp \
        Pages.cpp \
        PowerTapDevice.cpp \
        PowerTapUtil.cpp \
        ToolsDialog.cpp \
        Zones.cpp \
        main.cpp \
        srm.cpp \
        Season.cpp \
        MetricAggregator.cpp \
        SummaryMetrics.cpp \
        SplitRideDialog.cpp \
        XmlRideFile.cpp \
        ManualRideFile.cpp \
        ManualRideDialog.cpp \
        RideCalendar.cpp

win32 {
    INCLUDEPATH += ./win32

    LIBS += -lws2_32

    QMAKE_LFLAGS = -Wl,--enable-runtime-pseudo-reloc \
        -Wl,--script,win32/i386pe.x-no-rdata
    //QMAKE_CXXFLAGS += -fdata-sections
    RC_FILE = windowsico.rc

    QMAKE_EXTRA_TARGETS += revtarget
    PRE_TARGETDEPS      += temp_version.h
    revtarget.target     = temp_version.h
    revtarget.commands   = @echo "const char * const g_builddate = \"$(shell date /t)\";" \
                                 "const char * const g_svnversion = \"$(shell svnversion .)\";" > $$revtarget.target

    # this line has to be after SOURCES and HEADERS is declared or it doesn't work
    revtarget.depends = $$SOURCES $$HEADERS $$FORMS
}


RESOURCES = application.qrc

