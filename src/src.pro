TEMPLATE = app
TARGET = GoldenCheetah
DEPENDPATH += .
INCLUDEPATH += /usr/local/qwt/include /sw/include /usr/local/include
CONFIG += static debug
QT += xml sql
LIBS += /usr/local/qwt/lib/libqwt.a
LIBS += -lm -lz

!win32 {
    QMAKE_CXXFLAGS = -DGC_BUILD_DATE="`date +'\"%a_%b_%d,_%Y\"'`"
    QMAKE_CXXFLAGS += -DGC_SVN_VERSION=\\\"`svnversion . | cut -f '2' -d ':'`\\\"
    LIBS += srmio/libsrmio.a
    HEADERS += SrmDevice.h
    SOURCES += SrmDevice.cpp
}

QMAKE_CXXFLAGS += -DGC_MAJOR_VER=1
QMAKE_CXXFLAGS += -DGC_MINOR_VER=1

RC_FILE = images/gc.icns

macx {
    LIBS += -framework Carbon
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    CONFIG+=x86 ppc 
}

HEADERS += \
        AllPlot.h \
        BestIntervalDialog.h \
        ChooseCyclistDialog.h \
        CpintPlot.h \
        CsvRideFile.h \
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
        TimeUtils.h \
        ConfigDialog.h \
        D2XX.h \
        DatePickerDialog.h \
        CommPort.h \
        LogTimeScaleDraw.h \
        LogTimeScaleEngine.h \
        Pages.h \
        PowerTapDevice.h \
        PowerTapUtil.h \
        Serial.h \
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
        ManualRideDialog.h

SOURCES += \
        AllPlot.cpp \
        BestIntervalDialog.cpp \
        ChooseCyclistDialog.cpp \
        CpintPlot.cpp \
        CsvRideFile.cpp \
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
        TimeUtils.cpp \
        BasicRideMetrics.cpp \
        BikeScore.cpp \
        ConfigDialog.cpp \
        D2XX.cpp \
        DatePickerDialog.cpp \
        CommPort.cpp \
        LogTimeScaleDraw.cpp \
        LogTimeScaleEngine.cpp \
        Pages.cpp \
        PowerTapDevice.cpp \
        PowerTapUtil.cpp \
        Serial.cpp \
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
        ManualRideDialog.cpp

# win32 is after SOURCES and HEADERS so we can remove Serial.h/.cpp
win32 {
    INCLUDEPATH += ../../../2009.01/qt/include \
        ../../qwt-5.1.1/src \
        ../../boost_1_38_0 \
        ./win32

    LIBS = ../../qwt-5.1.1/lib/libqwt5.a \
        -lws2_32

    QMAKE_LFLAGS = -Wl,--enable-runtime-pseudo-reloc \
        -Wl,--script,win32/i386pe.x-no-rdata
    //QMAKE_CXXFLAGS += -fdata-sections
    RC_FILE -= images/gc.icns
    RC_FILE += windowsico.rc
    HEADERS -= Serial.h
    SOURCES -= Serial.cpp

    QMAKE_EXTRA_TARGETS += revtarget
    PRE_TARGETDEPS      += temp_version.h
    revtarget.target     = temp_version.h
    revtarget.commands   = @echo "const char * const g_builddate = \"$(shell date /t)\";" \
                                 "const char * const g_svnversion = \"$(shell svnversion .)\";" > $$revtarget.target

    # this line has to be after SOURCES and HEADERS is declared or it doesn't work
    revtarget.depends = $$SOURCES $$HEADERS $$FORMS
}


RESOURCES = application.qrc

