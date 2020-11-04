cmake_minimum_required(VERSION 3.5)

project(obs-asio)

# Retrieve obassasio.dll from https://www.un4seen.com/download.php?bassasio14
# Put also bassasio.dll in obs-plugins/64bit folder.
# Be careful there are two versions of dll (x86 and x64)

##########################################
# find libobs                            #
##########################################
include(external/FindLibObs.cmake)
find_package(LibObs REQUIRED)
##########################################
# QT support                             #
##########################################
set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 REQUIRED Core Widgets)

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

include_directories(
	"${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api"
	${Qt5Core_INCLUDES}
	${Qt5Widgets_INCLUDES}
)
set(obs-asio-sdk_HEADERS
	src/bassasio-loader.h
	)

set(obs-asio_HEADERS
	)

set(obs-asio_SOURCES
	src/asio-input.cpp
)

add_library(obs-asio MODULE
	${obs-asio_SOURCES}
	${obs-asio_HEADERS}
	${obs-asio-sdk_HEADERS}
)
target_link_libraries(obs-asio
	libobs
	Qt5::Core
	Qt5::Widgets
)

#install_obs_plugin_with_data(win-asio data) ==> internal plugin install
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
			
#Copy to obs-studio dev environment for immediate testing
	COMMAND if $<CONFIG:Debug>==1 (
		"${CMAKE_COMMAND}" -E copy
			"$<TARGET_FILE:obs-asio>"
			"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${OBS_ARCH_NAME}")

	COMMAND if $<CONFIG:Debug>==1 (
		"${CMAKE_COMMAND}" -E make_directory
			"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/obs-asio")

	COMMAND if $<CONFIG:Debug>==1 (
		"${CMAKE_COMMAND}" -E copy_directory
			"${PROJECT_SOURCE_DIR}/data"
			"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/obs-asio")
	)