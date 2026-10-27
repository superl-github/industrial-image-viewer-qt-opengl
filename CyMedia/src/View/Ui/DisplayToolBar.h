/**
 * @file DisplayToolBar.h
 * @brief 显示控件顶部工具栏（缩放 / 翻转）。
 * @ingroup Display
 *
 * @details
 *  将原 `Display.cpp` 中内联的 6 个按钮、样式和连接逻辑收敛到独立 UI 组件。
 *  工具栏只发出"用户意图"信号，不直接操作视图，便于替换皮肤或做无按钮嵌入。
 */
#pragma once

#include <QString>
#include <QWidget>

namespace CyMedia {
    /**
     * @class CyMedia::DisplayToolBar
     * @brief 显示控件顶部的黑色工具栏。
     */
    class DisplayToolBar : public QWidget {
        Q_OBJECT

    public:
        /**
         * @brief 构造工具栏。
         * @param iconPrefix 图标资源前缀（如 ":/resources/ICONS/"）。
         * @param parent 父窗口。
         */
        explicit DisplayToolBar(const QString& iconPrefix, QWidget* parent = nullptr);
        ~DisplayToolBar() override;

    signals:
        /// 请求放大。
        void zoomInRequested();
        /// 请求缩小。
        void zoomOutRequested();
        /// 请求 1:1 显示。
        void zoomRawRequested();
        /// 请求自适应窗口。
        void zoomFitRequested();
        /// 请求水平翻转。
        void flipHorizontalRequested();
        /// 请求垂直翻转。
        void flipVerticalRequested();

    private:
        class PrivateData;
        PrivateData* d = nullptr;
    };
}
