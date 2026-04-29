#include "cycustomwidget.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFormLayout>
#include <QLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QValidator>




namespace CyCustomWidget
{
    //-----------------------------------------------------------------------------------------------
    //----------------------CyRangeSlider
    class CyRangeSlider::CyRangeSliderPrivate : public QObject
    {
        Q_OBJECT
    public:
        enum PressArea{
            PressOnLeftHandle,
            PressOnRightHandle,
            PressOnRangeHandle,
            PressOnBlank
        };
        explicit CyRangeSliderPrivate(CyRangeSlider* parent);
        ~CyRangeSliderPrivate();

    public:
        int validLength();
        bool setMinimumValue(int num);
        bool setMaximumValue(int num);
        bool setFirstValue(int num);
        bool setSecondValue(int num);
        bool setminimumInterval(int num);

    public:
        CyRangeSlider* m_parent;
        Options type;
        Qt::Orientation orientation;
        QColor backgrundColorEnabled;
        QColor backGroundColorDisEnable;
        QColor backGroundColor;
        QLinearGradient lineGraient;
        int sliderLeftRightMargin = 1;  // slider和widget左右间隔 1
        int sliderBarHeight = 5;        // slider宽 5
        int handleWidth = 11;           // handle宽
        float handleHeight = 11;
        bool bIsEnabled = true;

        int maxValue = 100;
        int minValue = 0;
        int firstValue = 0;
        int secondValue = 100;
        int minInterval = 1;
        int maxInterval = maxValue - minValue;

        bool useGradient = false;               // 是否使用渐变填充
        bool handleTracking = false;            // Handle移动时实时发出信号
        bool bFirstChange = false;
        bool bSecondChange = false;
        PressArea pressArea = PressOnBlank;     // 鼠标当前按下区域
        int pressPos = 0;
        int delta = 0;
    };
    CyRangeSlider::CyRangeSliderPrivate::CyRangeSliderPrivate(CyRangeSlider* parent)
    {
        m_parent = parent;
    }
    CyRangeSlider::CyRangeSliderPrivate::~CyRangeSliderPrivate()
    {

    }
    int CyRangeSlider::CyRangeSliderPrivate::validLength()
    {
        int len = (orientation == Qt::Horizontal) ? m_parent->width() : m_parent->height();
        return len - sliderLeftRightMargin * 2 - handleWidth * (type.testFlag(DoubleHandles) ? 2 : 1);
    }
    bool CyRangeSlider::CyRangeSliderPrivate::setMinimumValue(int num)
    {
        if (num >= maxValue - minInterval){
            num = maxValue - minInterval;
        }
        if (num == minValue) {
            return false;
        }
        minValue = num;
        maxInterval = maxValue - minValue;
        return true;
    }
    bool CyRangeSlider::CyRangeSliderPrivate::setMaximumValue(int num)
    {
        if (num <= minValue + minInterval){
            num = minValue + minInterval;
        }
        if (num == maxValue) {
            return false;
        } 
        maxValue = num;
        maxInterval = maxValue - minValue;
        return true;
    }
    bool CyRangeSlider::CyRangeSliderPrivate::setFirstValue(int num)
    {
        if (type == CyRangeSlider::RightHandle)
            return false;
        if (num > secondValue - minInterval){
            num = secondValue - minInterval;
        }
        if (num < minValue){
            num = minValue;
        }
        if (num == firstValue) {
            return false;
        }
        firstValue = num;
        return true;
    }
    bool CyRangeSlider::CyRangeSliderPrivate::setSecondValue(int num)
    {
        if (type == CyRangeSlider::LeftHandle)
            return false;
        if (num < firstValue + minInterval){
            num = firstValue + minInterval;
        }
        if (num > maxValue){
            num = maxValue;
        }
        if (num == secondValue) {
            return false;
        } 
        secondValue = num;
        return true;
    }
    bool CyRangeSlider::CyRangeSliderPrivate::setminimumInterval(int num)
    {
        if (type.testFlag(DoubleHandles)){
            if (num > maxInterval){
                num = maxInterval - 1;
            }
            if (num == minInterval) {
                return false;
            } 
            minInterval = num;
            return true;
        }
        return false;
    }

    CyRangeSlider::CyRangeSlider(QWidget* parent /*= nullptr*/)
        :QWidget(parent)
    {
        p_data = new CyRangeSliderPrivate(this);

        p_data->orientation = Qt::Horizontal;
        p_data->type = DoubleHandles;

        p_data->backgrundColorEnabled = QColor(0x1E, 0x90, 0xFF);
        p_data->backGroundColorDisEnable = QColor(Qt::darkGray);
        p_data->backGroundColor = p_data->backgrundColorEnabled;

        setMouseTracking(true);
    }

    CyRangeSlider::CyRangeSlider(Qt::Orientation ori, Options t /*= DoubleHandles*/, QWidget* parent /*= nullptr*/)
        :QWidget(parent)
    {
        p_data = new CyRangeSliderPrivate(this);

        p_data->orientation = ori;
        p_data->type = t;
        if (false == p_data->type.testFlag(DoubleHandles)) {
            p_data->minInterval = 0;
        }
        p_data->backgrundColorEnabled = QColor(0x1E, 0x90, 0xFF);
        p_data->backGroundColorDisEnable = QColor(Qt::darkGray);
        p_data->backGroundColor = p_data->backgrundColorEnabled;

        setMouseTracking(true);
    }

    CyRangeSlider::~CyRangeSlider()
    {

    }

    QSize CyRangeSlider::minimumSizeHint() const
    {
        if (p_data->orientation == Qt::Horizontal) {
            return QSize(p_data->handleWidth * 2 + p_data->sliderLeftRightMargin * 2, p_data->handleHeight);
        }
        else {
            return QSize(p_data->handleHeight, p_data->handleWidth * 2 + p_data->sliderLeftRightMargin * 2);
        }
    }

    QColor CyRangeSlider::selectedColor()
    {
        return p_data->backgrundColorEnabled;
    }

    int CyRangeSlider::sliderBarHeight()
    {
        return p_data->sliderBarHeight;
    }

    int CyRangeSlider::minimumValue()
    {
        return p_data->minValue;
    }

    int CyRangeSlider::maximumValue()
    {
        return p_data->maxValue;
    }

    int CyRangeSlider::firstValue()
    {
        return p_data->firstValue;
    }

    int CyRangeSlider::secondValue()
    {
        return p_data->secondValue;
    }

    int CyRangeSlider::interval()
    {
        return p_data->secondValue - p_data->firstValue;
    }

    bool CyRangeSlider::handleTrack()
    {
        return p_data->handleTracking;
    }

    bool CyRangeSlider::gradient()
    {
        return p_data->useGradient;
    }

    void CyRangeSlider::setSelectedColor(QColor color)
    {
        p_data->backgrundColorEnabled = color;
        if (isEnabled()){
            p_data->backGroundColor = p_data->backgrundColorEnabled;
            if (isVisible()) {
                update();
            }
        }
    }

    void CyRangeSlider::setSliderBarHeight(uint32_t height, float HandleZoom/* = 2.0*/)
    {
        p_data->sliderBarHeight = height;
        p_data->handleWidth = height + 2;
        p_data->handleHeight = height * HandleZoom + 1;
        if (isVisible()) {
            update();
        }
        if (p_data->orientation == Qt::Horizontal) {
            setMinimumSize(p_data->handleWidth * 2 + p_data->sliderLeftRightMargin * 2, p_data->handleHeight);
        }
        else {
            setMinimumSize(p_data->handleHeight, p_data->handleWidth * 2 + p_data->sliderLeftRightMargin * 2);
        }
    }

    void CyRangeSlider::setMinimumValue(int minNum, bool needSignal/* = true*/)
    {
        if (p_data->setMinimumValue(minNum))
        {
            bool first = p_data->setFirstValue(p_data->minValue);
            bool second = p_data->setSecondValue(p_data->maxValue);
            if (needSignal)
            {
                if (first && second){
                    emit allValueChanged(p_data->firstValue, p_data->secondValue);
                }
                else if (first){
                    emit firstValueChanged(p_data->firstValue);
                }
                else if (second){
                    emit secondValueCHanged(p_data->secondValue);
                }
            }
            if (isVisible()) {
                update();
            }
        }
    }

    void CyRangeSlider::setMaximumValue(int maxNum, bool needSignal/* = true*/)
    {
        if (p_data->setMaximumValue(maxNum))
        {
            bool first = p_data->setFirstValue(p_data->minValue);
            bool second = p_data->setSecondValue(p_data->maxValue);
            if (needSignal){
                if (first && second){
                    emit allValueChanged(p_data->firstValue, p_data->secondValue);
                }
                else if (first){
                    emit firstValueChanged(p_data->firstValue);
                }
                else if (second){
                    emit secondValueCHanged(p_data->secondValue);
                }
            }
            if (isVisible()) {
                update();
            }
        }
    }

    bool CyRangeSlider::setFirstValue(int firstNum, bool needSignal/* = true*/)
    {
        if (p_data->setFirstValue(firstNum)){
            if (needSignal){
                emit firstValueChanged(p_data->firstValue);
            }
            if (isVisible()) {
                update();
            }
            return true;
        }
        return false;
    }

    bool CyRangeSlider::setSecondValue(int secondNum, bool needSignal/* = true*/)
    {
        if (p_data->setSecondValue(secondNum)){
            if (needSignal){
                emit secondValueCHanged(p_data->secondValue);
            }
            if (isVisible()) {
                update();
            }
            return true;
        }
        return false;
    }

    void CyRangeSlider::setminimumInterval(int minInter)
    {
        if (p_data->setminimumInterval(minInter))
        {
            bool first = p_data->setFirstValue(p_data->firstValue);
            bool second = p_data->setSecondValue(p_data->secondValue);
            if (first && second){
                emit allValueChanged(p_data->firstValue, p_data->secondValue);
            }
            else if (first){
                emit firstValueChanged(p_data->firstValue);
            }
            else if (second){
                emit secondValueCHanged(p_data->secondValue);
            }
            if ((first || second) && isVisible()) {
                update();
            }
        }
    }

    void CyRangeSlider::setHandleTrack(bool tracking)
    {
        if (tracking != p_data->handleTracking) {
            p_data->handleTracking = tracking;
        }
    }

    void CyRangeSlider::setGradient(bool graident)
    {
        if (p_data->useGradient != graident) {
            p_data->useGradient = graident;
            if (p_data->useGradient) {
                auto pots = p_data->lineGraient.stops();
                if (pots.size() <= 0) {
                    p_data->lineGraient.setColorAt(0.0, Qt::black);
                    p_data->lineGraient.setColorAt(1.0, Qt::white);
                }
            }
        }
    }

    void CyRangeSlider::setGradientColor(qreal colorPos, QColor color)
    {
        p_data->lineGraient.setColorAt(colorPos, color);
    }

    void CyRangeSlider::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);
        QPainter painter(this);

        // Background
        QRectF backgroundRect;
        if (p_data->orientation == Qt::Horizontal){
            backgroundRect = QRectF(p_data->sliderLeftRightMargin, (height() - p_data->sliderBarHeight) / 2, width() - p_data->sliderLeftRightMargin * 2, p_data->sliderBarHeight);
        }
        else{
            backgroundRect = QRectF((width() - p_data->sliderBarHeight) / 2, p_data->sliderLeftRightMargin, p_data->sliderBarHeight, height() - p_data->sliderLeftRightMargin * 2);
        }
        QPen pen(Qt::gray, 0.8);
        QBrush backgroundBrush(QColor(0xD0, 0xD0, 0xD0));
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Qt4CompatiblePainting);
        painter.setBrush(backgroundBrush);
        painter.drawRoundedRect(backgroundRect, 1, 1);

        // First Handle rect
        pen.setColor(Qt::darkGray);
        pen.setWidth(0.5);
        QBrush handleBrush(QColor(0xFA, 0xFA, 0xFA));
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(handleBrush);
        QRectF fHandleRect = firstHandleRect();
        if (p_data->type.testFlag(LeftHandle)) {
            painter.drawRoundedRect(fHandleRect, 2, 2);
        }
        // Second Handle rect
        QRectF sHandleRect = secondHandleRect();
        if (p_data->type.testFlag(RightHandle))
            painter.drawRoundedRect(sHandleRect, 2, 2);

        // Range
        QRectF selectedRect(backgroundRect);
        if (p_data->orientation == Qt::Horizontal){
            selectedRect.setLeft((p_data->type.testFlag(LeftHandle) ? fHandleRect.right() : fHandleRect.left()) + 0.5);
            selectedRect.setRight((p_data->type.testFlag(RightHandle) ? sHandleRect.left() : sHandleRect.right()) - 0.5);
        }
        else{
            selectedRect.setTop((p_data->type.testFlag(LeftHandle) ? fHandleRect.bottom() : fHandleRect.top()) + 0.5);
            selectedRect.setBottom((p_data->type.testFlag(RightHandle) ? sHandleRect.top() : sHandleRect.bottom()) - 0.5);
        }
        QBrush selectedBrush(p_data->backGroundColor);
        if (p_data->bIsEnabled && p_data->useGradient) {
            p_data->lineGraient.setStart(selectedRect.topLeft());
            p_data->lineGraient.setFinalStop(selectedRect.topRight());
            selectedBrush = QBrush(p_data->lineGraient);
        }
        pen.setColor(Qt::gray);
        pen.setWidth(0.8);
        painter.setPen(pen);
        painter.setBrush(selectedBrush);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.drawRect(selectedRect);
    }

    QRectF CyRangeSlider::handleRect(int value)
    {
        if (p_data->orientation == Qt::Horizontal){
            return QRectF(value, (height() - p_data->handleHeight) / 2.0, p_data->handleWidth, p_data->handleHeight);
        }
        else{
            return QRectF((width() - p_data->handleHeight) / 2.0, value, p_data->handleHeight, p_data->handleWidth);
        }
    }

    QRectF CyRangeSlider::firstHandleRect()
    {
        float percentage = (p_data->firstValue - p_data->minValue) * 1.0 / p_data->maxInterval;
        return handleRect(percentage * p_data->validLength() + p_data->sliderLeftRightMargin);
    }

    QRectF CyRangeSlider::secondHandleRect()
    {
        float percentage = (p_data->secondValue - p_data->minValue) * 1.0 / p_data->maxInterval;
        return handleRect(percentage * p_data->validLength() + p_data->sliderLeftRightMargin + (p_data->type.testFlag(LeftHandle) ? p_data->handleWidth : 0));
    }

    void CyRangeSlider::mousePressEvent(QMouseEvent* event)
    {
        if (event->buttons() & Qt::LeftButton){
            int posValue = (p_data->orientation == Qt::Horizontal) ? event->pos().x() : event->pos().y();
            int posCheck = (p_data->orientation == Qt::Horizontal) ? event->pos().y() : event->pos().x();
            int posMax = (p_data->orientation == Qt::Horizontal) ? height() : width();
            int firstHandleRectPosValue = (p_data->orientation == Qt::Horizontal) ? firstHandleRect().x() : firstHandleRect().y();
            int secondHandleRectPosValue = (p_data->orientation == Qt::Horizontal) ? secondHandleRect().x() : secondHandleRect().y();
            p_data->pressPos = posValue;
            if (secondHandleRect().contains(event->pos()) && p_data->type.testFlag(RightHandle)){
                p_data->pressArea = CyRangeSliderPrivate::PressOnRightHandle;
                p_data->delta = posValue - (secondHandleRectPosValue + p_data->handleWidth / 2);
            }
            else if (firstHandleRect().contains(event->pos()) && p_data->type.testFlag(LeftHandle)){
                p_data->pressArea = CyRangeSliderPrivate::PressOnLeftHandle;
                p_data->delta = posValue - (firstHandleRectPosValue + p_data->handleWidth / 2);
            }
            else if (posValue > (firstHandleRectPosValue + p_data->handleWidth) && posValue < secondHandleRectPosValue && p_data->type.testFlag(DoubleHandles)){
                p_data->pressArea = CyRangeSliderPrivate::PressOnRangeHandle;
                p_data->delta = posValue - (firstHandleRectPosValue/* + p_data->handleSideLength / 2*/ + (secondHandleRectPosValue - firstHandleRectPosValue) / 2);
            }
            else{
                p_data->pressArea = CyRangeSliderPrivate::PressOnBlank;
            }
        }
        QWidget::mousePressEvent(event);
    }

    void CyRangeSlider::mouseMoveEvent(QMouseEvent* event)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            int posValue = (p_data->orientation == Qt::Horizontal) ? event->pos().x() : event->pos().y();
            //int firstHandleRectPosValue = (p_data->orientation == Qt::Horizontal) ? firstHandleRect().x() : firstHandleRect().y();
            //int secondHandleRectPosValue = (p_data->orientation == Qt::Horizontal) ? secondHandleRect().x() : secondHandleRect().y();
            int dstPos = posValue - p_data->delta;
            int dstValue = 0.0;
            bool TbFirst = false;
            bool TbSecond = false;
            if (p_data->pressArea == CyRangeSliderPrivate::PressOnLeftHandle){
                dstValue = (dstPos - p_data->sliderLeftRightMargin - p_data->handleWidth / 2) * 1.0 / p_data->validLength() * p_data->maxInterval + p_data->minValue;
                TbFirst = setFirstValue(dstValue, p_data->handleTracking);
                if (TbFirst && false == p_data->handleTracking) {
                    p_data->bFirstChange = true;
                }
            }
            else if (p_data->pressArea == CyRangeSliderPrivate::PressOnRightHandle)
            {
                dstValue = (dstPos - p_data->sliderLeftRightMargin - p_data->handleWidth / 2 - (p_data->type.testFlag(DoubleHandles) ? p_data->handleWidth : 0) * 1.0) / p_data->validLength() * p_data->maxInterval + p_data->minValue;
                TbSecond = setSecondValue(dstValue, p_data->handleTracking);
                if (TbSecond && false == p_data->handleTracking) {
                    p_data->bSecondChange = true;
                }
            }
            else if (p_data->pressArea == CyRangeSliderPrivate::PressOnRangeHandle){
                int TempInterLen = p_data->secondValue - p_data->firstValue;
                if (TempInterLen < p_data->maxInterval){
                    int leftHalfLen = TempInterLen / 2;
                    int rightHalfLen = leftHalfLen;
                    if ((TempInterLen % 2) != 0){
                        rightHalfLen += 1;
                    }
                    dstValue = (dstPos - p_data->sliderLeftRightMargin) * 1.0 / p_data->validLength() * p_data->maxInterval + p_data->minValue;
                    if (dstValue > p_data->maxValue - rightHalfLen){
                        dstValue = p_data->maxValue - rightHalfLen;
                    }
                    else if (dstValue < leftHalfLen + p_data->minValue){
                        dstValue = leftHalfLen + p_data->minValue;
                    }

                    int TfirstValue = dstValue - leftHalfLen;
                    int TsecondValue = dstValue + rightHalfLen;
                    if (dstPos < p_data->pressPos){
                        p_data->bFirstChange = p_data->setFirstValue(TfirstValue);
                        p_data->bSecondChange = p_data->setSecondValue(TsecondValue);
                    }
                    else{
                        p_data->bSecondChange = p_data->setSecondValue(TsecondValue);
                        p_data->bFirstChange = p_data->setFirstValue(TfirstValue);
                    }
                    p_data->pressPos = dstPos;
                    if (p_data->handleTracking){
                        if (p_data->bFirstChange && p_data->bSecondChange){
                            emit allValueChanged(p_data->firstValue, p_data->secondValue);
                        }
                        else if (p_data->bFirstChange){
                            emit firstValueChanged(p_data->firstValue);
                        }
                        else if (p_data->bSecondChange){
                            emit secondValueCHanged(p_data->secondValue);
                        }
                    }
                    if ((p_data->bFirstChange || p_data->bSecondChange) && isVisible()){
                        update();
                    }
                }
            }
        }
        QWidget::mouseMoveEvent(event);
    }

    void CyRangeSlider::mouseReleaseEvent(QMouseEvent* event)
    {
        if (false == p_data->handleTracking){
            if (p_data->pressArea == CyRangeSliderPrivate::PressOnLeftHandle && p_data->bFirstChange){
                emit firstValueChanged(p_data->firstValue);
            }
            else if (p_data->pressArea == CyRangeSliderPrivate::PressOnRightHandle && p_data->bSecondChange){
                emit secondValueCHanged(p_data->secondValue);
            }
            else if (p_data->pressArea == CyRangeSliderPrivate::PressOnRangeHandle){
                emit allValueChanged(p_data->firstValue, p_data->secondValue);
            }
            else {
                emit mouseRelease();
            }
        }
        else {
            emit mouseRelease();
        }
        p_data->pressArea = CyRangeSliderPrivate::PressOnBlank;
        QWidget::mouseReleaseEvent(event);
    }

    void CyRangeSlider::changeEvent(QEvent* event)
    {
        if (event->type() == QEvent::EnabledChange){
            if (isEnabled()){
                p_data->bIsEnabled = true;
                p_data->backGroundColor = p_data->backgrundColorEnabled;
            }
            else{
                p_data->bIsEnabled = false;
                p_data->backGroundColor = p_data->backGroundColorDisEnable;
            }
        }
        if (isVisible()) {
            update();
        }
    }


    //-----------------------------------------------------------------------------------------------
    //----------------------CyCheckButton
    class CyCheckButton::CyCheckButtonPrivate
    {
    public:
        CyCheckButton* parent = nullptr;
        qreal   backHeight = 24.0;              // 背景高度
        qreal   backBorder = 1.0;               // 背景边框宽度
        qreal   aspectRatio = 1.6;              // 宽高比
        qreal   backWidth = 0.0;                // 背景宽度
        qreal   handleBoder = 1.0;              // handle边框
        qreal   handleR = 0.0;                  // handle直径
        quint32 leftAndRigheMargin = 1;         // 左右间隔
        bool checked = false;                   // 选中状态
        QColor enableCheckColor;                // 启用选中时背景色
        QColor disenableCheckColor;             // 禁用时选中背景色
        QColor uncheckColor;                    // 未选中时背景色
        QColor handleColor;                     // Handle颜色
        QLinearGradient lineGraient;            // 渐变色
        bool useGradient = false;               // 是否使用渐变填充
        bool isEnabled = true;                  // 是否启用
    };

    CyCheckButton::CyCheckButton(QWidget* parent /*= nullptr*/)
        :QWidget(parent)
        , p_data(new CyCheckButtonPrivate)
    {
        p_data->backWidth = p_data->backHeight * p_data->aspectRatio;
        p_data->handleR = p_data->backHeight - (p_data->backBorder + p_data->handleBoder) * 2;


        p_data->enableCheckColor = QColor(0x1E, 0x90, 0xFF);
        p_data->disenableCheckColor = QColor(Qt::darkGray);
        p_data->uncheckColor = QColor(0xD0, 0xD0, 0xD0);
        p_data->handleColor = QColor(0xFA, 0xFA, 0xFA);
        setMaximumSize(p_data->backWidth + p_data->leftAndRigheMargin * 2,
            p_data->backHeight);
    }

    CyCheckButton::~CyCheckButton()
    {

    }

    QSize CyCheckButton::minimumSizeHint() const
    {
        return QSize(
            p_data->backWidth + p_data->leftAndRigheMargin * 2,
            p_data->backHeight
        );
    }

    bool CyCheckButton::isChecked()
    {
        return p_data->checked;
    }

    qreal CyCheckButton::slotHeight()
    {
        return p_data->backHeight;
    }

    void CyCheckButton::setSlotHeight(qreal height, qreal aspectRatio/* = 1.6*/)
    {
        if (p_data->backHeight != height || p_data->aspectRatio != aspectRatio) {
            p_data->aspectRatio = aspectRatio;
            p_data->backHeight = height;
            p_data->backWidth = p_data->backHeight * p_data->aspectRatio;
            p_data->handleR = p_data->backHeight - (p_data->backBorder + p_data->handleBoder) * 2;
            setMaximumSize(p_data->backWidth + p_data->leftAndRigheMargin * 2,
                p_data->backHeight);
            if (isVisible()) {
                update();
            }
        }
    }

    QColor CyCheckButton::enableCheckColor()
    {
        return p_data->enableCheckColor;
    }

    void CyCheckButton::setEnableCheckColor(QColor color)
    {
        p_data->enableCheckColor = color;
        if (isVisible()) {
            update();
        }
    }

    QColor CyCheckButton::disenableCheckColor()
    {
        return p_data->disenableCheckColor;
    }

    void CyCheckButton::setDisenableCheckColor(QColor color)
    {
        p_data->disenableCheckColor = color;
        if (isVisible()) {
            update();
        }
    }

    bool CyCheckButton::useGradient()
    {
        return p_data->useGradient;
    }

    void CyCheckButton::setUseGradient(bool use)
    {
        p_data->useGradient = use;
        if (p_data->useGradient) {
            auto pots = p_data->lineGraient.stops();
            if (pots.size() <= 0) {
                p_data->lineGraient.setColorAt(0.0, Qt::black);
                p_data->lineGraient.setColorAt(1.0, Qt::white);
            }
            if (isVisible() && p_data->checked) {
                update();
            }
        }
    }

    QLinearGradient CyCheckButton::gradient()
    {
        return p_data->lineGraient;
        if (p_data->checked && isVisible() && p_data->checked) {
            update();
        }
    }

    void CyCheckButton::setgradient(QLinearGradient gradient)
    {
        p_data->lineGraient = gradient;
    }

    void CyCheckButton::setChecked(bool check, bool needSignal /*= true*/)
    {
        if (check != p_data->checked) {
            p_data->checked = !p_data->checked;
            if (needSignal) {
                emit stateChanged(p_data->checked);
            }
            if (isVisible()) {
                update();
            }
        }
    }

    void CyCheckButton::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

        QPainter painter(this);
        QPen pen;
        QBrush brush;
        // Background
        QRectF backgroundRect(
            p_data->leftAndRigheMargin,
            0,
            width() - p_data->leftAndRigheMargin * 2,
            height()
        );
        qreal raidus = height() / 2;
        if (p_data->checked) {
            if (p_data->isEnabled) {
                if (p_data->useGradient) {
                    brush = QBrush(p_data->lineGraient);
                }
                else {
                    brush = QBrush(p_data->enableCheckColor);
                }
            }
            else {
                brush = QBrush(p_data->disenableCheckColor);
            }
        }
        else {
            brush = QBrush(p_data->uncheckColor);
        }
        pen.setColor(Qt::transparent);
        //pen.setColor(Qt::gray);
        pen.setWidth(p_data->backBorder);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.drawRoundedRect(backgroundRect, raidus, raidus);

        // handle
        pen.setColor(Qt::gray);
        pen.setWidth(p_data->handleBoder);
        if (p_data->isEnabled) {
            brush = QBrush(p_data->handleColor);
        }
        else {
            brush = QBrush(Qt::lightGray);
        }
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(brush);
        //painter.drawRoundedRect(handleRect(), 2, 2);
        painter.drawEllipse(handleRect());
    }

    void CyCheckButton::mousePressEvent(QMouseEvent* event)
    {
        if (event->buttons() & Qt::LeftButton) {
            p_data->checked = !p_data->checked;
            update();
            emit stateChanged(p_data->checked);
        }
        QWidget::mousePressEvent(event);
    }

    void CyCheckButton::changeEvent(QEvent* event)
    {
        if (event->type() == QEvent::EnabledChange) {
            if (isEnabled()) {
                p_data->isEnabled = true;
            }
            else {
                p_data->isEnabled = false;
            }
        }
        if (isVisible()) {
            update();
        }
    }

    QRectF CyCheckButton::handleRect()
    {
        QRectF handleRect;
        if (p_data->checked) {
            handleRect = QRectF(
                width() - p_data->leftAndRigheMargin - p_data->backBorder - p_data->handleBoder - p_data->handleR,
                p_data->backBorder + p_data->handleBoder,
                p_data->handleR,
                p_data->handleR
            );
        }
        else {
            handleRect = QRectF(
                p_data->leftAndRigheMargin + p_data->backBorder + p_data->handleBoder,
                p_data->backBorder + p_data->handleBoder,
                p_data->handleR,
                p_data->handleR
            );
        }
        return handleRect;
    }

    //-----------------------------------------------------------------------------------------------
    //----------------------SaveFileDoneDialog
    class SaveFileDoneDialog::PrivateData
    {
    public:
        QString path;
        int64_t hideTimeValue = 3500;   //ms
        int IconSize = 32;
        int TextHeight = 30;// 

        QPushButton* IconButton = nullptr;
        QLabel* Text = nullptr;
        QPushButton* OpenPathButton = nullptr;
        QTimer* hideTimer = nullptr;
        QString fontFamily = QString("Microsoft YaHei UI");
        quint32 btnTextSize = 15;
        quint32 textTextSize = 13;
    };

    SaveFileDoneDialog::SaveFileDoneDialog(QString filepath, QWidget* parent /*= nullptr*/)
        :QDialog(parent)
        ,p_data(new PrivateData)
    {
        initGUI();
        QDir dir(filepath);
        if (dir.exists()) {
            p_data->path = filepath;
        }
        else {
            p_data->path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        }
        p_data->hideTimer = new QTimer(this);
        connect(p_data->hideTimer, &QTimer::timeout, this,
            [this]() {
                p_data->hideTimer->stop();
                close();
            });
    }

    SaveFileDoneDialog::~SaveFileDoneDialog()
    {

    }

    QSize SaveFileDoneDialog::minimumSizeHint() const
    {
        int width = p_data->IconButton->minimumWidth() + p_data->Text->minimumWidth() + p_data->OpenPathButton->minimumWidth();
        return QSize(width + 20, p_data->TextHeight * 2 + 20);
    }

    QString SaveFileDoneDialog::path()
    {
        return p_data->path;
    }

    int64_t SaveFileDoneDialog::autoHiadeTime()
    {
        return p_data->hideTimeValue;
    }

    bool SaveFileDoneDialog::setPath(QString path)
    {
        QDir dir(path);
        if (dir.exists()) {
            p_data->path = path;
            return true;
        }
        else if (dir.mkpath(path)){
            p_data->path = path;
            return true;
        }
        return false;
    }

    void SaveFileDoneDialog::setText(QString text)
    {
        if (false == text.isEmpty()) {
            p_data->Text->setText(text);
            auto textFont = p_data->Text->font();
            textFont.setPointSize(p_data->textTextSize);
            textFont.setFamily(p_data->fontFamily);
            QFontMetrics fm2(textFont);
            int textWidth = fm2.width(p_data->Text->text());
            p_data->Text->setMaximumSize(textWidth + 6, p_data->TextHeight);
            p_data->Text->setMinimumSize(textWidth + 6, p_data->TextHeight);
            int width = p_data->IconButton->minimumWidth() + p_data->Text->minimumWidth() + p_data->OpenPathButton->minimumWidth();
            setMinimumSize(width + 20, p_data->IconSize + 40);
        }
    }

    void SaveFileDoneDialog::setIcon(QIcon icon)
    {
        p_data->IconButton->setIcon(icon);
    }

    void SaveFileDoneDialog::setButtonTextColor(QColor color)
    {
        QPalette pa = p_data->OpenPathButton->palette();
        pa.setColor(QPalette::ButtonText, color);
        p_data->OpenPathButton->setPalette(pa);
    }

    void SaveFileDoneDialog::setAutoHideTime(int64_t ms)
    {
        if (ms < 100)
            ms = 100;
        p_data->hideTimeValue = ms;
    }

    void SaveFileDoneDialog::flushHideTimer()
    {
        p_data->hideTimer->start(p_data->hideTimeValue);
    }

    void SaveFileDoneDialog::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        if (false == p_data->hideTimer->isActive()) {
            p_data->hideTimer->start(p_data->hideTimeValue);
        } 
    }

    void SaveFileDoneDialog::hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);
        if (p_data->hideTimer->isActive()) {
            p_data->hideTimer->stop();
        } 
        setVisible(false);
    }

    void SaveFileDoneDialog::initGUI()
    {
        setWindowFlags(Qt::FramelessWindowHint);
        setStyleSheet(
            QString(
            "QWidget\n"
            "{\n"
            "   background-color:rgba(0,0,0,150);\n"
            "}\n"
            "\n"
            "QPushButton\n"
            "{\n"
            "   font-family:%1;\n"
            "   font-size:%2;\n"
            "   border:none;\n"
            "   background-color:rgba(0,0,0,0);\n"
            "}"
            "\n"
            "QLabel\n"
            "{"
            "   font-family:%1;\n"
            "   font-size:%3;\n"
            "   color:white;\n"
            "   border:none;\n"
            "   background-color:rgba(0,0,0,0);\n"
            "}\n"
            )
            .arg(p_data->fontFamily)
            .arg(p_data->btnTextSize)
            .arg(p_data->textTextSize)
        );
        /*setAutoFillBackground(true);
        setWindowOpacity(0.3);*/
        QFont ft;
        ft.setFamily("p_data->fontFamily");
        ft.setPointSize(p_data->btnTextSize);
        ft.setUnderline(true);
        p_data->OpenPathButton = new QPushButton(this);
        setButtonTextColor(QColor("#2AA3C6"));
        p_data->OpenPathButton->setFont(ft);
        p_data->OpenPathButton->setText(tr("View"));
        p_data->OpenPathButton->setFlat(true);
        QFontMetrics fm(ft);
        int textWidth = fm.width(p_data->OpenPathButton->text());
        p_data->OpenPathButton->setMaximumSize(textWidth + 6, p_data->TextHeight);
        p_data->OpenPathButton->setMinimumSize(textWidth + 6, p_data->TextHeight);
        connect(p_data->OpenPathButton, &QPushButton::clicked, this, 
            [this]()
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(p_data->path));
                if (p_data->hideTimer->isActive()) {
                    p_data->hideTimer->stop();
                }
                close();
            });
        p_data->IconButton = new QPushButton(this);
        p_data->IconButton->setFlat(true);
        p_data->IconButton->setIconSize(QSize(p_data->IconSize, p_data->IconSize));
        p_data->IconButton->setMaximumSize(p_data->IconSize + 6, p_data->IconSize + 6);
        p_data->IconButton->setMinimumSize(p_data->IconSize + 6, p_data->IconSize + 6);

        p_data->Text = new QLabel(this);
        ft.setPointSize(p_data->textTextSize);
        ft.setUnderline(false);
        p_data->Text->setFont(ft);
        p_data->Text->setText(tr("File Saved!"));
        p_data->Text->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        QFontMetrics fm2(ft);
        textWidth = fm2.width(p_data->Text->text());
        p_data->Text->setMaximumSize(textWidth + 6, p_data->TextHeight);
        p_data->Text->setMinimumSize(textWidth + 6, p_data->TextHeight);

        QHBoxLayout* mainLayout = new QHBoxLayout(this);
        mainLayout->setSpacing(3);
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addWidget(p_data->IconButton);
        mainLayout->addWidget(p_data->Text);
        mainLayout->addWidget(p_data->OpenPathButton);

        int width = p_data->IconButton->minimumWidth() + p_data->Text->minimumWidth() + p_data->OpenPathButton->minimumWidth();
        setMinimumSize(width + 20, p_data->IconSize + 40);
    }


    /*********************************************class CyDisBackGround;*/
    waitWidget::waitWidget(QString text, QMovie* movie, QWidget* parent /*= nullptr*/)
        :QDialog(parent)
    {
        QVBoxLayout* mainLoayout = new QVBoxLayout(this);
        setLayout(mainLoayout);

        ui_movieLab = new QLabel(this);
        ui_movieLab->setMovie(movie);
        mainLoayout->addWidget(ui_movieLab);

        ui_textLab = new QLabel(this);
        ui_textLab->setText(text);
        mainLoayout->addWidget(ui_textLab);

        setWindowOpacity(0.8);

        //取消对话框模式
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        ui_movieLab->setStyleSheet("background-color:transparent");
        ui_movieLab->setScaledContents(true);
        movie->start();
    }

    void waitWidget::setMovie(QMovie* movie)
    {
        ui_movieLab->setMovie(movie);
    }

    void waitWidget::setText(QString text)
    {
        ui_textLab->setText(text);
    }

}

#include "cycustomwidget.moc"