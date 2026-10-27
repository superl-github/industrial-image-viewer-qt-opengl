/**
 * @file CyMediaDisRawInfoDialog.h
 * @brief 打开 RAW / 裸视频时使用的图像参数输入对话框。
 * @ingroup Display
 *
 * @details
 *  该对话框只负责收集"宽度、高度、位深、像素格式、像素值类型、文件头偏移、
 *  视频帧率"等参数，不依赖显示控件的内部实现。由业务层在调用 `ImageParse`
 *  之前使用。
 *
 *  @note 已从 `View/Display.h` 拆出：只使用显示控件时无需包含本文件。
 */
#pragma once

#include "common/ImageTypes.h"
#include "common/Export.h"

#include <QDialog>
#include <QString>

namespace CyMedia {
    /**
     * @class CyMedia::CyMediaDis_GetRawInfoDialog
     * @brief RAW 图像/裸视频参数输入对话框。
     */
    class CYMEDIA_LIB GetRawInfoDialog : public QDialog {
        Q_OBJECT

    public:
        explicit GetRawInfoDialog(QWidget* parent = nullptr);

        /// 刷新界面翻译（多语言切换时调用）。
        void flushTrans();

        bool SpecialIsPixeVisible();
        void setSpecialPixeVisible(bool visi);

        bool FpsIsPixeVisible();
        void setFpsVisible(bool visi);

        QString openFileName();
        quint32 imageWidth();
        quint32 imageHeight();
        quint32 imagenBit();
        CyMedia::ePixType imagePixelType();
        CyMedia::ePixelValueType specialPixe();
        quint32 imageOffset();
        float videoFps();

        void setOpenFileName(QString name);
        void setOpenInfo(
            quint32 w,
            quint32 h,
            quint32 bit,
            CyMedia::ePixType pixelType,
            CyMedia::ePixelValueType spv,
            quint32 offset);
        void setFPSValue(float fps);

    protected:
        void initGUI();

    private slots:
        void onOkClicked();
        void onCanCelClicked();

    private:
        class PrivateData;
        PrivateData* d;
    };
}

