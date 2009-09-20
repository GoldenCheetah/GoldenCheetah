# -*- mode: sh -*- ###########################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
##############################################

QWT_ROOT = ../..

include( $${QWT_ROOT}/qwtconfig.pri )

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

TEMPLATE  = lib

contains(CONFIG, QwtDll ) {
    CONFIG += dll
}
else {
    CONFIG += staticlib
}

MOC_DIR         = moc
OBJECTS_DIR     = obj$${SUFFIX_STR}
DESTDIR         = $${QWT_ROOT}/lib
INCLUDEPATH    += $${QWT_ROOT}/src
DEPENDPATH     += $${QWT_ROOT}/src

QWTLIB       = qwt$${SUFFIX_STR}

win32 {
    QwtDll {
        DEFINES += QT_DLL QWT_DLL QWT_MAKEDLL
        QWTLIB   = $${QWTLIB}$${VER_MAJ}
    }

    win32-msvc:LIBS  += $${QWT_ROOT}/lib/$${QWTLIB}.lib
    win32-msvc.net:LIBS  += $${QWT_ROOT}/lib/$${QWTLIB}.lib
    win32-msvc2002:LIBS += $${QWT_ROOT}/lib/$${QWTLIB}.lib
    win32-msvc2003:LIBS += $${QWT_ROOT}/lib/$${QWTLIB}.lib
    win32-msvc2005:LIBS += $${QWT_ROOT}/lib/$${QWTLIB}.lib
    win32-msvc2008:LIBS += $${QWT_ROOT}/lib/$${QWTLIB}.lib
    win32-g++:LIBS   += -L$${QWT_ROOT}/lib -l$${QWTLIB}
}
else {
    LIBS      += -L$${QWT_ROOT}/lib -l$${QWTLIB}
}

target.path    = $$INSTALLBASE/lib
headers.path   = $$INSTALLBASE/include
doc.path       = $$INSTALLBASE/doc

headers.files  = $$HEADERS
INSTALLS       = target headers
