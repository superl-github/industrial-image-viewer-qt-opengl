/**
 * @file DisplayTypes.h
 * @brief CyMedia 显示模块的轻量公共类型（不依赖 QGraphics / OpenGL）。
 * @ingroup Display
 *
 * @details
 *  该头文件只包含图像显示 API 需要的枚举和简单值类型，保持
 *  显示公共头轻量、可被非 UI 模块安全包含。
 *
 *  - `CyDisDrawItem::ItemType`：图形标注类型，供显示控件与业务层共享。
 *  - `CyMedia::StretchRange`：灰度拉伸区间，供拉伸控件/处理管线共享。
 *
 *  @note 该头文件保持 C++/QtCore 级别依赖，可被非 UI 模块安全包含。
 */
#pragma once

#include "common/ImageTypes.h"

#include <cstdint>
#include <functional>

namespace CyDisDrawItem {
    /**
     * @brief 可绘制的图形标注类型。
     * @details `Invalid` 表示未激活绘制；其余值对应点、矩形、线、椭圆、多边形。
     */
    enum ItemType {
        Invalid = 0, ///< 未激活 / 取消绘制
        Point,       ///< 点
        Rectangle,   ///< 矩形
        Line,        ///< 线
        Ellipse,     ///< 椭圆
        Polygon,     ///< 多边形
        // Text, Image ... 预留
    };
}

namespace CyMedia {
    /**
     * @brief 灰度/对比度拉伸区间。
     * @details `start`、`end` 为拉伸输入范围，`max` 为当前位深允许的最大值。
     */
    struct StretchRange {
        int32_t start = 0; ///< 拉伸起点
        int32_t end = 0;   ///< 拉伸终点
        int32_t max = 0;   ///< 位深上限（例如 8bit 为 255）
    };

    /**
     * @brief 图像处理完成回调。
     * @details 参数依次为：处理后的图像信息、数据指针、用户上下文。
     *          回调在工作线程中执行，data 仅在回调期间有效。
     */
    using ImageFrameCallback = std::function<void(ImageShowInfo&, uint8_t*, void*)>;
}

