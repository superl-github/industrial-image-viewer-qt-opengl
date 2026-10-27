cmake_minimum_required(VERSION 3.16)

project(CyMedia VERSION 1.2.7 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# The library currently depends on the Windows FFmpeg binaries in 3rdparty.
if(NOT WIN32)
    message(FATAL_ERROR "CyMedia currently supports Windows builds only.")
endif()

find_package(Qt5 5.14 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

option(CYMEDIA_BUILD_DEMO "Build the CyMediaDisTest demo application" ON)
option(CYMEDIA_DEPLOY_QT "Run windeployqt for the demo after build" OFF)

add_subdirectory(CyMedia)

if(CYMEDIA_BUILD_DEMO)
    add_subdirectory(CyMediaDisTest)
endif()
