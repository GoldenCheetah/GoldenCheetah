
TEMPLATE = app
TARGET = GoldenCheetah
DEPENDPATH += .
INCLUDEPATH += /usr/local/qwt/include ../srm /sw/include
CONFIG += static debug
QT += xml
LIBS += /usr/local/qwt/lib/libqwt.a 
LIBS += ../srm/libsrm.a ../pt/libpt.a
LIBS += -lm -lz -lftd2xx
macx {
    LIBS += -framework Carbon
}

macx || unix {
    LIBS += ../pt/Serial.o
}
LIBS += ../pt/D2XX.o

QMAKE_CXXFLAGS = -DGC_BUILD_DATE="`date +'\"%a_%b_%d,_%Y\"'`"
RC_FILE = images/gc.icns

HEADERS += \
	AllPlot.h \
	BestIntervalDialog.h \
	ChooseCyclistDialog.h \
	CpintPlot.h \
	CsvRideFile.h \
        DatePickerDialog.h \
	DownloadRideDialog.h \
        LogTimeScaleDraw.h \
        LogTimeScaleEngine.h \
	MainWindow.h \
	PowerHist.h \
	RawRideFile.h \
	RideFile.h \
	RideItem.h \
	RideMetric.h \
	SrmRideFile.h \
	TcxParser.h \
	TcxRideFile.h \
	TimeUtils.h \
        Zones.h \
        Pages.h \
        ConfigDialog.h \
        cpint.h \
	PfPvPlot.h

SOURCES += \
	AllPlot.cpp \
        BasicRideMetrics.cpp \
	BestIntervalDialog.cpp \
        BikeScore.cpp \
	ChooseCyclistDialog.cpp \
	CpintPlot.cpp \
	CsvRideFile.cpp \
        DatePickerDialog.cpp \
	DownloadRideDialog.cpp \
        LogTimeScaleDraw.cpp \
        LogTimeScaleEngine.cpp \
	MainWindow.cpp \
	PowerHist.cpp \
	RawRideFile.cpp \
	RideFile.cpp \
	RideItem.cpp \
	RideMetric.cpp \
	SrmRideFile.cpp \
	TcxParser.cpp \
	TcxRideFile.cpp \
	TimeUtils.cpp \
        Zones.cpp \
        Pages.cpp \
        ConfigDialog.cpp \
	cpint.cpp \
	PfPvPlot.cpp \
	main.cpp

RESOURCES = application.qrc

