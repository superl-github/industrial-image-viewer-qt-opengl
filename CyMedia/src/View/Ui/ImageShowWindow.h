/**
 * @file ImageShowWindow.h
 * @brief 单张图像快速预览窗口（内部 UI 辅助组件）。
 * @ingroup Display
 *
 * @details
 *  该窗口由 `CyMediaDis::imgShow()` 创建，组合一个 `CyMediaDis` 显示控件和
 *  一个鼠标位置/像素值状态栏。它属于 UI 层的便捷封装，不参与主显示控件的
 *  图像处理管线。
 */
#pragma once

#include "View/Display.h"

#include <QLabel>
#include <QWidget>

namespace CyMedia {
    /**
     * @brief 单图预览窗口。
     * @details 仅供 `CyMediaDis::imgShow()` 内部使用；调用方拿到的是 `QWidget*`。
     */
    class ImageShowWindow : public QWidget {
        Q_OBJECT

    public:
        explicit ImageShowWindow(QWidget* parent = nullptr);
        ~ImageShowWindow() override;

        /// 返回内部显示控件。
        CyMedia::CyMediaDis* display() const;

        /// 上传一帧图像并刷新显示。
        void upImageData(CyMedia::ImageShowInfo info, void* data);

    private:
        void onUpPosPix(qint32 x, qint32 y, double r, double g, double b, bool signlR);

    private:
        CyMedia::CyMediaDis* ui_disPlay = nullptr;
        QLabel* ui_PosRGBLab = nullptr;
    };
}
