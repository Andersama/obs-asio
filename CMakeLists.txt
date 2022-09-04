cmake_minimum_required(VERSION 3.5)

project(obs-asio VERSION 3.1.0)
add_library(obs-asio MODULE)

# in cmake gui set the following:
# LIBOBS_INCLUDE_DIR = path to obs.h, obs-studio/libobs
# LIBOBS_LIB = filepath to obs.lib, ex: obs-studio/build/libobs/RelWithDebInfo/obs.lib
# OBS_FRONTEND_LIB = filepath to obs-frontend-api.lib , ex: obs-studio/build/UI/obs-frontend-api/RelWithDebInfo/obs-frontend-api.lib
# QTDIR = path to QT , ex: I:/Qt/5.9/msvc2015_64 for x64
# JUCE_LIBRARY == path of juce.lib
# JUCE_LIBRARY_DEBUG == path of juce_debug.lib (debug version of the lib)
# The Juce lib can be created with ProJucer with either a static lib project or a dynamic lib project.
# JUCE_INCLUDE_DIR == path of Juce includes; if you used ProJucer, this will be the path of JuceLibraryCode.
# If you select to build with a juce dll, you'll have to copy it in the obs-plugins folder.

include(FetchContent)
FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG 7.0.2
)

FetchContent_Declare(
	ASIOSDK
	URL	https://www.steinberg.net/asiosdk
	URL_HASH MD5=b161ab28894dd1dd91e075194f003dec
)

FetchContent_MakeAvailable(JUCE ASIOSDK)

##########################################
# find libobs                            #
##########################################
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
include(cmake/ObsPluginHelpers.cmake)
find_qt(COMPONENTS Widgets Core)

##########################################
# set architecture                       #
##########################################
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ARCH 64)
else()
	set(ARCH 32)
endif()

if(ARCH EQUAL 64)
	set(OBS_ARCH_NAME "64bit")
	set(OBS_BUILDDIR_ARCH "build64")
else()
	set(OBS_ARCH_NAME "32bit")
	set(OBS_BUILDDIR_ARCH "build32")
endif()


##########################################
# QT support                             #
##########################################
set(CMAKE_PREFIX_PATH "${QTDIR}")
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(obs-asio_QRC
	asio-input.qrc)
	
set(obs-asio_SOURCES
	src/asio-input.cpp
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/asio-input.cpp)

target_compile_definitions(obs-asio
	PUBLIC
	# JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
	JUCE_WEB_BROWSER=1  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
	JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
	JUCE_ASIO=1
)

target_include_directories(obs-asio PRIVATE ${CMAKE_SOURCE_DIR}/src ${JUCE_MODULES_DIR} ${asiosdk_SOURCE_DIR}/common)

qt_add_resources(obs-asio_QRC_SOURCES ${win-asio_QRC})

target_link_libraries(obs-asio PRIVATE
	OBS::libobs
	OBS::obs-frontend-api
	juce::juce_core
	juce::juce_audio_devices
	juce::juce_audio_utils
	Qt::Core
	Qt::Widgets
)

# --- Windows-specific build settings and tasks ---
if(OS_WINDOWS)
  configure_file(cmake/bundle/windows/installer-Windows.iss.in
                 ${CMAKE_BINARY_DIR}/installer-Windows.generated.iss)

  configure_file(cmake/bundle/windows/resource.rc.in ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc)

  if(MSVC)
    # suppress some errors
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /WX- /w14596)
  endif()
  # --- End of section ---
endif()

setup_plugin_target(obs-asio)
