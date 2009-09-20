# -*- mode: sh -*- ################################################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
###################################################################

include( ../qwtconfig.pri )

TEMPLATE = subdirs

contains(CONFIG, QwtPlot) {
	
	SUBDIRS += \
    	cpuplot \
    	curvdemo1   \
    	curvdemo2 \
    	simple_plot \
    	realtime_plot \
    	spectrogram \
    	histogram 

	contains(CONFIG, QwtWidgets) {

		SUBDIRS += \
    		bode \
    		data_plot \
    		event_filter
	}
	
	contains(CONFIG, QwtSVGItem) {

		SUBDIRS += \
			svgmap
	}
}

contains(CONFIG, QwtWidgets) {

	SUBDIRS += \
    	sysinfo \
    	radio \
    	dials \
    	sliders
}


