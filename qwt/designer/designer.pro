# -*- mode: sh -*- ###########################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
##############################################

QWT_ROOT = ..

include ( $${QWT_ROOT}/qwtconfig.pri )

contains(CONFIG, QwtDesigner) {

	CONFIG    += warn_on

	SUFFIX_STR =

    VVERSION = $$[QT_VERSION]
    isEmpty(VVERSION) {

        # Qt 3
        debug {
            SUFFIX_STR = $${DEBUG_SUFFIX}
        }
        else {
            SUFFIX_STR = $${RELEASE_SUFFIX}
        }
    }
    else {

        CONFIG(debug, debug|release) {
            SUFFIX_STR = $${DEBUG_SUFFIX}
        }
        else {
            SUFFIX_STR = $${RELEASE_SUFFIX}
        }
    }

	TEMPLATE        = lib
	MOC_DIR         = moc
	OBJECTS_DIR     = obj$${SUFFIX_STR}
	DESTDIR         = plugins/designer
	INCLUDEPATH    += $${QWT_ROOT}/src 
	DEPENDPATH     += $${QWT_ROOT}/src 

    LIBNAME         = qwt$${SUFFIX_STR}
	contains(CONFIG, QwtDll) {
		win32 {
			DEFINES += QT_DLL QWT_DLL
			LIBNAME = $${LIBNAME}$${VER_MAJ}
		}
	}

	!contains(CONFIG, QwtPlot) {
		DEFINES += NO_QWT_PLOT
	}

	!contains(CONFIG, QwtWidgets) {
		DEFINES += NO_QWT_WIDGETS
	}

	unix:LIBS      += -L$${QWT_ROOT}/lib -l$${LIBNAME}
	win32-msvc:LIBS  += $${QWT_ROOT}/lib/$${LIBNAME}.lib
	win32-msvc.net:LIBS  += $${QWT_ROOT}/lib/$${LIBNAME}.lib
	win32-msvc2002:LIBS += $${QWT_ROOT}/lib/$${LIBNAME}.lib
	win32-msvc2003:LIBS += $${QWT_ROOT}/lib/$${LIBNAME}.lib
	win32-msvc2005:LIBS += $${QWT_ROOT}/lib/$${LIBNAME}.lib
	win32-msvc2008:LIBS += $${QWT_ROOT}/lib/$${LIBNAME}.lib
	win32-g++:LIBS   += -L$${QWT_ROOT}/lib -l$${LIBNAME}

	# isEmpty(QT_VERSION) does not work with Qt-4.1.0/MinGW

	VVERSION = $$[QT_VERSION]
	isEmpty(VVERSION) {
		# Qt 3 
		TARGET    = qwtplugin$${SUFFIX_STR}
		CONFIG   += qt plugin

		UI_DIR    = ui

		HEADERS  += qwtplugin.h
		SOURCES  += qwtplugin.cpp

		target.path = $(QTDIR)/plugins/designer
		INSTALLS += target

		IMAGES  += \
			pixmaps/qwtplot.png \
			pixmaps/qwtanalogclock.png \
			pixmaps/qwtcounter.png \
			pixmaps/qwtcompass.png \
			pixmaps/qwtdial.png \
			pixmaps/qwtknob.png \
			pixmaps/qwtscale.png \
			pixmaps/qwtslider.png \
			pixmaps/qwtthermo.png \
			pixmaps/qwtwheel.png \
			pixmaps/qwtwidget.png 

	} else {

		# Qt 4

		TARGET    = qwt_designer_plugin$${SUFFIX_STR}
		CONFIG    += qt designer plugin 

		RCC_DIR   = resources

		HEADERS += \
			qwt_designer_plugin.h

		SOURCES += \
			qwt_designer_plugin.cpp

	    contains(CONFIG, QwtPlot) {

			HEADERS += \
				qwt_designer_plotdialog.h

			SOURCES += \
				qwt_designer_plotdialog.cpp
		}

		RESOURCES += \
			qwt_designer_plugin.qrc

		target.path = $$[QT_INSTALL_PLUGINS]/designer
		INSTALLS += target
	}
}
else {
	TEMPLATE        = subdirs # do nothing
}
