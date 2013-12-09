################################################################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
################################################################

# Copied and modified from qt_functions.prf

defineReplace(qwtLibraryTarget) {

    unset(LIBRARY_NAME)
    LIBRARY_NAME = $$1

    mac:contains(QWT_CONFIG, QwtFramework) {

        QMAKE_FRAMEWORK_BUNDLE_NAME = $$LIBRARY_NAME
        export(QMAKE_FRAMEWORK_BUNDLE_NAME)
    }

    contains(TEMPLATE, .*lib):CONFIG(debug, debug|release) {

        !debug_and_release|build_pass {

            mac:RET = $$member(LIBRARY_NAME, 0)_debug
            win32:RET = $$member(LIBRARY_NAME, 0)d
        }
    }

    isEmpty(RET):RET = $$LIBRARY_NAME
    return($$RET)
}

defineTest(qwtAddLibrary) {

    LIB_NAME = $$1

    unset(LINKAGE)

    mac:contains(QWT_CONFIG, QwtFramework) {

        LINKAGE = -framework $${LIB_NAME}
    }

    isEmpty(LINKAGE) {

        if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {

            mac:LINKAGE = -l$${LIB_NAME}_debug
            win32:LINKAGE = -l$${LIB_NAME}d
        }
    }

    isEmpty(LINKAGE) {

        LINKAGE = -l$${LIB_NAME}
    }

    !isEmpty(QMAKE_LSB) {

        QMAKE_LFLAGS *= --lsb-shared-libs=$${LIB_NAME}
    }

    LIBS += $$LINKAGE
    export(LIBS)
    export(QMAKE_LFLAGS)

    return(true)
}
