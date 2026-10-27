# =============================================================================
# CyMedia library
# =============================================================================

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CYMEDIA_ARCH "x64")
else()
    set(CYMEDIA_ARCH "Win32")
endif()

set(CYMEDIA_FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ffmpeg")
set(CYMEDIA_FFMPEG_INCLUDE_DIR "${CYMEDIA_FFMPEG_ROOT}/include/${CYMEDIA_ARCH}")
set(CYMEDIA_FFMPEG_LIB_DIR "${CYMEDIA_FFMPEG_ROOT}/Lib/${CYMEDIA_ARCH}")
set(CYMEDIA_FFMPEG_BIN_DIR "${CYMEDIA_FFMPEG_ROOT}/Bin/${CYMEDIA_ARCH}")

if(NOT EXISTS "${CYMEDIA_FFMPEG_INCLUDE_DIR}")
    message(FATAL_ERROR "FFmpeg include directory not found: ${CYMEDIA_FFMPEG_INCLUDE_DIR}")
endif()
if(NOT EXISTS "${CYMEDIA_FFMPEG_LIB_DIR}")
    message(FATAL_ERROR "FFmpeg library directory not found: ${CYMEDIA_FFMPEG_LIB_DIR}")
endif()

# Make the runtime path available to the demo target.
set(CYMEDIA_FFMPEG_BIN_DIR "${CYMEDIA_FFMPEG_BIN_DIR}" PARENT_SCOPE)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(CYMEDIA_SOURCES
    3rdparty/qcustomplot/qcustomplot.cpp

    src/Calculate/CalcBayer.cpp
    src/Calculate/CalcColor.cpp
    src/Calculate/CalcMono.cpp
    src/Calculate/CalcRgb.cpp
    src/Calculate/CalcYUV.cpp
    src/Calculate/Calculate.cpp

    src/Parse/FileFunc.cpp
    src/Parse/ImageParse.cpp
    src/Parse/VideoParse.cpp
    src/Parse/VideoParseBase.cpp
    src/Parse/VideoParseFfmpeg.cpp
    src/Parse/VideoParseRaw.cpp

    src/common/DisLog.cpp

    src/View/Display.cpp
    src/View/Annotation/BaseItem.cpp
    src/View/Annotation/DrawItem.cpp
    src/View/Annotation/DrawItemTool.cpp
    src/View/Annotation/Item_Ellipse.cpp
    src/View/Annotation/Item_Line.cpp
    src/View/Annotation/Item_Point.cpp
    src/View/Annotation/Item_Polygon.cpp
    src/View/Annotation/Item_Rect.cpp
    src/View/Annotation/ItemFactory.cpp
    src/View/Annotation/ItemManager.cpp
    src/View/Dialogs/CyMediaDisRawInfoDialog.cpp
    src/View/Dialogs/LicenseW.cpp
    src/View/Pipeline/ImagePipeline.cpp
    src/View/Pipeline/ImageProcessor.cpp
    src/View/Render/CyDMediaDisScen.cpp
    src/View/Render/CyMediaDisView.cpp
    src/View/Render/CyMediaDisViewBckDraw.cpp
    src/View/Render/CyMediaDisViewThumbnail.cpp
    src/View/Ui/DisplayToolBar.cpp
    src/View/Ui/ImageShowWindow.cpp
    src/View/Widgets/CyMediaDisGrayStretch.cpp
    src/View/Widgets/CyMediaDisGrayTest.cpp
    src/View/Widgets/CyMediaRecTimeW.cpp
    src/View/Widgets/CyQCP.cpp
    src/View/Widgets/cycustomwidget.cpp
)

set(CYMEDIA_HEADERS
    3rdparty/qcustomplot/qcustomplot.h
    3rdparty/stb_image/stb_image.h
    3rdparty/stb_image/stb_image_write.h

    src/Calculate/CalcBayer.h
    src/Calculate/CalcColor.h
    src/Calculate/CalcMono.h
    src/Calculate/CalcRgb.h
    src/Calculate/CalcYUV.h
    src/Calculate/Calculate.h

    src/Parse/FileFunc.h
    src/Parse/ImageParse.h
    src/Parse/VideoFormatRegistry.h
    src/Parse/VideoParse.h
    src/Parse/VideoParseBase.h
    src/Parse/VideoParseFfmpeg.h
    src/Parse/VideoParseRaw.h

    src/common/CyMediaBaseDef.h
    src/common/Export.h
    src/common/Version.h
    src/common/Language.h
    src/common/Log.h
    src/common/ImageTypes.h
    src/common/ParseTypes.h
    src/common/DisLog.h

    src/View/Display.h
    src/View/DisplayTypes.h
    src/View/Annotation/BaseItem.h
    src/View/Annotation/DrawItem.h
    src/View/Annotation/DrawItemTool.h
    src/View/Annotation/ItemFactory.h
    src/View/Annotation/ItemManager.h
    src/View/Annotation/Item_Ellipse.h
    src/View/Annotation/Item_Line.h
    src/View/Annotation/Item_Point.h
    src/View/Annotation/Item_Polygon.h
    src/View/Annotation/Item_Rect.h
    src/View/Dialogs/CyMediaDisRawInfoDialog.h
    src/View/Dialogs/LicenseW.h
    src/View/Pipeline/ImagePipeline.h
    src/View/Pipeline/ImageProcessor.h
    src/View/Render/CyDMediaDisScen.h
    src/View/Render/CyMediaDisView.h
    src/View/Render/CyMediaDisViewBckDraw.h
    src/View/Render/CyMediaDisViewThumbnail.h
    src/View/Ui/DisplayToolBar.h
    src/View/Ui/ImageShowWindow.h
    src/View/Widgets/CyMediaDisGrayStretch.h
    src/View/Widgets/CyMediaDisGrayTest.h
    src/View/Widgets/CyMediaRecTimeW.h
    src/View/Widgets/CyQCP.h
    src/View/Widgets/cycustomwidget.h
)

add_library(CyMedia SHARED
    ${CYMEDIA_SOURCES}
    ${CYMEDIA_HEADERS}
    CyMedia.qrc
)

# ---------------------------------------------------------------------------
# Includes / definitions
# ---------------------------------------------------------------------------
target_include_directories(CyMedia
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/common>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/common>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qcustomplot
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stb_image
        ${CYMEDIA_FFMPEG_INCLUDE_DIR}
)

target_compile_definitions(CyMedia PRIVATE
    CYMEDIA_EXPORT
    QT_NO_PRINTER
)

if(MSVC)
    target_compile_options(CyMedia PRIVATE /utf-8 /MP)
endif()

# ---------------------------------------------------------------------------
# Qt / FFmpeg
# ---------------------------------------------------------------------------
target_link_libraries(CyMedia
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
)

if(CYMEDIA_ARCH STREQUAL "Win32")
    set(CYMEDIA_FFMPEG_LIBS
        avcodec-63
        avdevice-63
        avfilter-12
        avformat-63
        avutil-61
        swresample-7
        swscale-10
    )
else()
    set(CYMEDIA_FFMPEG_LIBS
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
    )
endif()

set(CYMEDIA_FFMPEG_LINK_LIBS)
foreach(_ffmpeg_name IN LISTS CYMEDIA_FFMPEG_LIBS)
    string(REPLACE "-" "_" _ffmpeg_var "${_ffmpeg_name}")
    find_library(FFMPEG_${_ffmpeg_var}_LIBRARY
        NAMES ${_ffmpeg_name}
        PATHS "${CYMEDIA_FFMPEG_LIB_DIR}"
        NO_DEFAULT_PATH
    )
    if(NOT FFMPEG_${_ffmpeg_var}_LIBRARY)
        message(FATAL_ERROR
            "FFmpeg library '${_ffmpeg_name}' not found in ${CYMEDIA_FFMPEG_LIB_DIR}")
    endif()
    list(APPEND CYMEDIA_FFMPEG_LINK_LIBS ${FFMPEG_${_ffmpeg_var}_LIBRARY})
endforeach()

target_link_libraries(CyMedia PRIVATE ${CYMEDIA_FFMPEG_LINK_LIBS})

# ---------------------------------------------------------------------------
# Installation (headers + binary)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS CyMedia
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Keep the flat umbrella header available; module headers use common/*.h.
install(FILES src/common/CyMediaBaseDef.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY src/common/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY src/View/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/View
    FILES_MATCHING
        PATTERN "Display.h"
        PATTERN "DisplayTypes.h"
        PATTERN "BaseItem.h"
        PATTERN "DrawItem.h"
        PATTERN "CyMediaDisRawInfoDialog.h"
        PATTERN "LicenseW.h"
        PATTERN "CyMediaDisGrayStretch.h"
        PATTERN "CyMediaDisGrayTest.h"
        PATTERN "CyMediaRecTimeW.h"
        PATTERN "README.md"
)
install(FILES
    src/Parse/ImageParse.h
    src/Parse/VideoParse.h
    src/Parse/VideoParseBase.h
    src/Parse/FileFunc.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Parse
)
install(DIRECTORY src/Calculate/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Calculate
    FILES_MATCHING PATTERN "*.h"
)

# Runtime data required by the parser / color map renderer.
install(DIRECTORY "${CYMEDIA_FFMPEG_BIN_DIR}/"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILES_MATCHING PATTERN "*.dll"
)
install(DIRECTORY resources/colorMap_ALL/
    DESTINATION ${CMAKE_INSTALL_BINDIR}/colorMap
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()
