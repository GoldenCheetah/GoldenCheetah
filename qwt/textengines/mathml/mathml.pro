# -*- mode: sh -*- ###########################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
##############################################

VVERSION = $$[QT_VERSION]
isEmpty(VVERSION) { 

	# Qt3

	message(qwtmathml is not supported for Qt3 !)
	TEMPLATE  = subdirs

} else {

	# Qt4

	include( ../textengines.pri )

	exists( qtmmlwidget.cpp ) {

		TARGET    = qwtmathml$${SUFFIX_STR}
		VERSION   = 1.0.0
		QT       += xml

		HEADERS = \
			qwt_mathml_text_engine.h

		SOURCES = \
			qwt_mathml_text_engine.cpp

		# The files below can be found in the MathML tarball of the Qt Solution 
    	# package http://www.trolltech.com/products/qt/addon/solutions/catalog/4/Widgets/qtmmlwidget
		# that is available for owners of a commercial Qt license.
		#
		# Copy them here, or modify the pro file to your installation.
	
		HEADERS += qtmmlwidget.h
		SOURCES += qtmmlwidget.cpp
	}

	else {
		error( "qtmmlwidget.cpp is missing, see http://www.trolltech.com/products/qt/addon/solutions/catalog/4/Widgets/qtmmlwidget" )
	}
}
