# -*- mode: sh -*- ################################################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
###################################################################

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

TEMPLATE     = app

MOC_DIR      = moc
INCLUDEPATH += $${QWT_ROOT}/src
DEPENDPATH  += $${QWT_ROOT}/src
OBJECTS_DIR  = obj$${SUFFIX_STR}
DESTDIR      = $${QWT_ROOT}/examples/bin$${SUFFIX_STR}

QWTLIB       = qwt$${SUFFIX_STR}

win32 {
    contains(CONFIG, QwtDll) {
        DEFINES    += QT_DLL QWT_DLL
        QWTLIB = $${QWTLIB}$${VER_MAJ}
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
    LIBS        += -L$${QWT_ROOT}/lib -l$${QWTLIB}
}
