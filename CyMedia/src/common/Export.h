/**
 * @file Export.h
 * @brief CyMedia 动态库导入/导出宏与编译器能力检测。
 * @ingroup Common
 */
#pragma once

#if (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64) || defined(__SSE2__)
#define HAS_SSE2 1
#else
#define HAS_SSE2 0
#endif

#if defined(_WIN32)
#  if defined(CYMEDIA_EXPORT)
#    define CYMEDIA_LIB __declspec(dllexport)
#  else
#    define CYMEDIA_LIB __declspec(dllimport)
#  endif
#else
#  define CYMEDIA_LIB
#endif
