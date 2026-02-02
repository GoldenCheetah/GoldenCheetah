###############################################################################
# Qwt Widget Library - CMakeLists.txt
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
###############################################################################

cmake_minimum_required(VERSION 3.16)

# Qwt version
set(QWT_VERSION_MAJOR 6)
set(QWT_VERSION_MINOR 2)
set(QWT_VERSION_PATCH 0)

# Find Qt (support both Qt5 and Qt6)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(STATUS "Qwt: Found Qt version: ${QT_VERSION}")

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg OpenGL OpenGLWidgets PrintSupport)

# Enable automatic MOC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(QWT_STATIC "Build static library" OFF)

# Platform-specific settings
if(WIN32)
    if(MSVC)
        add_compile_options(/W3)
    else()
        add_compile_options(-Wall -W)
    endif()
elseif(UNIX AND NOT APPLE)
    add_compile_options(-Wall -W)
elseif(APPLE)
    add_compile_options(-Wall -W)
endif()

# Automatically find source files
file(GLOB QWT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB QWT_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

# Remove unwanted files if any
list(FILTER QWT_SOURCES EXCLUDE REGEX ".*_yacc\\.c$")
list(FILTER QWT_SOURCES EXCLUDE REGEX ".*_lex\\.c$")

# Exclude legacy OpenGL canvas (uses QGLWidget which doesn't exist in Qt6)
# Use qwt_plot_opengl_canvas.cpp instead which uses QOpenGLWidget
list(FILTER QWT_SOURCES EXCLUDE REGEX ".*qwt_plot_glcanvas\\.cpp$")
list(FILTER QWT_HEADERS EXCLUDE REGEX ".*qwt_plot_glcanvas\\.h$")

# Create library
if(QWT_STATIC)
    add_library(qwt STATIC ${QWT_SOURCES} ${QWT_HEADERS})
else()
    add_library(qwt SHARED ${QWT_SOURCES} ${QWT_HEADERS})
endif()

# Set library properties
set_target_properties(qwt PROPERTIES
    VERSION "${QWT_VERSION_MAJOR}.${QWT_VERSION_MINOR}.${QWT_VERSION_PATCH}"
    SOVERSION ${QWT_VERSION_MAJOR}
    OUTPUT_NAME qwt
)

# Include directories
target_include_directories(qwt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link Qt libraries
target_link_libraries(qwt PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# Platform-specific
if(WIN32)
    target_link_libraries(qwt PUBLIC gdi32 user32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(qwt PUBLIC m)
endif()

message(STATUS "Qwt configuration complete")
message(STATUS "  Qt version: ${QT_VERSION}")
message(STATUS "  Source files: ${QWT_SOURCES.count}")
if(QWT_STATIC)
    message(STATUS "  Build type: Static")
else()
    message(STATUS "  Build type: Shared")
endif()
