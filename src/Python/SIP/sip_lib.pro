TEMPLATE = lib
CONFIG += staticlib
CONFIG -= precompile_header
CONFIG += no_pch
TARGET = sip_lib

# Common settings
app_path = ../..
include($${app_path}/gcconfig.pri)

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
!isEmpty(PYTHONINCLUDES) {
    QMAKE_CXXFLAGS += $${PYTHONINCLUDES}
    QMAKE_CFLAGS += $${PYTHONINCLUDES}
}

# Source Generation
# We run make in the current directory to generate the SIP C++ files
win32 {
    SYSTEM_MAKE = $(MAKE)
} else {
    SYSTEM_MAKE = make
}

# Hook to run data generation before compilation
compile.commands = $${SYSTEM_MAKE} -f Makefile.sip
compile.depends = FORCE
QMAKE_EXTRA_TARGETS += compile
PRE_TARGETDEPS += compile

# Explicitly add the generated sources
# We use a wildcard, but we need to ensure they exist first.
# qmake expands wildcards at parse time, so this is tricky if they don't exist yet.
# However, SIP generation usually happens before qmake in many flows, or we rely on them being present.
# If they are not present, qmake might miss them.
# A better approach for the sources list:
SOURCES += Bindings.cpp
SOURCES += $$files(sip*.cpp)
HEADERS += $$files(sip*.h)

# If no SIP files file found (clean build), we might need to rely on the fact that
# the user installs sip-build and runs it. 
# But let's try to trigger it.
system($${SYSTEM_MAKE})

# Re-glob after system call
SOURCES += $$files(sip*.cpp)
HEADERS += $$files(sip*.h)
