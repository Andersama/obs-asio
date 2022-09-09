cmake_minimum_required(VERSION 3.5)

project(obs-asio VERSION 3.1.0)
add_library(obs-asio MODULE)

set(PLUGIN_AUTHOR "Andersama & pkv")

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/Andersama/JUCE
  GIT_TAG open_asio_juce)

FetchContent_MakeAvailable(JUCE)

# ##################################################################################################
# find libobs                                                                                      #
# ##################################################################################################
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
include(cmake/ObsPluginHelpers.cmake)
find_qt(COMPONENTS Widgets Core)

# ##################################################################################################
# set architecture                                                                                 #
# ##################################################################################################
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

# ##################################################################################################
# QT support                                                                                       #
# ##################################################################################################
set(CMAKE_PREFIX_PATH "${QTDIR}")
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(obs-asio_QRC asio-input.qrc)

set(obs-asio_SOURCES src/asio-input.cpp)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/asio-input.cpp)

target_compile_definitions(
  obs-asio
  PUBLIC # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
         JUCE_WEB_BROWSER=1 # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the
                            # `juce_add_plugin` call
         JUCE_USE_CURL=0 # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
         JUCE_ASIO=1)

target_include_directories(obs-asio PRIVATE ${CMAKE_SOURCE_DIR}/src ${JUCE_MODULES_DIR})

qt_add_resources(obs-asio_QRC_SOURCES ${win-asio_QRC})

target_link_libraries(
  obs-asio
  PRIVATE OBS::libobs
          OBS::obs-frontend-api
          juce::juce_core
          juce::juce_audio_devices
          juce::juce_audio_utils
          Qt::Core
          Qt::Widgets)

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
