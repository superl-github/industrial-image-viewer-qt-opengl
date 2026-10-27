#pragma once

#include <QWidget>
#include <QDialog>
#include <QFlags>
#include <QMovie>
#include <QLabel>

namespace CyCustomWidget
{
    enum uiLanguage {
        English,
        simple_Chinese,
    };

    class CyRangeSlider : public QWidget
    {
        Q_OBJECT

    public:
        enum Option
        {
            NoHandle = 0x0,
            LeftHandle = 0x1,
            RightHandle = 0x2,
            DoubleHandles = LeftHandle | RightHandle
        };
        Q_DECLARE_FLAGS(Options, Option)

        CyRangeSlider(QWidget* parent = nullptr);
        CyRangeSlider(Qt::Orientation ori, Options t = DoubleHandles, QWidget* parent = nullptr);
        ~CyRangeSlider();

        QSize minimumSizeHint() const override;
        QColor selectedColor();
        int sliderBarHeight();
        int minimumValue();
        int maximumValue();
        int firstValue();
        int secondValue();
        int interval();
        bool handleTrack();
        bool gradient();

        void setSelectedColor(QColor color);
        void setSliderBarHeight(uint32_t height, float HandleZoom = 2.0);
        void setMinimumValue(int minNum, bool needSignal = true);
        void setMaximumValue(int maxNum, bool needSignal = true);
        bool setFirstValue(int firstNum, bool needSignal = true);
        bool setSecondValue(int secondNum, bool needSignal = true);
        void setminimumInterval(int minInter);
        void setHandleTrack(bool tracking);
        void setGradient(bool graident);
        void setGradientColor(qreal colorPos, QColor color);
        

    signals:
        void firstValueChanged(int firstNum);
        void secondValueCHanged(int secondNum);
        void allValueChanged(int firstNum, int secondNum);
        void mouseRelease();    // 如果tracking为true，正常触发，如果tracking为false, 只会在不触发上面三个信号的情况下触发

    protected:
        QRectF handleRect(int value);
        QRectF firstHandleRect();
        QRectF secondHandleRect();
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event)override;
        void mouseMoveEvent(QMouseEvent* event)override;
        void mouseReleaseEvent(QMouseEvent* event)override;
        void changeEvent(QEvent* event)override;

    private:
        Q_DISABLE_COPY(CyRangeSlider)
        class CyRangeSliderPrivate;
        CyRangeSliderPrivate* p_data = nullptr;
    };


    class CyCheckButton : public QWidget
    {
        Q_OBJECT

    public:
        CyCheckButton(QWidget* parent = nullptr);
        ~CyCheckButton();

        QSize minimumSizeHint() const override;
        bool isChecked();

        qreal slotHeight();
        void setSlotHeight(qreal height, qreal aspectRatio = 1.6);

        QColor enableCheckColor();
        void setEnableCheckColor(QColor color);

        QColor disenableCheckColor();
        void setDisenableCheckColor(QColor color);

        bool useGradient();
        void setUseGradient(bool use);

        QLinearGradient gradient();
        void setgradient(QLinearGradient gradient);

    public slots:
        void setChecked(bool check, bool needSignal = true);

    signals:
        void stateChanged(bool checked);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event)override;
        void changeEvent(QEvent* event)override;

    private:
        QRectF handleRect();

        class CyCheckButtonPrivate;
        CyCheckButtonPrivate* p_data = nullptr;
    };


    class SaveFileDoneDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit SaveFileDoneDialog(QString filepath, QWidget* parent = nullptr);
        ~SaveFileDoneDialog();

    public:
        QSize minimumSizeHint() const override;
        QString path();
        int64_t autoHiadeTime();

        bool setPath(QString path);
        void setText(QString text);
        void setIcon(QIcon icon);
        void setButtonTextColor(QColor color);
        void setAutoHideTime(int64_t ms);
        void flushHideTimer();

    protected:
        void showEvent(QShowEvent* event)override;
        void hideEvent(QHideEvent* event)override;

    private:
        void initGUI();

        class PrivateData;
        PrivateData* p_data;
    };

    class waitWidget : public QDialog 
    {
        Q_OBJECT
    public:
        waitWidget(QString text, QMovie* movie, QWidget* parent = nullptr);

    public:
        void setMovie(QMovie* movie);

        void setText(QString text);

    private:
        QLabel* ui_textLab = nullptr;
        QLabel* ui_movieLab = nullptr;
    };
}

