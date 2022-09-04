cmake_minimum_required(VERSION 3.5)

project(obs-asio)

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

##########################################
# find libobs                            #
##########################################
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
include(cmake/ObsPluginHelpers.cmake)

if(NOT DEFINED OBS_FRONTEND_LIB)
	set(OBS_FRONTEND_LIB "OBS_FRONTEND_LIB-NOTFOUND" CACHE FILEPATH "OBS frontend library")
	message(FATAL_ERROR "Could not find OBS Frontend API\'s library !")
endif()
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

find_package(Qt5Core REQUIRED)
# Find the QtWidgets library
find_package(Qt5Widgets)


include_directories(
	SYSTEM "${LIBOBS_INCLUDE_DIR}"
	"${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api"
	"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/UI"
	${JUCE_INCLUDE_DIR}
	${JUCE_INCLUDE_DIR}/modules
	${Qt5Core_INCLUDES}
	${Qt5Widgets_INCLUDES}
)

set(obs-asio_QRC
	asio-input.qrc)
	
set(obs-asio_SOURCES
	src/asio-input.cpp
)

set(JUCE_LIB optimized ${JUCE_LIBRARY} debug ${JUCE_LIBRARY_DEBUG})

qt5_add_resources(obs-asio_QRC_SOURCES ${win-asio_QRC})

add_library(obs-asio MODULE
	${win-asio_SOURCES}
	${win-asio_QRC_SOURCES}
)

target_link_libraries(obs-asio
	libobs
	obs-frontend-api
	${JUCE_LIB}
	Qt5::Core
	Qt5::Widgets
)

#install_obs_plugin_with_data(obs-asio data) ==> internal plugin install
#install_external_plugin_with_data(obs-asio data)

set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release")
add_custom_command(TARGET obs-asio POST_BUILD
	COMMAND if $<CONFIG:Release>==1 (
		"${CMAKE_COMMAND}" -E make_directory
			"${RELEASE_DIR}/data/obs-plugins/obs-asio"
			"${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")

	COMMAND if $<CONFIG:Release>==1 (
		"${CMAKE_COMMAND}" -E copy_directory
			"${PROJECT_SOURCE_DIR}/src/data"
			"${RELEASE_DIR}/data/obs-plugins/obs-asio")

	COMMAND if $<CONFIG:Release>==1 (
		"${CMAKE_COMMAND}" -E copy
			"$<TARGET_FILE:obs-asio>"
			"${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")
)			
