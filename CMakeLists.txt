cmake_minimum_required(VERSION 3.5)

project(obs-asio)

# in cmake gui set the following:
# LIBOBS_INCLUDE_DIR = path to obs.h, obs-studio/libobs
# LIBOBS_LIB = filepath to obs.lib, ex: obs-studio/build/libobs/RelWithDebInfo/obs.lib
# OBS_FRONTEND_LIB = filepath to obs-frontend-api.lib , ex: obs-studio/build/UI/obs-frontend-api/RelWithDebInfo/obs-frontend-api.lib
# QTDIR = path to QT , ex: I:/Qt/5.9/msvc2015_64 for x64
# PORTAUDIO_LIBRARY == filepath of portaudio_x64.lib (ex: portaudio/build/Debug/portaudio_x64.lib)
# PORTAUDIO_INCLUDE_DIR == path of bortaudio.h (usually portaudio/include)
# For building portaudio, either use regular portaudio (binary distribution forbidden due to non compliance with GPL v2).
# Or use our fork: https://github.com/pkviet/portaudio on branch openASIO which is under GPL v2+.
# Put also portaudio_x64.dll in obs-plugins/bin folder

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
# find libobs                            #
##########################################
include(external/FindLibObs.cmake)
find_package(LibObs REQUIRED)
if(NOT DEFINED OBS_FRONTEND_LIB)
	set(OBS_FRONTEND_LIB "OBS_FRONTEND_LIB-NOTFOUND" CACHE FILEPATH "OBS frontend library")
	message(FATAL_ERROR "Could not find OBS Frontend API\'s library !")
endif()
##########################################
# QT support                             #
##########################################
set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)


include_directories(
	SYSTEM "${LIBOBS_INCLUDE_DIR}"
	"${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api"
	"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/UI"
	${PORTAUDIO_INCLUDE_DIR}
	${Qt5Core_INCLUDES}
	${Qt5Widgets_INCLUDES}
)
set(obs-asio-sdk_HEADERS
	)

set(obs-asio_HEADERS
	src/circle-buffer.h
	)

set(obs-asio_SOURCES
	src/asio-input.cpp
)
set(obs-asio_QRC
	src/asio-input.qrc)

qt5_add_resources(obs-asio_QRC_SOURCES ${obs-asio_QRC})

add_library(obs-asio MODULE
	${obs-asio_SOURCES}
	${obs-asio_HEADERS}
	${obs-asio-sdk_HEADERS}
	${obs-asio_QRC_SOURCES}
)

target_link_libraries(obs-asio
	libobs
	"${OBS_FRONTEND_LIB}"
	${PORTAUDIO_LIBRARY}
	Qt5::Core
	Qt5::Widgets
)

#install_obs_plugin_with_data(win-asio data) ==> internal plugin install
install_external_plugin_with_data(obs-asio data)

# set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release")
# add_custom_command(TARGET obs-asio POST_BUILD
	# COMMAND if $<CONFIG:Release>==1 (
		# "${CMAKE_COMMAND}" -E make_directory
			# "${RELEASE_DIR}/data/obs-plugins/obs-asio"
			# "${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")

	# COMMAND if $<CONFIG:Release>==1 (
		# "${CMAKE_COMMAND}" -E copy_directory
			# "${PROJECT_SOURCE_DIR}/src/data"
			# "${RELEASE_DIR}/data/obs-plugins/obs-asio")

	# COMMAND if $<CONFIG:Release>==1 (
		# "${CMAKE_COMMAND}" -E copy
			# "$<TARGET_FILE:obs-asio>"
			# "${RELEASE_DIR}/obs-plugins/${OBS_ARCH_NAME}")
			
# #Copy to obs-studio dev environment for immediate testing
	# COMMAND if $<CONFIG:Debug>==1 (
		# "${CMAKE_COMMAND}" -E copy
			# "$<TARGET_FILE:obs-asio>"
			# "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${OBS_ARCH_NAME}")

	# COMMAND if $<CONFIG:Debug>==1 (
		# "${CMAKE_COMMAND}" -E make_directory
			# "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/obs-asio")

	# COMMAND if $<CONFIG:Debug>==1 (
		# "${CMAKE_COMMAND}" -E copy_directory
			# "${PROJECT_SOURCE_DIR}/data"
			# "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/obs-asio")
	# )