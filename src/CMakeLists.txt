###############################################################################
# GoldenCheetah - Main Application CMakeLists.txt
# 
# This CMakeLists.txt builds the GoldenCheetah application with support for:
# - Qt5/Qt6 dual support
# - C++17 standard
# - All optional features (Python, R, USB, etc.)
# - Platform-specific configurations
###############################################################################

cmake_minimum_required(VERSION 3.16)

# Project information
project(GoldenCheetah VERSION 3.7 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable automatic Qt processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

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
option(GC_WANT_D2XX "Enable FTDI D2XX driver support" OFF)
option(GC_WANT_SRMIO "Enable SRMIO power meter support" OFF)
option(GC_WANT_USBXPRESS "Enable USBXpress ANT+ USB1 support" OFF)
option(GC_WANT_X11 "Enable X11 support on Linux" OFF)

# Find Qt (support both Qt5 and Qt6)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
message(STATUS "GoldenCheetah: Found Qt version: ${QT_VERSION}")

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
    find_package(Qt5 REQUIRED COMPONENTS WebEngine)
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
    
    # Windows resources
    set(GC_RC_FILE Resources/win32/windowsico.rc)
    
    # Windows-specific libraries
    set(GC_PLATFORM_LIBS ws2_32 gdi32 user32)
    
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    add_compile_options(
        -fstack-protector-strong
        -Wno-unused-variable
        -Wno-sign-compare
    )
    add_compile_definitions(_FORTIFY_SOURCE=2)
    add_link_options(-Wl,-z,relro,-z,now)
    
    # Math library
    link_libraries(m)
    
    # X11 support (optional)
    if(GC_WANT_X11)
        find_package(X11 REQUIRED)
        list(APPEND GC_PLATFORM_LIBS X11::X11)
    endif()
    
elseif(APPLE)
    # macOS-specific settings
    add_compile_options(-fstack-protector-strong)
    
    # macOS frameworks
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    list(APPEND GC_PLATFORM_LIBS ${IOKIT_FRAMEWORK} ${APPKIT_FRAMEWORK} objc)
    
    # Set deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")
endif()

# Global include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/ANT
    ${CMAKE_CURRENT_SOURCE_DIR}/Train
    ${CMAKE_CURRENT_SOURCE_DIR}/FileIO
    ${CMAKE_CURRENT_SOURCE_DIR}/Cloud
    ${CMAKE_CURRENT_SOURCE_DIR}/Charts
    ${CMAKE_CURRENT_SOURCE_DIR}/Metrics
    ${CMAKE_CURRENT_SOURCE_DIR}/Gui
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Planning
    ${CMAKE_CURRENT_SOURCE_DIR}/../qwt/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/qxt/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/qtsolutions/json
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/qtsolutions/qwtcurve
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/qtsolutions/flowlayout
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/qtsolutions/codeeditor
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/lmfit
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/boost
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/kmeans
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/voronoi
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/httpserver
    ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/qzip
    ${GSL_INCLUDE_DIRS}
)

# Define QT_QT_INCLUDE_DIR before using
include_directories(${QT_INCLUDE_DIR})

# ANT+ Source Files
set(GC_SOURCES_ANT
    ANT/ANTChannel.cpp
    ANT/ANT.cpp
    ANT/ANTlocalController.cpp
    ANT/ANTLogger.cpp
    ANT/ANTMessage.cpp
)

set(GC_HEADERS_ANT
    ANT/ANTChannel.h
    ANT/ANT.h
    ANT/ANTlocalController.h
    ANT/ANTLogger.h
    ANT/ANTMessage.h
    ANT/ANTMessages.h
)

# Charts and associated widgets
set(GC_SOURCES_CHARTS
    Charts/Aerolab.cpp
    Charts/AerolabWindow.cpp
    Charts/AllPlot.cpp
    Charts/AllPlotInterval.cpp
    Charts/AllPlotSlopeCurve.cpp
    Charts/AllPlotWindow.cpp
    Charts/BlankState.cpp
    Charts/ChartBar.cpp
    Charts/ChartSettings.cpp
    Charts/CPPlot.cpp
    Charts/CpPlotCurve.cpp
    Charts/CriticalPowerWindow.cpp
    Charts/DiaryWindow.cpp
    Charts/ExhaustionDialog.cpp
    Charts/GcOverlayWidget.cpp
    Charts/GcPane.cpp
    Charts/GoldenCheetah.cpp
    Charts/GenericAnnotations.cpp
    Charts/GenericChart.cpp
    Charts/GenericLegend.cpp
    Charts/GenericPlot.cpp
    Charts/GenericSelectTool.cpp
    Charts/HistogramWindow.cpp
    Charts/HrPwPlot.cpp
    Charts/HrPwWindow.cpp
    Charts/IndendPlotMarker.cpp
    Charts/IntervalSummaryWindow.cpp
    Charts/LogTimeScaleDraw.cpp
    Charts/LTMCanvasPicker.cpp
    Charts/LTMChartParser.cpp
    Charts/LTMOutliers.cpp
    Charts/LTMPlot.cpp
    Charts/LTMPopup.cpp
    Charts/LTMSettings.cpp
    Charts/LTMTool.cpp
    Charts/LTMTrend.cpp
    Charts/LTMWindow.cpp
    Charts/MetadataWindow.cpp
    Charts/MUPlot.cpp
    Charts/MUWidget.cpp
    Charts/Overview.cpp
    Charts/OverviewItems.cpp
    Charts/PfPvPlot.cpp
    Charts/PfPvWindow.cpp
    Charts/PowerHist.cpp
    Charts/ReferenceLineDialog.cpp
    Charts/RideEditor.cpp
    Charts/RideMapWindow.cpp
    # Charts/RouteWindow.cpp  # Disabled - has refactoring issues (missing getRides(), protected routes, etc.)
    Charts/ScatterPlot.cpp
    Charts/ScatterWindow.cpp
    Charts/SmallPlot.cpp
    Charts/TreeMapPlot.cpp
    Charts/TreeMapWindow.cpp
    Charts/UserChart.cpp
    Charts/UserChartData.cpp
    Charts/UserChartOverviewItem.cpp
    Charts/UserChartWindow.cpp
)

set(GC_HEADERS_CHARTS
    Charts/Aerolab.h
    Charts/AerolabWindow.h
    Charts/AllPlot.h
    Charts/AllPlotInterval.h
    Charts/AllPlotSlopeCurve.h
    Charts/AllPlotWindow.h
    Charts/BlankState.h
    Charts/ChartBar.h
    Charts/ChartSettings.h
    Charts/CpPlotCurve.h
    Charts/CPPlot.h
    Charts/CriticalPowerWindow.h
    Charts/DaysScaleDraw.h
    Charts/DiaryWindow.h
    Charts/ExhaustionDialog.h
    Charts/GcOverlayWidget.h
    Charts/GcPane.h
    Charts/GoldenCheetah.h
    Charts/GenericAnnotations.h
    Charts/GenericChart.h
    Charts/GenericLegend.h
    Charts/GenericPlot.h
    Charts/GenericSelectTool.h
    Charts/HistogramWindow.h
    Charts/HrPwPlot.h
    Charts/HrPwWindow.h
    Charts/IndendPlotMarker.h
    Charts/IntervalSummaryWindow.h
    Charts/LogTimeScaleDraw.h
    Charts/LTMCanvasPicker.h
    Charts/LTMChartParser.h
    Charts/LTMOutliers.h
    Charts/LTMPlot.h
    Charts/LTMPopup.h
    Charts/LTMSettings.h
    Charts/LTMTool.h
    Charts/LTMTrend.h
    Charts/LTMTrend2.h
    Charts/LTMWindow.h
    Charts/MetadataWindow.h
    Charts/MUPlot.h
    Charts/MUPool.h
    Charts/MUWidget.h
    Charts/Overview.h
    Charts/OverviewItems.h
    Charts/PfPvPlot.h
    Charts/PfPvWindow.h
    Charts/PowerHist.h
    Charts/ReferenceLineDialog.h
    Charts/RideEditor.h
    Charts/RideMapWindow.h
    # Charts/RouteWindow.h  # Disabled - has refactoring issues
    Charts/ScatterPlot.h
    Charts/ScatterWindow.h
    Charts/SmallPlot.h
    Charts/TreeMapPlot.h
    Charts/TreeMapWindow.h
    Charts/UserChart.h
    Charts/UserChartData.h
    Charts/UserChartOverviewItem.h
    Charts/UserChartWindow.h
    Charts/ZoneScaleDraw.h
)

# Cloud services
set(GC_SOURCES_CLOUD
    Cloud/CalendarDownload.cpp
    Cloud/CloudService.cpp
    Cloud/LocalFileStore.cpp
    Cloud/OAuthDialog.cpp
    Cloud/WithingsDownload.cpp
    Cloud/Strava.cpp
    Cloud/CyclingAnalytics.cpp
    Cloud/RideWithGPS.cpp
    Cloud/TrainingsTageBuch.cpp
    Cloud/Selfloops.cpp
    Cloud/Velohero.cpp
    Cloud/SportsPlusHealth.cpp
    Cloud/AddCloudWizard.cpp
    Cloud/Withings.cpp
    Cloud/MeasuresDownload.cpp
    Cloud/Xert.cpp
    Cloud/Azum.cpp
    Cloud/Dropbox.cpp
    Cloud/OpenData.cpp
    Cloud/SixCycle.cpp
    Cloud/PolarFlow.cpp
    Cloud/SportTracks.cpp
    Cloud/Nolio.cpp
)

set(GC_HEADERS_CLOUD
    Cloud/CalendarDownload.h
    Cloud/CloudService.h
    Cloud/LocalFileStore.h
    Cloud/OAuthDialog.h
    Cloud/WithingsDownload.h
    Cloud/Strava.h
    Cloud/CyclingAnalytics.h
    Cloud/RideWithGPS.h
    Cloud/TrainingsTageBuch.h
    Cloud/Selfloops.h
    Cloud/Velohero.h
    Cloud/SportsPlusHealth.h
    Cloud/AddCloudWizard.h
    Cloud/Withings.h
    Cloud/MeasuresDownload.h
    Cloud/Xert.h
    Cloud/Azum.h
    Cloud/Dropbox.h
    Cloud/OpenData.h
    Cloud/SixCycle.h
    Cloud/PolarFlow.h
    Cloud/SportTracks.h
    Cloud/Nolio.h
)

# Core data structures
set(GC_SOURCES_CORE
    Core/Athlete.cpp
    Core/Context.cpp
    Core/DataFilter.cpp
    Core/FreeSearch.cpp
    Core/GcUpgrade.cpp
    Core/IdleTimer.cpp
    Core/IntervalItem.cpp
    Core/main.cpp
    Core/NamedSearch.cpp
    Core/RideCache.cpp
    Core/RideCacheModel.cpp
    Core/RideItem.cpp
    Core/Route.cpp
    Core/RouteParser.cpp
    Core/Season.cpp
    Core/SeasonDialogs.cpp
    Core/Seasons.cpp
    Core/Settings.cpp
    Core/Specification.cpp
    Core/TimeUtils.cpp
    Core/Units.cpp
    Core/UserData.cpp
    Core/Utils.cpp
    Core/Measures.cpp
    Core/Quadtree.cpp
    Core/SplineLookup.cpp
)

set(GC_HEADERS_CORE
    Core/Athlete.h
    Core/Context.h
    Core/DataFilter.h
    Core/FreeSearch.h
    Core/GcCalendarModel.h
    Core/GcUpgrade.h
    Core/IdleTimer.h
    Core/IntervalItem.h
    Core/NamedSearch.h
    Core/RideCache.h
    Core/RideCacheModel.h
    Core/RideDB.h
    Core/RideItem.h
    Core/Route.h
    Core/RouteParser.h
    Core/Season.h
    Core/SeasonDialogs.h
    Core/Seasons.h
    Core/Secrets.h
    Core/Settings.h
    Core/Specification.h
    Core/TimeUtils.h
    Core/Units.h
    Core/UserData.h
    Core/Utils.h
    Core/Measures.h
    Core/Quadtree.h
    Core/SplineLookup.h
)

# File and Device IO
set(GC_SOURCES_FILEIO
    FileIO/ArchiveFile.cpp
    FileIO/AthleteBackup.cpp
    FileIO/Bin2RideFile.cpp
    FileIO/BinRideFile.cpp
    FileIO/CommPort.cpp
    FileIO/Computrainer3dpFile.cpp
    FileIO/CsvRideFile.cpp
    FileIO/DataProcessor.cpp
    FileIO/Device.cpp
    FileIO/FitlogParser.cpp
    FileIO/FitlogRideFile.cpp
    FileIO/FitRideFile.cpp
    FileIO/FixAeroPod.cpp
    FileIO/FixDeriveDistance.cpp
    FileIO/FixDeriveHeadwind.cpp
    FileIO/FixDerivePower.cpp
    FileIO/FixDeriveTorque.cpp
    FileIO/FixElevation.cpp
    FileIO/FixLapSwim.cpp
    FileIO/FixFreewheeling.cpp
    FileIO/FixGaps.cpp
    FileIO/FixGPS.cpp
    FileIO/FixRunningCadence.cpp
    FileIO/FixRunningPower.cpp
    FileIO/FixHRSpikes.cpp
    FileIO/FixMoxy.cpp
    FileIO/FixPower.cpp
    FileIO/FixSmO2.cpp
    FileIO/FixSpeed.cpp
    FileIO/FixSpikes.cpp
    FileIO/FixTorque.cpp
    FileIO/GcRideFile.cpp
    FileIO/GpxParser.cpp
    FileIO/GpxRideFile.cpp
    FileIO/JouleDevice.cpp
    FileIO/LapsEditor.cpp
    FileIO/MacroDevice.cpp
    FileIO/ManualRideFile.cpp
    FileIO/MoxyDevice.cpp
    FileIO/PolarRideFile.cpp
    FileIO/PowerTapDevice.cpp
    FileIO/PowerTapUtil.cpp
    FileIO/PwxRideFile.cpp
    FileIO/QuarqParser.cpp
    FileIO/QuarqRideFile.cpp
    FileIO/RawRideFile.cpp
    FileIO/RideAutoImportConfig.cpp
    FileIO/RideFileCache.cpp
    FileIO/RideFileCommand.cpp
    FileIO/RideFile.cpp
    FileIO/RideFileTableModel.cpp
    FileIO/Serial.cpp
    FileIO/SlfParser.cpp
    FileIO/SlfRideFile.cpp
    FileIO/SmfParser.cpp
    FileIO/SmfRideFile.cpp
    FileIO/SmlParser.cpp
    FileIO/SmlRideFile.cpp
    FileIO/Snippets.cpp
    FileIO/SrdRideFile.cpp
    FileIO/SrmRideFile.cpp
    FileIO/SyncRideFile.cpp
    FileIO/TacxCafRideFile.cpp
    FileIO/TcxParser.cpp
    FileIO/TcxRideFile.cpp
    FileIO/TxtRideFile.cpp
    FileIO/WkoRideFile.cpp
    FileIO/XDataDialog.cpp
    FileIO/XDataTableModel.cpp
    FileIO/FilterHRV.cpp
    FileIO/MeasuresCsvImport.cpp
    FileIO/LocationInterpolation.cpp
    FileIO/TTSReader.cpp
    FileIO/EpmRideFile.cpp
    FileIO/EpmParser.cpp
)

set(GC_HEADERS_FILEIO
    FileIO/ArchiveFile.h
    FileIO/AthleteBackup.h
    FileIO/Bin2RideFile.h
    FileIO/BinRideFile.h
    FileIO/CommPort.h
    FileIO/Computrainer3dpFile.h
    FileIO/CsvRideFile.h
    FileIO/DataProcessor.h
    FileIO/Device.h
    FileIO/FitlogParser.h
    FileIO/FitlogRideFile.h
    FileIO/FitRideFile.h
    FileIO/GcRideFile.h
    FileIO/GpxParser.h
    FileIO/GpxRideFile.h
    FileIO/JouleDevice.h
    FileIO/JsonRideFile.h
    FileIO/LapsEditor.h
    FileIO/MacroDevice.h
    FileIO/ManualRideFile.h
    FileIO/MoxyDevice.h
    FileIO/PolarRideFile.h
    FileIO/PowerTapDevice.h
    FileIO/PowerTapUtil.h
    FileIO/PwxRideFile.h
    FileIO/QuarqParser.h
    FileIO/QuarqRideFile.h
    FileIO/RawRideFile.h
    FileIO/RideAutoImportConfig.h
    FileIO/RideFileCache.h
    FileIO/RideFileCommand.h
    FileIO/RideFile.h
    FileIO/RideFileTableModel.h
    FileIO/Serial.h
    FileIO/SlfParser.h
    FileIO/SlfRideFile.h
    FileIO/SmfParser.h
    FileIO/SmfRideFile.h
    FileIO/SmlParser.h
    FileIO/SmlRideFile.h
    FileIO/SrdRideFile.h
    FileIO/SrmRideFile.h
    FileIO/SyncRideFile.h
    FileIO/TcxParser.h
    FileIO/TcxRideFile.h
    FileIO/TxtRideFile.h
    FileIO/WkoRideFile.h
    FileIO/XDataDialog.h
    FileIO/XDataTableModel.h
    FileIO/FilterHRV.h
    FileIO/MeasuresCsvImport.h
    FileIO/LocationInterpolation.h
    FileIO/TTSReader.h
    FileIO/EpmParser.h
    FileIO/EpmRideFile.h
)

# GUI Elements
set(GC_SOURCES_GUI
    Gui/AboutDialog.cpp
    Gui/AddIntervalDialog.cpp
    Gui/AnalysisSidebar.cpp
    Gui/ChooseCyclistDialog.cpp
    Gui/ColorButton.cpp
    Gui/Colors.cpp
    Gui/CompareDateRange.cpp
    Gui/CompareInterval.cpp
    Gui/ComparePane.cpp
    Gui/ConfigDialog.cpp
    Gui/DiarySidebar.cpp
    Gui/DragBar.cpp
    Gui/EstimateCPDialog.cpp
    Gui/GcCrashDialog.cpp
    Gui/GcSideBarItem.cpp
    Gui/GcToolBar.cpp
    Gui/GcWindowLayout.cpp
    Gui/GcWindowRegistry.cpp
    Gui/GenerateHeatMapDialog.cpp
    Gui/HelpWhatsThis.cpp
    Gui/HelpWindow.cpp
    Gui/IntervalTreeView.cpp
    Gui/LTMSidebar.cpp
    Gui/MainWindow.cpp
    Gui/NewAthleteWizard.cpp
    Gui/Pages.cpp
    Gui/RideNavigator.cpp
    Gui/SaveDialogs.cpp
    Gui/SearchBox.cpp
    Gui/SearchFilterBox.cpp
    Gui/SolveCPDialog.cpp
    Gui/AthleteTab.cpp
    Gui/AbstractView.cpp
    Gui/ToolsRhoEstimator.cpp
    Gui/Views.cpp
    Gui/BatchProcessingDialog.cpp
    Gui/DownloadRideDialog.cpp
    Gui/ManualActivityWizard.cpp
    Gui/EditUserMetricDialog.cpp
    Gui/NewSideBar.cpp
    Gui/MergeActivityWizard.cpp
    Gui/RideImportWizard.cpp
    Gui/SplitActivityWizard.cpp
    Gui/SolverDisplay.cpp
    Gui/MetricSelect.cpp
    Gui/AddTileWizard.cpp
    Gui/NavigationModel.cpp
    Gui/AthleteView.cpp
    Gui/ChartSpace.cpp
    Gui/AthleteConfigDialog.cpp
    Gui/AthletePages.cpp
    Gui/Perspective.cpp
    Gui/PerspectiveDialog.cpp
    Gui/SplashScreen.cpp
    Gui/StyledItemDelegates.cpp
    Gui/MetadataDialog.cpp
    Gui/ActionButtonBox.cpp
    Gui/MetricOverrideDialog.cpp
    Gui/QTFullScreen.cpp
)

set(GC_HEADERS_GUI
    Gui/AboutDialog.h
    Gui/AddIntervalDialog.h
    Gui/AnalysisSidebar.h
    Gui/ChooseCyclistDialog.h
    Gui/ColorButton.h
    Gui/Colors.h
    Gui/CompareDateRange.h
    Gui/CompareInterval.h
    Gui/ComparePane.h
    Gui/ConfigDialog.h
    Gui/DiarySidebar.h
    Gui/DragBar.h
    Gui/EstimateCPDialog.h
    Gui/GcCrashDialog.h
    Gui/GcSideBarItem.h
    Gui/GcToolBar.h
    Gui/GcWindowLayout.h
    Gui/GcWindowRegistry.h
    Gui/GenerateHeatMapDialog.h
    Gui/HelpWhatsThis.h
    Gui/HelpWindow.h
    Gui/IntervalTreeView.h
    Gui/LTMSidebar.h
    Gui/MainWindow.h
    Gui/NewAthleteWizard.h
    Gui/Pages.h
    Gui/RideNavigator.h
    Gui/RideNavigatorProxy.h
    Gui/SaveDialogs.h
    Gui/SearchBox.h
    Gui/SearchFilterBox.h
    Gui/SolveCPDialog.h
    Gui/AthleteTab.h
    Gui/AbstractView.h
    Gui/ToolsRhoEstimator.h
    Gui/Views.h
    Gui/BatchProcessingDialog.h
    Gui/DownloadRideDialog.h
    Gui/ManualActivityWizard.h
    Gui/NewSideBar.h
    Gui/MergeActivityWizard.h
    Gui/RideImportWizard.h
    Gui/SplitActivityWizard.h
    Gui/SolverDisplay.h
    Gui/MetricSelect.h
    Gui/AddTileWizard.h
    Gui/NavigationModel.h
    Gui/AthleteView.h
    Gui/ChartSpace.h
    Gui/AthleteConfigDialog.h
    Gui/AthletePages.h
    Gui/Perspective.h
    Gui/PerspectiveDialog.h
    Gui/SplashScreen.h
    Gui/StyledItemDelegates.h
    Gui/MetadataDialog.h
    Gui/ActionButtonBox.h
    Gui/MetricOverrideDialog.h
    Gui/QTFullScreen.h
)

# Metrics and Models
set(GC_SOURCES_METRICS
    Metrics/aBikeScore.cpp
    Metrics/aCoggan.cpp
    Metrics/AerobicDecoupling.cpp
    Metrics/Banister.cpp
    Metrics/BasicRideMetrics.cpp
    Metrics/BikeScore.cpp
    Metrics/Coggan.cpp
    Metrics/CPSolver.cpp
    Metrics/DanielsPoints.cpp
    Metrics/Estimator.cpp
    Metrics/ExtendedCriticalPower.cpp
    Metrics/GOVSS.cpp
    Metrics/HrTimeInZone.cpp
    Metrics/HrZones.cpp
    Metrics/LeftRightBalance.cpp
    Metrics/PaceTimeInZone.cpp
    Metrics/PaceZones.cpp
    Metrics/PDModel.cpp
    Metrics/PeakPace.cpp
    Metrics/PeakPower.cpp
    Metrics/PeakHr.cpp
    Metrics/PMCData.cpp
    Metrics/PowerProfile.cpp
    Metrics/RideMetadata.cpp
    Metrics/RideMetric.cpp
    Metrics/RunMetrics.cpp
    Metrics/SwimMetrics.cpp
    Metrics/SpecialFields.cpp
    Metrics/Statistic.cpp
    Metrics/SustainMetric.cpp
    Metrics/SwimScore.cpp
    Metrics/TimeInZone.cpp
    Metrics/TRIMPPoints.cpp
    Metrics/UserMetric.cpp
    Metrics/UserMetricParser.cpp
    Metrics/VDOTCalculator.cpp
    Metrics/VDOT.cpp
    Metrics/WattsPerKilogram.cpp
    Metrics/WPrime.cpp
    Metrics/Zones.cpp
    Metrics/HrvMetrics.cpp
    Metrics/BlinnSolver.cpp
    Metrics/RowMetrics.cpp
    Metrics/FastKmeans.cpp
)

set(GC_HEADERS_METRICS
    Metrics/Banister.h
    Metrics/CPSolver.h
    Metrics/Estimator.h
    Metrics/ExtendedCriticalPower.h
    Metrics/HrZones.h
    Metrics/PaceZones.h
    Metrics/PDModel.h
    Metrics/PMCData.h
    Metrics/PowerProfile.h
    Metrics/RideMetadata.h
    Metrics/RideMetric.h
    Metrics/SpecialFields.h
    Metrics/Statistic.h
    Metrics/UserMetricParser.h
    Metrics/UserMetricSettings.h
    Metrics/VDOTCalculator.h
    Metrics/WPrime.h
    Metrics/Zones.h
    Metrics/BlinnSolver.h
    Metrics/FastKmeans.h
)

# Planning
set(GC_SOURCES_PLANNING
    Planning/PlanningWindow.cpp
)

set(GC_HEADERS_PLANNING
    Planning/PlanningWindow.h
)

# Train View Components
set(GC_SOURCES_TRAIN
    Train/AddDeviceWizard.cpp
    Train/BT40Controller.cpp
    Train/BT40Device.cpp
    Train/CalibrationData.cpp
    Train/ComputrainerController.cpp
    Train/Computrainer.cpp
    Train/DaumController.cpp
    Train/Daum.cpp
    Train/DeviceConfiguration.cpp
    Train/DeviceTypes.cpp
    Train/DialWindow.cpp
    Train/ErgofitController.cpp
    Train/Ergofit.cpp
    Train/ErgofitConnection.cpp
    Train/TrainerDay.cpp
    Train/TrainerDayDownloadDialog.cpp
    Train/ErgFile.cpp
    Train/ErgFilePlot.cpp
    Train/Ftms.cpp
    Train/GarminServiceHelper.cpp
    Train/PhysicsUtility.cpp
    Train/BicycleSim.cpp
    Train/PolynomialRegression.cpp
    Train/StravaRoutesDownload.cpp
    Train/VideoSyncFileBase.cpp
    Train/ErgFileBase.cpp
    Train/ModelFilter.cpp
    Train/MultiFilterProxyModel.cpp
    Train/WorkoutFilter.cpp
    Train/FilterEditor.cpp
    Train/WorkoutFilterBox.cpp
    Train/TagBar.cpp
    Train/TagWidget.cpp
    Train/TrainerDayAPIQuery.cpp
    Train/TrainerDayAPIDialog.cpp
    Train/ElevationChartWindow.cpp
    Train/TrainBottom.cpp
    Train/TrainDB.cpp
    Train/TrainSidebar.cpp
    Train/VideoLayoutParser.cpp
    Train/VideoSyncFile.cpp
    Train/WorkoutPlotWindow.cpp
    Train/WebPageWindow.cpp
    Train/WorkoutWidget.cpp
    Train/WorkoutWidgetItems.cpp
    Train/WorkoutWindow.cpp
    Train/WorkoutWizard.cpp
    Train/ZwoParser.cpp
    Train/LiveMapWebPageWindow.cpp
    Train/ScalingLabel.cpp
    Train/InfoWidget.cpp
    Train/PowerInfoWidget.cpp
    Train/PowerZonesWidget.cpp
    Train/RatingWidget.cpp
    Train/ErgOverview.cpp
    Train/Shy.cpp
    Train/WorkoutTagWrapper.cpp
    Train/WorkoutMenuProvider.cpp
    # Train/Imagic.cpp  # Requires GC_HAVE_LIBUSB
    Train/KettlerController.cpp
    Train/Kettler.cpp
    Train/KettlerConnection.cpp
    Train/KettlerRacerController.cpp
    Train/KettlerRacer.cpp
    Train/KettlerRacerConnection.cpp
    Train/KurtInRide.cpp
    Train/KurtSmartControl.cpp
    Train/Library.cpp
    Train/LibraryParser.cpp
    Train/MeterWidget.cpp
    Train/NullController.cpp
    Train/RealtimeController.cpp
    Train/RealtimeData.cpp
    Train/RealtimePlot.cpp
    Train/RealtimePlotWindow.cpp
    Train/RemoteControl.cpp
    Train/SpinScanPlot.cpp
    Train/SpinScanPlotWindow.cpp
    Train/SpinScanPolarPlot.cpp
    Train/VideoWindow.cpp
    Train/VMProConfigurator.cpp
    Train/VMProWidget.cpp
    Train/MonarkController.cpp
    Train/MonarkConnection.cpp
)

set(GC_HEADERS_TRAIN
    Train/AddDeviceWizard.h
    Train/BT40Controller.h
    Train/BT40Device.h
    Train/CalibrationData.h
    Train/ComputrainerController.h
    Train/Computrainer.h
    Train/DaumController.h
    Train/Daum.h
    Train/DeviceConfiguration.h
    Train/DeviceTypes.h
    Train/DialWindow.h
    Train/ErgofitController.h
    Train/Ergofit.h
    Train/ErgofitConnection.h
    Train/TrainerDayDownloadDialog.h
    Train/TrainerDay.h
    Train/ErgFile.h
    Train/ErgFilePlot.h
    Train/Ftms.h
    Train/GarminServiceHelper.h
    Train/PhysicsUtility.h
    Train/BicycleSim.h
    Train/PolynomialRegression.h
    Train/StravaRoutesDownload.h
    Train/VideoSyncFileBase.h
    Train/ErgFileBase.h
    Train/ModelFilter.h
    Train/MultiFilterProxyModel.h
    Train/WorkoutFilter.h
    Train/FilterEditor.h
    Train/WorkoutFilterBox.h
    Train/TagBar.h
    Train/Taggable.h
    Train/TagStore.h
    Train/TagWidget.h
    Train/TrainerDayAPIQuery.h
    Train/TrainerDayAPIDialog.h
    Train/ElevationChartWindow.h
    Train/TrainBottom.h
    Train/TrainDB.h
    Train/TrainSidebar.h
    Train/VideoLayoutParser.h
    Train/VideoSyncFile.h
    Train/WorkoutPlotWindow.h
    Train/WebPageWindow.h
    Train/WorkoutWidget.h
    Train/WorkoutWidgetItems.h
    Train/WorkoutWindow.h
    Train/WorkoutWizard.h
    Train/ZwoParser.h
    Train/LiveMapWebPageWindow.h
    Train/ScalingLabel.h
    Train/InfoWidget.h
    Train/PowerInfoWidget.h
    Train/PowerZonesWidget.h
    Train/RatingWidget.h
    Train/ErgOverview.h
    Train/Shy.h
    Train/WorkoutTagWrapper.h
    Train/WorkoutMenuProvider.h
    # Train/Imagic.h  # Requires GC_HAVE_LIBUSB
    Train/KettlerController.h
    Train/Kettler.h
    Train/KettlerConnection.h
    Train/KettlerRacerController.h
    Train/KettlerRacer.h
    Train/KettlerRacerConnection.h
    Train/KurtInRide.h
    Train/KurtSmartControl.h
    Train/Library.h
    Train/LibraryParser.h
    Train/MeterWidget.h
    Train/NullController.h
    Train/RealtimeController.h
    Train/RealtimeData.h
    Train/RealtimePlot.h
    Train/RealtimePlotWindow.h
    Train/RemoteControl.h
    Train/SpinScanPlot.h
    Train/SpinScanPlotWindow.h
    Train/SpinScanPolarPlot.h
    Train/VideoWindow.h
    Train/VMProConfigurator.h
    Train/VMProWidget.h
    Train/MonarkController.h
    Train/MonarkConnection.h
    Train/Integrator.h
    Train/MenuProvider.h
    Train/MultiRegressionizer.h
)

# Contributed solutions
set(GC_SOURCES_CONTRIB
    ../contrib/qtsolutions/codeeditor/codeeditor.cpp
    ../contrib/qtsolutions/json/mvjson.cpp
    ../contrib/qtsolutions/flowlayout/flowlayout.cpp
    ../contrib/qtsolutions/qwtcurve/qwt_plot_gapped_curve.cpp
    ../contrib/qxt/src/qxtspanslider.cpp
    ../contrib/qxt/src/qxtstringspinbox.cpp
    ../contrib/qzip/zip.cpp
    ../contrib/lmfit/lmcurve.c
    ../contrib/lmfit/lmmin.c
    ../contrib/kmeans/kmeans_dataset.cpp
    ../contrib/kmeans/kmeans_general_functions.cpp
    ../contrib/kmeans/hamerly_kmeans.cpp
    ../contrib/kmeans/kmeans.cpp
    ../contrib/kmeans/original_space_kmeans.cpp
    ../contrib/kmeans/triangle_inequality_base_kmeans.cpp
    ../contrib/voronoi/Voronoi.cpp
)

set(GC_HEADERS_CONTRIB
    ../contrib/qtsolutions/codeeditor/codeeditor.h
    ../contrib/qtsolutions/json/mvjson.h
    ../contrib/qtsolutions/flowlayout/flowlayout.h
    ../contrib/qtsolutions/qwtcurve/qwt_plot_gapped_curve.h
    ../contrib/qxt/src/qxtspanslider.h
    ../contrib/qxt/src/qxtspanslider_p.h
    ../contrib/qxt/src/qxtstringspinbox.h
    ../contrib/qzip/zipreader.h
    ../contrib/qzip/zipwriter.h
    ../contrib/lmfit/lmcurve.h
    ../contrib/lmfit/lmcurve_tyd.h
    ../contrib/lmfit/lmmin.h
    ../contrib/lmfit/lmstruct.h
    ../contrib/boost/GeometricTools_BSplineCurve.h
    ../contrib/kmeans/kmeans_dataset.h
    ../contrib/kmeans/kmeans_general_functions.h
    ../contrib/kmeans/hamerly_kmeans.h
    ../contrib/kmeans/kmeans.h
    ../contrib/kmeans/original_space_kmeans.h
    ../contrib/kmeans/triangle_inequality_base_kmeans.h
    ../contrib/voronoi/Voronoi.h
)

# HTTP API Web Services
if(GC_WANT_HTTP)
    set(GC_SOURCES_HTTP
        Core/APIWebService.cpp
        ../contrib/httpserver/httpglobal.cpp
        ../contrib/httpserver/httplistener.cpp
        ../contrib/httpserver/httpconnectionhandler.cpp
        ../contrib/httpserver/httpconnectionhandlerpool.cpp
        ../contrib/httpserver/httprequest.cpp
        ../contrib/httpserver/httpresponse.cpp
        ../contrib/httpserver/httpcookie.cpp
        ../contrib/httpserver/httprequesthandler.cpp
        ../contrib/httpserver/httpsession.cpp
        ../contrib/httpserver/httpsessionstore.cpp
        ../contrib/httpserver/staticfilecontroller.cpp
    )
    
    set(GC_HEADERS_HTTP
        Core/APIWebService.h
        ../contrib/httpserver/httpglobal.h
        ../contrib/httpserver/httplistener.h
        ../contrib/httpserver/httpconnectionhandler.h
        ../contrib/httpserver/httpconnectionhandlerpool.h
        ../contrib/httpserver/httprequest.h
        ../contrib/httpserver/httpresponse.h
        ../contrib/httpserver/httpcookie.h
        ../contrib/httpserver/httprequesthandler.h
        ../contrib/httpserver/httpsession.h
        ../contrib/httpserver/httpsessionstore.h
        ../contrib/httpserver/staticfilecontroller.h
    )
    
    add_definitions(-DGC_WANT_HTTP)
endif()

# Python Support
if(GC_WANT_PYTHON)
    set(GC_SOURCES_PYTHON
        Python/PythonEmbed.cpp
        Charts/PythonChart.cpp
        Python/PythonSyntax.cpp
        Python/SIP/Bindings.cpp
        Python/SIP/sipgoldencheetahBindings.cpp
        Python/SIP/sipgoldencheetahcmodule.cpp
        Python/SIP/sipgoldencheetahQString.cpp
        Python/SIP/sipgoldencheetahQStringList.cpp
        Python/SIP/sipgoldencheetahPythonDataSeries.cpp
        Python/SIP/sipgoldencheetahPythonXDataSeries.cpp
        FileIO/FixPyScriptsDialog.cpp
        FileIO/FixPySettings.cpp
        FileIO/FixPyRunner.cpp
        FileIO/FixPyDataProcessor.cpp
    )
    
    set(GC_HEADERS_PYTHON
        Python/PythonEmbed.h
        Charts/PythonChart.h
        Python/PythonSyntax.h
        FileIO/FixPyScriptsDialog.h
        FileIO/FixPySettings.h
        FileIO/FixPyRunner.h
        FileIO/FixPyScript.h
        FileIO/FixPyDataProcessor.h
    )
    
    add_definitions(-DGC_HAVE_PYTHON)
endif()

# R Support
if(GC_WANT_R)
    set(GC_SOURCES_R
        R/REmbed.cpp
        R/RTool.cpp
        R/RGraphicsDevice.cpp
        R/RSyntax.cpp
        R/RLibrary.cpp
        Charts/RChart.cpp
        Charts/RCanvas.cpp
    )
    
    set(GC_HEADERS_R
        R/REmbed.h
        R/RTool.h
        R/RGraphicsDevice.h
        R/RSyntax.h
        R/RLibrary.h
        Charts/RChart.h
        Charts/RCanvas.h
    )
    
    add_definitions(-DSTRICT_R_HEADERS)
endif()

# iCal Support
if(GC_HAVE_ICAL)
    set(GC_SOURCES_ICAL
        Core/ICalendar.cpp
        Charts/DiaryWindow.cpp
        Cloud/CalDAV.cpp
        Cloud/CalDAVCloud.cpp
    )
    
    set(GC_HEADERS_ICAL
        Core/ICalendar.h
        Charts/DiaryWindow.h
        Cloud/CalDAV.h
        Cloud/CalDAVCloud.h
    )
    
    add_definitions(-DGC_HAVE_ICAL)
endif()

# libusb Support
if(GC_HAVE_LIBUSB)
    set(GC_SOURCES_USB
        Train/LibUsb.cpp
        Train/Fortius.cpp
        Train/FortiusController.cpp
        Train/Imagic.cpp
        Train/ImagicController.cpp
    )
    
    set(GC_HEADERS_USB
        Train/LibUsb.h
        Train/Fortius.h
        Train/FortiusController.h
        Train/Imagic.h
        Train/ImagicController.h
    )
    
    add_definitions(-DGC_HAVE_LIBUSB)
    
    if(NOT LIBUSB_V_1)
        set(GC_SOURCES_USB ${GC_SOURCES_USB} Train/EzUsb.c)
        set(GC_HEADERS_USB ${GC_HEADERS_USB} Train/EzUsb.h)
    else()
        add_definitions(-DLIBUSB_V_1)
        set(GC_SOURCES_USB ${GC_SOURCES_USB} Train/EzUsb-1.0.c)
        set(GC_HEADERS_USB ${GC_HEADERS_USB} Train/EzUsb-1.0.h)
    endif()
endif()

# VLC Support
if(GC_HAVE_VLC)
    add_definitions(-DGC_HAVE_VLC)
endif()

# Samplerate Support
if(GC_HAVE_SAMPLERATE)
    add_definitions(-DGC_HAVE_SAMPLERATE)
endif()

# D2XX Support
if(GC_WANT_D2XX)
    set(GC_SOURCES_D2XX FileIO/D2XX.cpp)
    set(GC_HEADERS_D2XX FileIO/D2XX.h)
    add_definitions(-DGC_HAVE_D2XX)
endif()

# SRMIO Support
if(GC_WANT_SRMIO)
    set(GC_SOURCES_SRMIO FileIO/SrmDevice.cpp)
    set(GC_HEADERS_SRMIO FileIO/SrmDevice.h)
    add_definitions(-DGC_HAVE_SRMIO)
endif()

# USBXPRESS Support
if(GC_WANT_USBXPRESS)
    set(GC_SOURCES_USBXPRESS Train/USBXpress.cpp)
    set(GC_HEADERS_USBXPRESS Train/USBXpress.h)
    add_definitions(-DGC_HAVE_USBXPRESS)
endif()

# Generate parser files using bison and flex
find_program(BISON_EXECUTABLE bison)
find_program(FLEX_EXECUTABLE flex)

# Get the main binary directory (parent of src/)
get_filename_component(GC_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/.." ABSOLUTE)

if(BISON_EXECUTABLE AND FLEX_EXECUTABLE)
    message(STATUS "Found bison: ${BISON_EXECUTABLE}")
    message(STATUS "Found flex: ${FLEX_EXECUTABLE}")
    message(STATUS "Using binary directory: ${GC_BINARY_DIR}")
    
    # Generate DataFilter parser - bison 3.8+ outputs .hpp instead of .tab.h
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/DataFilter_yacc.cpp ${GC_BINARY_DIR}/DataFilter_yacc.h
        COMMAND ${BISON_EXECUTABLE} -d -p DataFilter -o ${GC_BINARY_DIR}/DataFilter_yacc.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/DataFilter.y
        COMMAND ${CMAKE_COMMAND} -E copy ${GC_BINARY_DIR}/DataFilter_yacc.hpp ${GC_BINARY_DIR}/DataFilter_yacc.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Core/DataFilter.y
        COMMENT "Generating DataFilter parser"
        WORKING_DIRECTORY ${GC_BINARY_DIR}
    )
    
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/DataFilter_lex.cpp
        COMMAND ${FLEX_EXECUTABLE} -PDataFilter --nounistd -o ${GC_BINARY_DIR}/DataFilter_lex.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/DataFilter.l
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Core/DataFilter.l
        COMMENT "Generating DataFilter lexer"
    )
    
    # Generate JsonRideFile parser
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/JsonRideFile_yacc.cpp ${GC_BINARY_DIR}/JsonRideFile_yacc.h
        COMMAND ${BISON_EXECUTABLE} -d -p JsonRideFile -o ${GC_BINARY_DIR}/JsonRideFile_yacc.cpp ${CMAKE_CURRENT_SOURCE_DIR}/FileIO/JsonRideFile.y
        COMMAND ${CMAKE_COMMAND} -E copy ${GC_BINARY_DIR}/JsonRideFile_yacc.hpp ${GC_BINARY_DIR}/JsonRideFile_yacc.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/FileIO/JsonRideFile.y
        COMMENT "Generating JsonRideFile parser"
        WORKING_DIRECTORY ${GC_BINARY_DIR}
    )
    
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/JsonRideFile_lex.cpp
        COMMAND ${FLEX_EXECUTABLE} -PJsonRideFile --nounistd -o ${GC_BINARY_DIR}/JsonRideFile_lex.cpp ${CMAKE_CURRENT_SOURCE_DIR}/FileIO/JsonRideFile.l
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/FileIO/JsonRideFile.l
        COMMENT "Generating JsonRideFile lexer"
    )
    
    # Generate RideDB parser
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/RideDB_yacc.cpp ${GC_BINARY_DIR}/RideDB_yacc.h
        COMMAND ${BISON_EXECUTABLE} -d -p RideDB -o ${GC_BINARY_DIR}/RideDB_yacc.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/RideDB.y
        COMMAND ${CMAKE_COMMAND} -E copy ${GC_BINARY_DIR}/RideDB_yacc.hpp ${GC_BINARY_DIR}/RideDB_yacc.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Core/RideDB.y
        COMMENT "Generating RideDB parser"
        WORKING_DIRECTORY ${GC_BINARY_DIR}
    )
    
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/RideDB_lex.cpp
        COMMAND ${FLEX_EXECUTABLE} -PRideDB --nounistd -o ${GC_BINARY_DIR}/RideDB_lex.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Core/RideDB.l
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Core/RideDB.l
        COMMENT "Generating RideDB lexer"
    )
    
    # Generate WorkoutFilter parser
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/WorkoutFilter_yacc.cpp ${GC_BINARY_DIR}/WorkoutFilter_yacc.h
        COMMAND ${BISON_EXECUTABLE} -d -p WorkoutFilter -o ${GC_BINARY_DIR}/WorkoutFilter_yacc.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Train/WorkoutFilter.y
        COMMAND ${CMAKE_COMMAND} -E copy ${GC_BINARY_DIR}/WorkoutFilter_yacc.hpp ${GC_BINARY_DIR}/WorkoutFilter_yacc.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Train/WorkoutFilter.y
        COMMENT "Generating WorkoutFilter parser"
        WORKING_DIRECTORY ${GC_BINARY_DIR}
    )
    
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/WorkoutFilter_lex.cpp
        COMMAND ${FLEX_EXECUTABLE} -PWorkoutFilter --nounistd -o ${GC_BINARY_DIR}/WorkoutFilter_lex.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Train/WorkoutFilter.l
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Train/WorkoutFilter.l
        COMMENT "Generating WorkoutFilter lexer"
    )
    
    # Generate TrainerDayAPIQuery parser
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.cpp ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.h
        COMMAND ${BISON_EXECUTABLE} -d -p TrainerDayAPIQuery -o ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Train/TrainerDayAPIQuery.y
        COMMAND ${CMAKE_COMMAND} -E copy ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.hpp ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Train/TrainerDayAPIQuery.y
        COMMENT "Generating TrainerDayAPIQuery parser"
        WORKING_DIRECTORY ${GC_BINARY_DIR}
    )
    
    add_custom_command(
        OUTPUT ${GC_BINARY_DIR}/TrainerDayAPIQuery_lex.cpp
        COMMAND ${FLEX_EXECUTABLE} -PTrainerDayAPIQuery --nounistd -o ${GC_BINARY_DIR}/TrainerDayAPIQuery_lex.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Train/TrainerDayAPIQuery.l
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Train/TrainerDayAPIQuery.l
        COMMENT "Generating TrainerDayAPIQuery lexer"
    )
    
    # Add custom target for all parsers
    add_custom_target(ParserGeneration ALL
        DEPENDS
        ${GC_BINARY_DIR}/DataFilter_yacc.cpp
        ${GC_BINARY_DIR}/DataFilter_yacc.h
        ${GC_BINARY_DIR}/DataFilter_lex.cpp
        ${GC_BINARY_DIR}/JsonRideFile_yacc.cpp
        ${GC_BINARY_DIR}/JsonRideFile_yacc.h
        ${GC_BINARY_DIR}/JsonRideFile_lex.cpp
        ${GC_BINARY_DIR}/RideDB_yacc.cpp
        ${GC_BINARY_DIR}/RideDB_yacc.h
        ${GC_BINARY_DIR}/RideDB_lex.cpp
        ${GC_BINARY_DIR}/WorkoutFilter_yacc.cpp
        ${GC_BINARY_DIR}/WorkoutFilter_yacc.h
        ${GC_BINARY_DIR}/WorkoutFilter_lex.cpp
        ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.cpp
        ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.h
        ${GC_BINARY_DIR}/TrainerDayAPIQuery_lex.cpp
    )
    
    # Parser sources - use paths relative to main binary dir
    set(GC_SOURCES_PARSERS
        ${GC_BINARY_DIR}/DataFilter_yacc.cpp
        ${GC_BINARY_DIR}/DataFilter_lex.cpp
        ${GC_BINARY_DIR}/JsonRideFile_yacc.cpp
        ${GC_BINARY_DIR}/JsonRideFile_lex.cpp
        ${GC_BINARY_DIR}/RideDB_yacc.cpp
        ${GC_BINARY_DIR}/RideDB_lex.cpp
        ${GC_BINARY_DIR}/WorkoutFilter_yacc.cpp
        ${GC_BINARY_DIR}/WorkoutFilter_lex.cpp
        ${GC_BINARY_DIR}/TrainerDayAPIQuery_yacc.cpp
        ${GC_BINARY_DIR}/TrainerDayAPIQuery_lex.cpp
    )
else()
    message(FATAL_ERROR "bison and/or flex not found. Please install bison and flex.")
endif()

# Add generated parser directory to include path
include_directories(${GC_BINARY_DIR})

# Combine all sources
set(GC_SOURCES
    ${GC_SOURCES_ANT}
    ${GC_SOURCES_CHARTS}
    ${GC_SOURCES_CLOUD}
    ${GC_SOURCES_CORE}
    ${GC_SOURCES_FILEIO}
    ${GC_SOURCES_GUI}
    ${GC_SOURCES_METRICS}
    ${GC_SOURCES_PLANNING}
    ${GC_SOURCES_TRAIN}
    ${GC_SOURCES_CONTRIB}
    ${GC_SOURCES_HTTP}
    ${GC_SOURCES_PYTHON}
    ${GC_SOURCES_R}
    ${GC_SOURCES_ICAL}
    ${GC_SOURCES_USB}
    ${GC_SOURCES_D2XX}
    ${GC_SOURCES_SRMIO}
    ${GC_SOURCES_USBXPRESS}
    ${GC_SOURCES_PARSERS}
)

set(GC_HEADERS
    ${GC_HEADERS_ANT}
    ${GC_HEADERS_CHARTS}
    ${GC_HEADERS_CLOUD}
    ${GC_HEADERS_CORE}
    ${GC_HEADERS_FILEIO}
    ${GC_HEADERS_GUI}
    ${GC_HEADERS_METRICS}
    ${GC_HEADERS_PLANNING}
    ${GC_HEADERS_TRAIN}
    ${GC_HEADERS_CONTRIB}
    ${GC_HEADERS_HTTP}
    ${GC_HEADERS_PYTHON}
    ${GC_HEADERS_R}
    ${GC_HEADERS_ICAL}
    ${GC_HEADERS_USB}
    ${GC_HEADERS_D2XX}
    ${GC_HEADERS_SRMIO}
    ${GC_HEADERS_USBXPRESS}
)

# Create executable
add_executable(GoldenCheetah
    ${GC_SOURCES}
    ${GC_HEADERS}
    ${GC_RC_FILE}
    Resources/application.qrc
)

# Set executable properties
set_target_properties(GoldenCheetah PROPERTIES
    OUTPUT_NAME GoldenCheetah
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Link Qt libraries
target_link_libraries(GoldenCheetah PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${QT_VERSION_MAJOR}::WebEngineCore
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::WebChannel
    Qt${QT_VERSION_MAJOR}::Positioning
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# Qt6-specific linking
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(GoldenCheetah PRIVATE
        Qt${QT_VERSION_MAJOR}::WebEngineQuick
        Qt${QT_VERSION_MAJOR}::Core5Compat
    )
endif()

# Qt5-specific linking
if(QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(GoldenCheetah PRIVATE Qt${QT_VERSION_MAJOR}::WebEngine)
    if(APPLE)
        target_link_libraries(GoldenCheetah PRIVATE Qt${QT_VERSION_MAJOR}::MacExtras)
    endif()
endif()

# Link GSL
target_link_libraries(GoldenCheetah PRIVATE GSL::gsl GSL::gslcblas)

# Link ZLIB
target_link_libraries(GoldenCheetah PRIVATE ZLIB::ZLIB)

# Link platform-specific libraries
target_link_libraries(GoldenCheetah PRIVATE ${GC_PLATFORM_LIBS})

# QWT library is built from the top-level CMakeLists.txt
# No need to add it here again

# Link QWT library (use generator expression for library path)
set(QWT_LIB_DIR "${CMAKE_BINARY_DIR}/qwt")
link_directories(${QWT_LIB_DIR})
target_link_libraries(GoldenCheetah PRIVATE qwt)

# Install targets
install(TARGETS GoldenCheetah
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# Print configuration summary
message(STATUS "===========================================")
message(STATUS "GoldenCheetah Build Configuration")
message(STATUS "===========================================")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "-------------------------------------------")
message(STATUS "Features:")
message(STATUS "  Python Support: ${GC_WANT_PYTHON}")
message(STATUS "  R Support: ${GC_WANT_R}")
message(STATUS "  HTTP API: ${GC_WANT_HTTP}")
message(STATUS "  iCal Support: ${GC_HAVE_ICAL}")
message(STATUS "  USB Support: ${GC_HAVE_LIBUSB}")
message(STATUS "  VLC Support: ${GC_HAVE_VLC}")
message(STATUS "  Samplerate: ${GC_HAVE_SAMPLERATE}")
message(STATUS "===========================================")
