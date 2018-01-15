cmake_minimum_required(VERSION 3.5)

project(obs-asio)

# in cmake gui set the following:
# RTAUDIO_LIBRARY == path of rtaudio_static.lib
# RTAUDIO_INCLUDE_DIR == path of RtAudio.h
# put also rtaudio.dll in obs-plugins/bin folder

##########################################
# find libobs                            #
##########################################
include(external/FindLibObs.cmake)
find_package(LibObs REQUIRED)

#########################################
# set architecture                      #
#########################################
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
# Find includes in corresponding build directories
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
#set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
#find_package(Qt5Widgets)


include_directories(
#	SYSTEM "${CMAKE_SOURCE_DIR}/libobs"
	"deps/include/rtaudio"
	${RTAUDIO_INCLUDE_DIR}
)
set(obs-asio-sdk_HEADERS
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
	${RTAUDIO_LIBRARY}
#	Qt5::Widgets
)

#install_obs_plugin_with_data(win-asio data)
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