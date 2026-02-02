cmake_minimum_required(VERSION 3.16)

project(GoldenCheetah VERSION 3.7 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard for C files
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable automatic MOC, UIC, and RCC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Policy settings
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

# Build options
option(GC_WANT_PYTHON "Enable Python scripting support" OFF)
option(GC_WANT_R "Enable R statistical computing support" OFF)
option(GC_WANT_HTTP "Enable HTTP API web services" ON)
option(GC_HAVE_ICAL "Enable iCal calendar support" OFF)
option(GC_HAVE_LIBUSB "Enable USB device support" OFF)
option(GC_HAVE_VLC "Enable VLC video playback" OFF)
option(GC_HAVE_SAMPLERATE "Enable sample rate conversion" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

# Find Qt (support both Qt5 and Qt6, prefer Qt6)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(STATUS "Found Qt version: ${QT_VERSION}")

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Widgets
    Xml
    Sql
    Network
    Svg
    Concurrent
    SerialPort
    Multimedia
    MultimediaWidgets
    WebEngineCore
    WebEngineWidgets
    WebChannel
    Positioning
    Bluetooth
    Charts
    OpenGL
    PrintSupport
)

# Qt6-specific modules
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS
        WebEngineQuick
        Core5Compat
    )
endif()

# Qt5-specific modules
if(QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS
        WebEngine
    )
    if(APPLE)
        find_package(Qt5 COMPONENTS MacExtras)
    endif()
endif()

# Find required dependencies
find_package(GSL REQUIRED)
find_package(ZLIB REQUIRED)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    if(MSVC)
        # Security hardening for MSVC
        add_compile_options(/GS /sdl /W3)
        add_compile_definitions(_FORTIFY_SOURCE=2 NOMINMAX)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DYNAMICBASE /NXCOMPAT")
    else()
        # MinGW
        add_compile_options(-Wno-unused-variable -Wno-sign-compare)
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    find_package(X11)
    
    # Security hardening for GCC/Clang
    add_compile_options(
        -fstack-protector-strong
        -Wno-unused-variable
        -Wno-sign-compare
    )
    add_compile_definitions(_FORTIFY_SOURCE=2)
    add_link_options(-Wl,-z,relro,-z,now)
    
    # Math library
    link_libraries(m)
elseif(APPLE)
    # macOS-specific settings
    add_compile_options(-fstack-protector-strong)
    
    # macOS frameworks
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    link_libraries(${IOKIT_FRAMEWORK} ${APPKIT_FRAMEWORK} objc)
    
    # Set deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")
endif()

# Global include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ANT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Train
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileIO
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Cloud
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Charts
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Metrics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Planning
    ${CMAKE_CURRENT_SOURCE_DIR}/qwt/src
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/qxt/src
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/qtsolutions/json
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/qtsolutions/qwtcurve
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/qtsolutions/flowlayout
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/qtsolutions/codeeditor
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/lmfit
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/kmeans
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/voronoi
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/httpserver
    ${CMAKE_CURRENT_SOURCE_DIR}/contrib/qzip
    ${GSL_INCLUDE_DIRS}
)

# Add subdirectories
add_subdirectory(qwt)
add_subdirectory(src)

# Optional: Add unit tests if enabled
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(unittests)
endif()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "GoldenCheetah")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "GoldenCheetah")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Performance analysis software for cyclists and triathletes")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
endif()

include(CPack)

# Print configuration summary
message(STATUS "===========================================")
message(STATUS "GoldenCheetah Configuration Summary")
message(STATUS "===========================================")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "-------------------------------------------")
message(STATUS "Optional Features:")
message(STATUS "  Python Support: ${GC_WANT_PYTHON}")
message(STATUS "  R Support: ${GC_WANT_R}")
message(STATUS "  HTTP API: ${GC_WANT_HTTP}")
message(STATUS "  iCal Support: ${GC_HAVE_ICAL}")
message(STATUS "  USB Support: ${GC_HAVE_LIBUSB}")
message(STATUS "  VLC Support: ${GC_HAVE_VLC}")
message(STATUS "  Unit Tests: ${BUILD_TESTS}")
message(STATUS "===========================================")
