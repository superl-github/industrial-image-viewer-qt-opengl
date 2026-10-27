# =============================================================================
# CyMediaDisTest demo / smoke-test application
#
# Two modes:
#   1. In-tree: added by the top-level CMakeLists.txt (CyMedia target exists).
#   2. Standalone: configure this folder directly from a release package:
#        cmake -S Demo/CyMediaDisTest -B build -G "Visual Studio 17 2022" -A x64 `
#              -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017_64"
# =============================================================================

cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(CyMediaDisTest LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    find_package(Qt5 5.14 REQUIRED COMPONENTS Core Gui Widgets)
endif()

set(CYMEDIA_DEMO_SOURCES
    main.cpp
    CyMediaDisTest.cpp
    CyMediaTestCommon.cpp
    CyPlaySlider.cpp
    CyMediaDisTest.h
    CyMediaTestCommon.h
    CyPlaySlider.h
    CyPixelFormat.h
    resource.h
    CyMediaDisTest.ui
    CyMediaDisTest.qrc
)

if(WIN32)
    list(APPEND CYMEDIA_DEMO_SOURCES CyMediaTest.rc)
endif()

add_executable(CyMediaDisTest ${CYMEDIA_DEMO_SOURCES})

target_include_directories(CyMediaDisTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# ---------------------------------------------------------------------------
# Standalone package mode: create an imported CyMedia target from the release
# package layout produced by GenRelease.bat / cmake --install.
# ---------------------------------------------------------------------------
if(NOT TARGET CyMedia)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_cymedia_arch "x64")
    else()
        set(_cymedia_arch "Win32")
    endif()

    get_filename_component(_cymedia_default_root "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../CyMedia/src/View/Display.h")
        get_filename_component(_cymedia_default_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    endif()
    set(CYMEDIA_PACKAGE_ROOT "${_cymedia_default_root}" CACHE PATH
        "CyMedia release package root (contains include/, Bin/, Lib/)")

    find_path(CYMEDIA_INCLUDE_DIR
        NAMES View/Display.h
        HINTS "${CYMEDIA_PACKAGE_ROOT}/include"
        NO_DEFAULT_PATH
    )
    if(NOT CYMEDIA_INCLUDE_DIR)
        message(FATAL_ERROR
            "CyMedia headers not found. Set -DCYMEDIA_PACKAGE_ROOT=<package root> "
            "or use the top-level CMake project.")
    endif()

    find_library(CYMEDIA_LIB_RELEASE
        NAMES CyMedia
        HINTS "${CYMEDIA_PACKAGE_ROOT}/Lib/${_cymedia_arch}/Release"
        NO_DEFAULT_PATH
    )
    find_library(CYMEDIA_LIB_DEBUG
        NAMES CyMedia
        HINTS "${CYMEDIA_PACKAGE_ROOT}/Lib/${_cymedia_arch}/Debug"
        NO_DEFAULT_PATH
    )
    if(NOT CYMEDIA_LIB_RELEASE AND NOT CYMEDIA_LIB_DEBUG)
        message(FATAL_ERROR
            "CyMedia.lib not found under ${CYMEDIA_PACKAGE_ROOT}/Lib/${_cymedia_arch}.")
    endif()

    add_library(CyMedia SHARED IMPORTED)
    set_target_properties(CyMedia PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CYMEDIA_INCLUDE_DIR};${CYMEDIA_INCLUDE_DIR}/common"
    )

    if(CYMEDIA_LIB_RELEASE)
        set_target_properties(CyMedia PROPERTIES
            IMPORTED_CONFIGURATIONS "RELEASE"
            IMPORTED_IMPLIB_RELEASE "${CYMEDIA_LIB_RELEASE}"
            IMPORTED_LOCATION_RELEASE
                "${CYMEDIA_PACKAGE_ROOT}/Bin/${_cymedia_arch}/Release/CyMedia.dll"
        )
    endif()
    if(CYMEDIA_LIB_DEBUG)
        set_target_properties(CyMedia PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_IMPLIB_DEBUG "${CYMEDIA_LIB_DEBUG}"
            IMPORTED_LOCATION_DEBUG
                "${CYMEDIA_PACKAGE_ROOT}/Bin/${_cymedia_arch}/Debug/CyMedia.dll"
        )
    elseif(CYMEDIA_LIB_RELEASE)
        # The release package may only contain Release; map Debug to Release
        # so the demo can still be configured during development.
        set_target_properties(CyMedia PROPERTIES
            IMPORTED_IMPLIB_DEBUG "${CYMEDIA_LIB_RELEASE}"
            IMPORTED_LOCATION_DEBUG
                "${CYMEDIA_PACKAGE_ROOT}/Bin/${_cymedia_arch}/Release/CyMedia.dll"
        )
    endif()
endif()

target_link_libraries(CyMediaDisTest PRIVATE
    CyMedia
    Qt5::Widgets
)

if(WIN32)
    # The demo uses main() with the Windows subsystem, matching the VS project.
    set_target_properties(CyMediaDisTest PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(CyMediaDisTest PRIVATE Qt5::WinMain)
endif()

if(MSVC)
    target_compile_options(CyMediaDisTest PRIVATE /utf-8 /MP)
endif()

# ---------------------------------------------------------------------------
# Runtime layout: make the demo runnable directly from the build tree.
# ---------------------------------------------------------------------------
set(CYMEDIA_DEMO_RUNTIME_DIR "$<TARGET_FILE_DIR:CyMediaDisTest>")

add_custom_command(TARGET CyMediaDisTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:CyMedia>"
        "${CYMEDIA_DEMO_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/App_en_US.qm"
        "${CMAKE_CURRENT_SOURCE_DIR}/App_zh_CN.qm"
        "${CYMEDIA_DEMO_RUNTIME_DIR}"
    COMMENT "Copying CyMedia runtime next to CyMediaDisTest"
)

if(DEFINED CYMEDIA_FFMPEG_BIN_DIR AND EXISTS "${CYMEDIA_FFMPEG_BIN_DIR}")
    add_custom_command(TARGET CyMediaDisTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CYMEDIA_FFMPEG_BIN_DIR}"
            "${CYMEDIA_DEMO_RUNTIME_DIR}"
        COMMENT "Copying FFmpeg DLLs next to CyMediaDisTest"
    )
elseif(DEFINED CYMEDIA_PACKAGE_ROOT)
    add_custom_command(TARGET CyMediaDisTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CYMEDIA_PACKAGE_ROOT}/Bin/${_cymedia_arch}/$<CONFIG>"
            "${CYMEDIA_DEMO_RUNTIME_DIR}"
        COMMENT "Copying package runtime next to CyMediaDisTest"
    )
endif()

if(DEFINED CYMEDIA_DEPLOY_QT AND CYMEDIA_DEPLOY_QT)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${Qt5_DIR}/../../../bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET CyMediaDisTest POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-compiler-runtime
                "$<TARGET_FILE:CyMediaDisTest>"
            COMMENT "Deploying Qt runtime with windeployqt"
        )
    else()
        message(WARNING "windeployqt not found; set CYMEDIA_DEPLOY_QT=OFF or add Qt bin to PATH.")
    endif()
endif()

