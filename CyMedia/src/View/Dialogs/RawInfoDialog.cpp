#include "RawInfoDialog.h"

#include "View/Display.h" // CyMediaDis::pixelFormatStr

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace CyMedia {
    class GetRawInfoDialog::PrivateData {
    public:
        QLabel* uiFileNameLable = nullptr;
        QLabel* uiWidthTextLable = nullptr;
        QLabel* uiHeightTextLable = nullptr;
        QLabel* uiBitTextLable = nullptr;
        QLabel* ui_special_value_Lable = nullptr;
        QLabel* uiPixeFormatLable = nullptr;
        QLabel* uiOffsetLable = nullptr;
        QLabel* uiFPSLable = nullptr;

        QSpinBox* uiWidthBox = nullptr;
        QSpinBox* uiHeightBox = nullptr;
        QSpinBox* uiBitBox = nullptr;
        QComboBox* ui_special_value_box = nullptr;
        QComboBox* uiPixelFormatBox = nullptr;
        QSpinBox* uiOffsetBox = nullptr;
        QDoubleSpinBox* uiFPSBox = nullptr;

        QPushButton* uiOkBtn = nullptr;
        QPushButton* uiCancleBtn = nullptr;
    };

    GetRawInfoDialog::GetRawInfoDialog(QWidget* parent /*= nullptr*/)
        : QDialog(parent)
        , d(new GetRawInfoDialog::PrivateData) {
        initGUI();
        adjustSize();
    }

    void GetRawInfoDialog::flushTrans() {
        d->uiWidthTextLable->setText(tr("Image width(number of columns)"));
        d->uiHeightTextLable->setText(tr("Image height(number of line)"));
        d->uiBitTextLable->setText(tr("Bit width"));
        d->ui_special_value_Lable->setText(tr("Pixel data format"));
        d->uiPixeFormatLable->setText(tr("Pixel format"));
        d->uiOffsetLable->setText(tr("Data head offset"));
        d->uiFPSLable->setText(tr("Fps"));

        d->uiOkBtn->setText(tr("confirm"));
        d->uiCancleBtn->setText(tr("cancle"));
    }

    bool GetRawInfoDialog::SpecialIsPixeVisible() {
        return d->ui_special_value_box->isVisible();
    }

    void GetRawInfoDialog::setSpecialPixeVisible(bool visi) {
        d->ui_special_value_Lable->setVisible(visi);
        d->ui_special_value_box->setVisible(visi);
    }

    bool GetRawInfoDialog::FpsIsPixeVisible() {
        return d->uiFPSBox->isVisible();
    }

    void GetRawInfoDialog::setFpsVisible(bool visi) {
        d->uiFPSLable->setVisible(visi);
        d->uiFPSBox->setVisible(visi);
    }

    QString GetRawInfoDialog::openFileName() {
        return d->uiFileNameLable->text();
    }

    quint32 GetRawInfoDialog::imageWidth() {
        return d->uiWidthBox->value();
    }

    quint32 GetRawInfoDialog::imageHeight() {
        return d->uiHeightBox->value();
    }

    quint32 GetRawInfoDialog::imagenBit() {
        return d->uiBitBox->value();
    }

    CyMedia::ePixType GetRawInfoDialog::imagePixelType() {
        return CyMedia::ePixType(d->uiPixelFormatBox->currentIndex());
    }

    CyMedia::ePixelValueType GetRawInfoDialog::specialPixe() {
        return CyMedia::ePixelValueType(d->ui_special_value_box->currentIndex());
    }

    quint32 GetRawInfoDialog::imageOffset() {
        return d->uiOffsetBox->value();
    }

    float GetRawInfoDialog::videoFps() {
        return d->uiFPSBox->value();
    }

    void GetRawInfoDialog::setOpenFileName(QString name) {
        d->uiFileNameLable->setText(name);
    }

    void GetRawInfoDialog::setOpenInfo(quint32 w, quint32 h, quint32 bit,
        CyMedia::ePixType pixelType, CyMedia::ePixelValueType spv, quint32 offset) {
        d->uiWidthBox->setValue(w);
        d->uiHeightBox->setValue(h);
        d->uiBitBox->setValue(bit);
        d->ui_special_value_box->setCurrentIndex(int(spv));
        d->uiPixelFormatBox->setCurrentIndex(int(pixelType));
        d->uiOffsetBox->setValue(offset);
    }

    void GetRawInfoDialog::setFPSValue(float fps) {
        d->uiFPSBox->setValue(fps);
    }

    void GetRawInfoDialog::initGUI() {
        d->uiFileNameLable = new QLabel(this);
        d->uiWidthTextLable = new QLabel(this);
        d->uiHeightTextLable = new QLabel(this);
        d->uiBitTextLable = new QLabel(this);
        d->ui_special_value_Lable = new QLabel(this);
        d->uiPixeFormatLable = new QLabel(this);
        d->uiOffsetLable = new QLabel(this);
        d->uiFPSLable = new QLabel(this);

        d->uiWidthBox = new QSpinBox(this);
        d->uiWidthBox->setRange(2, 20000);
        d->uiWidthBox->setValue(1000);
        d->uiHeightBox = new QSpinBox(this);
        d->uiHeightBox->setRange(2, 20000);
        d->uiHeightBox->setValue(1000);
        d->uiBitBox = new QSpinBox(this);
        d->uiBitBox->setRange(1, 32);
        d->uiBitBox->setValue(8);
        d->ui_special_value_box = new QComboBox(this);
        d->ui_special_value_box->addItem("int(default)");
        d->ui_special_value_box->addItem("float32");
        d->ui_special_value_box->addItem("float64");
        d->uiPixelFormatBox = new QComboBox(this);
        int pixelTypeI = int(CyMedia::MONO);
        for (; pixelTypeI <= int(CyMedia::FOURCC_NV21); pixelTypeI++) {
            d->uiPixelFormatBox->addItem(CyMediaDis::pixelFormatStr(CyMedia::ePixType(pixelTypeI)));
        }
        ((QListView*)d->uiPixelFormatBox->view())->setRowHidden(int(CyMedia::MONO_OVERSIZE), true);

        d->uiOffsetBox = new QSpinBox(this);
        d->uiOffsetBox->setRange(0, 0x7EFFFFFF);
        d->uiOffsetBox->setValue(0);

        d->uiFPSBox = new QDoubleSpinBox(this);
        d->uiFPSBox->setRange(1, 1000);
        d->uiFPSBox->setValue(24);

        d->uiOkBtn = new QPushButton(this);
        d->uiCancleBtn = new QPushButton(this);
        d->uiFileNameLable->setText("None");
        d->uiFileNameLable->setAlignment(Qt::AlignHCenter);

        QWidget* formWidget = new QWidget(this);
        auto formLayout = new QFormLayout(formWidget);
        quint32 rowNum = 0;
        formLayout->setWidget(rowNum, QFormLayout::LabelRole, d->uiWidthTextLable);
        formLayout->setWidget(rowNum, QFormLayout::FieldRole, d->uiWidthBox);
        rowNum++;
        formLayout->setWidget(rowNum, QFormLayout::LabelRole, d->uiHeightTextLable);
        formLayout->setWidget(rowNum, QFormLayout::FieldRole, d->uiHeightBox);
        rowNum++;
        formLayout->setWidget(rowNum, QFormLayout::LabelRole, d->uiPixeFormatLable);
        formLayout->setWidget(rowNum, QFormLayout::FieldRole, d->uiPixelFormatBox);
        rowNum++;
        formLayout->setWidget(rowNum, QFormLayout::LabelRole, d->uiBitTextLable);
        formLayout->setWidget(rowNum, QFormLayout::FieldRole, d->uiBitBox);
        rowNum++;
        formLayout->setWidget(rowNum, QFormLayout::LabelRole, d->ui_special_value_Lable);
        formLayout->setWidget(rowNum, QFormLayout::FieldRole, d->ui_special_value_box);
        rowNum++;
        formLayout->setWidget(rowNum, QFormLayout::LabelRole, d->uiOffsetLable);
        formLayout->setWidget(rowNum, QFormLayout::FieldRole, d->uiOffsetBox);
        rowNum++;
        formLayout->setWidget(rowNum, QFormLayout::LabelRole, d->uiFPSLable);
        formLayout->setWidget(rowNum, QFormLayout::FieldRole, d->uiFPSBox);
        rowNum++;

        QWidget* btnWidget = new QWidget(this);
        QHBoxLayout* btnLayout = new QHBoxLayout(btnWidget);
        btnLayout->addWidget(d->uiOkBtn);
        btnLayout->addWidget(d->uiCancleBtn);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(d->uiFileNameLable);
        mainLayout->addWidget(formWidget);
        mainLayout->addWidget(btnWidget);

        connect(d->uiOkBtn, &QPushButton::clicked, this, &GetRawInfoDialog::onOkClicked);
        connect(d->uiCancleBtn, &QPushButton::clicked, this, &GetRawInfoDialog::onCanCelClicked);
        connect(d->uiPixelFormatBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                switch (CyMedia::ePixType(index)) {
                    case CyMedia::MONO:
                    case CyMedia::BAYERGR:
                    case CyMedia::BAYERBG:
                    case CyMedia::BAYERGB:
                    case CyMedia::BAYERRG:
                    case CyMedia::MONO_OVERSIZE: {
                    }break;

                    case CyMedia::RGB:
                    case CyMedia::RGBA:
                    case CyMedia::BGR: {
                        d->ui_special_value_box->setCurrentIndex(0);
                    }break;

                    case CyMedia::MONO10P:
                    case CyMedia::MONO10P_GVSP: {
                        d->uiBitBox->setValue(10);
                    }break;

                    case CyMedia::MONO12P:
                    case CyMedia::MONO12P_GVSP: {
                        d->uiBitBox->setValue(12);
                        d->ui_special_value_box->setCurrentIndex(0);
                    }break;
                }
            });

        flushTrans();
    }

    void GetRawInfoDialog::onOkClicked() {
        this->accept();
    }

    void GetRawInfoDialog::onCanCelClicked() {
        this->reject();
    }
}
