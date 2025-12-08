TEMPLATE = lib
CONFIG += staticlib
CONFIG -= precompile_header

TARGET = sip_lib

# Common settings
app_path = ../..
# We need to include gcconfig, because it can contain the paths to the headers if we do not use autodetection.
include($${app_path}/gcconfig.pri)

# Only build this library if we enable Python support.
contains(DEFINES, "GC_WANT_PYTHON") {
    message("Enabling SIP Python bindings")
    INCLUDEPATH += $$replace(PYTHONINCLUDES, ^-I, )
    INCLUDEPATH += $${GSL_INCLUDES}
    INCLUDEPATH += $${LIBZ_INCLUDE}
    INCLUDEPATH += $${D2XX_INCLUDE}
    INCLUDEPATH += $${SRMIO_INCLUDE}
    INCLUDEPATH += $${ICAL_INCLUDE}
    INCLUDEPATH += $${LIBUSB_INCLUDE}
    INCLUDEPATH += $${USBXPRESS_INCLUDE}
    INCLUDEPATH += $${VLC_INCLUDE}
    INCLUDEPATH += $${SAMPLERATE_INCLUDE}
    # Include paths (adjusted from src.pro)
    INCLUDEPATH += . \
                $${app_path}/ANT \
                $${app_path}/Train \
                $${app_path}/FileIO \
                $${app_path}/Cloud \
                $${app_path}/Charts \
                $${app_path}/Metrics \
                $${app_path}/Gui \
                $${app_path}/Core \
                $${app_path}/Planning

    # External dependencies (adjust paths relative to here)
    INCLUDEPATH +=  $${app_path}/../qwt/src \
                    $${app_path}/../contrib/qxt/src \
                    $${app_path}/../contrib/qtsolutions/json \
                    $${app_path}/../contrib/qtsolutions/qwtcurve \
                    $${app_path}/../contrib/qtsolutions/flowlayout \
                    $${app_path}/../contrib/lmfit \
                    $${app_path}/../contrib/boost \
                    $${app_path}/../contrib/kmeans \
                    $${app_path}/../contrib/voronoi

    # Python Config
    DEFINES += SIP_STATIC_MODULE
    QT += core gui widgets core5compat xml sql network svg concurrent charts opengl webenginecore webenginewidgets webchannel positioning webenginequick

    INCLUDEPATH += $${app_path} ..

    # Source Generation

    # Hook to run data generation before compilation
    win32 {
        SYSTEM_MAKE = make
    } else {
        SYSTEM_MAKE = make
    }
    # We run make in the current directory to generate the SIP C++ files
    compile.commands = $${SYSTEM_MAKE} -f Makefile.SIP
    compile.depends = goldencheetah.sip
    QMAKE_EXTRA_TARGETS += compile
    PRE_TARGETDEPS += compile

    # Explicitly add the generated sources
    CPPSOURCES += $$files(sip*.cpp)
    CSOURCES += $$files(sip*.c)
    HEADERS += $$files(sip*.h)

    # If no SIP files file found (clean build), we might need to rely on the fact that
    # the user installs sip-build and runs it.
    # But let's try to trigger it.
    isEmpty(CPPSOURCES) || isEmpty(CSOURCES) || isEmpty(HEADERS) {
        message("Generating SIP bindings")
        system($${SYSTEM_MAKE} -f Makefile.SIP)
        # Re-glob after system call
    } else {
        message("SIP bindings already generated")
    }
    SOURCES += $$files(sip*.cpp)
    SOURCES += $$files(sip*.c)
    HEADERS += $$files(sip*.h)

    SOURCES += Bindings.cpp
}
