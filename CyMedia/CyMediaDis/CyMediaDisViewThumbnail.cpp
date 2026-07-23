#include "CyMediaDisViewThumbnail.h"
#include "CyMediaDisView.h"
#include "CyMediaCalc/CyMediaCalc.h"

static const int THUMBNAIL_MIN_SIZE = 120;      // 最小边长

CyMediaDisViewThumbnail::CyMediaDisViewThumbnail(CyMediaDisView* parentView, QWidget* parent /*= nullptr*/)
    : QWidget(parent)
    , m_parentView(parentView) {
}

CyMediaDisViewThumbnail::~CyMediaDisViewThumbnail() {

}

void CyMediaDisViewThumbnail::setScene(QGraphicsScene* scene) {
    m_scene = scene;
}

void CyMediaDisViewThumbnail::setViewRect(const QRectF& rect) {
    mViewRect = rect;
    update();
}

bool CyMediaDisViewThumbnail::isBeingDragged() {
    return mDragging;
}

void CyMediaDisViewThumbnail::setThumbnailSize(const QSize& size) {
    resize(size);
}

void CyMediaDisViewThumbnail::upBackImage(CyMedia::ImageShowInfo info, uint8_t* data) {
    QImage image;
    if (info.isBayer()) {
        uint32_t rgbLen = info.width * info.height * 3;
        if (info.bit > 8 && info.bit <= 16) {
            rgbLen *= 2;
        }
        uint8_t* rgbData = new uint8_t[rgbLen];
        CyMediaCalc::bayer2RGB(info, data, rgbData);
        image = QImage(info.width, info.height, QImage::Format_RGB888);
        auto pImage = image.bits();

        if (info.bit <= 8) {
            memcpy(pImage, rgbData, rgbLen);
        }
        else if (info.bit <= 16) {
            float maxPixel = (1 << info.bit) - 1;
            for (int y = 0; y < info.height; y++) {
                for (int x = 0; x < info.width; x++) {
                    for (int p = 0; p < 3; p++) {
                        pImage[(y * info.width + x) * 3 + p] = (rgbData[(y * info.width + x) * 3 + p] * 255) / maxPixel;
                    }
                }
            }
        }
    }
    else if (info.isYUV()) {
        uint32_t rgbLen = info.width * info.height * 3;
        uint8_t* rgbData = new uint8_t[rgbLen];
        CyMediaCalc::YUV2RGB(info, data, rgbData);
        image = QImage(info.width, info.height, QImage::Format_RGB888);
        auto pImage = image.bits();
        memcpy(pImage, rgbData, rgbLen);
    }
    else {
        switch (info.format) {
            case CyMedia::MONO: {
                // 单色图像
                if (info.bit == 8) {
                    image = QImage(data, info.width, info.height, QImage::Format_Grayscale8);
                }
                else if (info.bit <= 16) {
                    uint16_t* pImage = (uint16_t*)data;
                    float maxPixel = (1 << info.bit) - 1;
                    image = QImage(info.width, info.height, QImage::Format_Grayscale8);
                    for (int y = 0; y < info.height; y++) {
                        for (int x = 0; x < info.width; x++) {
                            image.bits()[y * info.width + x] = (pImage[y * info.width + x] * 255) / maxPixel;
                        }
                    }
                }
                else if (info.bit < 32) {
                    uint32_t* pImage = (uint32_t*)data;
                    float maxPixel = (1 << info.bit) - 1;
                    image = QImage(info.width, info.height, QImage::Format_Grayscale8);
                    for (int y = 0; y < info.height; y++) {
                        for (int x = 0; x < info.width; x++) {
                            image.bits()[y * info.width + x] = (pImage[y * info.width + x] * 255) / maxPixel;
                        }
                    }
                }
            }break;
            case CyMedia::RGB: {
                // RGB图像
                if (info.bit == 8) {
                    image = QImage(data, info.width, info.height, QImage::Format_RGB888);
                }
                else if (info.bit == 16) {
                    // 处理16位RGB图像
                    uint16_t* pImage = (uint16_t*)data;
                    float maxPixel = (1 << info.bit) - 1;
                    image = QImage(info.width, info.height, QImage::Format_RGB888);
                    for (int y = 0; y < info.height; y++) {
                        for (int x = 0; x < info.width; x++) {
                            for (int p = 0; p < 3; p++) {
                                image.bits()[(y * info.width + x) * 3 + p] = (pImage[(y * info.width + x) * 3 + p] * 255) / maxPixel;
                            }
                        }
                    }
                }
            }break;
            case CyMedia::RGBA: {
                // RGBA图像
                if (info.bit == 8) {
                    image = QImage(data, info.width, info.height, QImage::Format_RGBA8888);
                }
                else if (info.bit == 16) {
                    // 处理16位RGB图像
                    image = QImage(info.width, info.height, QImage::Format_RGBA64);
                    memcpy(image.bits(), data, info.width * info.height * 2);
                }
            }break;
        }
    }

    // 如果图像创建成功，设置为缩略图
    if (false == image.isNull()) {
        mBackImage = image;
        // 缩放到合适的大小，保持宽高比
        int thumbSize = THUMBNAIL_MIN_SIZE;
        if (image.width() > image.height()) {
            int newHeight = thumbSize * image.height() / image.width();
            mBackImage = mBackImage.scaled(thumbSize, newHeight, Qt::KeepAspectRatio);
        }
        else {
            int newWidth = thumbSize * image.width() / image.height();
            mBackImage = mBackImage.scaled(newWidth, thumbSize, Qt::KeepAspectRatio);
        }
        //update(); // 重绘缩略图
    }
}

QImage& CyMediaDisViewThumbnail::backImage() {
    return mBackImage;
}

void CyMediaDisViewThumbnail::setBackgroundColor(QColor color) {
    mBackGroundColor = color;
}

void CyMediaDisViewThumbnail::setSelectColor(QColor color) {
    color.setAlpha(0xFF);
    mSelectRectColor = color;

    color.setAlpha(mSelectRectColor_transparent.alpha());
    mSelectRectColor_transparent = color;
}

bool CyMediaDisViewThumbnail::drawBorder() {
    return mDrawColor;
}

void CyMediaDisViewThumbnail::setDrawBorder(bool draw) {
    mDrawColor = draw;
}

QColor CyMediaDisViewThumbnail::borderColor() {
    return mBorderColor;
}

void CyMediaDisViewThumbnail::setBorderColor(QColor color) {
    mBorderColor = color;
}

void CyMediaDisViewThumbnail::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    // 绘制场景缩略图
    if (m_scene) {
        // 绘制背景
        if (mBackImage.isNull()) {
            painter.setBrush(mBackGroundColor);
            painter.setPen(Qt::transparent);
            painter.drawRect(rect());
        }
        else {
            painter.drawImage(rect(), mBackImage);
        }

        // 绘制控制矩形
        QRectF drawRect = getDrawRect();
        painter.setBrush(mSelectRectColor_transparent);
        painter.setPen(mSelectRectColor);
        painter.drawRect(drawRect);
    }
    //绘制边框
    if (mDrawColor) {
        painter.setPen(QPen(mBorderColor, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(this->rect());
    }
}

void CyMediaDisViewThumbnail::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_parentView) {
        QRectF drawRect = getDrawRect();
        if (drawRect.contains(event->pos())) {
            mDragging = true;
            setCursor(Qt::ClosedHandCursor);
            // 计算偏移比例（相对于 drawRect 左上角）
            mClickOffsetRatio = QPointF(
                (event->x() - drawRect.x()) / drawRect.width(),
                (event->y() - drawRect.y()) / drawRect.height()
            );
            // 钳制到 [0,1]
            mClickOffsetRatio.setX(qBound(0.0, mClickOffsetRatio.x(), 1.0));
            mClickOffsetRatio.setY(qBound(0.0, mClickOffsetRatio.y(), 1.0));
        }
    }
    QWidget::mousePressEvent(event);
}

void CyMediaDisViewThumbnail::mouseMoveEvent(QMouseEvent* event) {
    if (mDragging && m_parentView && m_parentView->scene()) {
        QRectF drawRect = getDrawRect();
        // 计算期望的 drawRect 左上角（保持偏移不变）
        double newDrawX = event->x() - mClickOffsetRatio.x() * drawRect.width();
        double newDrawY = event->y() - mClickOffsetRatio.y() * drawRect.height();
        // 不裁剪边界，让滚动条自动钳制（允许超出）
        double xRatio = newDrawX / width();
        double yRatio = newDrawY / height();
        // 通知主视图更新视口
        m_parentView->setViewFromThumbnailPos(xRatio, yRatio);
    }
    QWidget::mouseMoveEvent(event);
}

void CyMediaDisViewThumbnail::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mDragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseReleaseEvent(event);
}

QRectF CyMediaDisViewThumbnail::getDrawRect() const {
    if (mViewRect.isEmpty())
        return mViewRect;

    QRectF drawRect = mViewRect;
    qreal w = drawRect.width();
    qreal h = drawRect.height();
    if (w < MIN_RECT_SIZE || h < MIN_RECT_SIZE) {
        qreal scale = qMax(MIN_RECT_SIZE / w, MIN_RECT_SIZE / h);
        QPointF center = drawRect.center();
        drawRect = QRectF(center.x() - w * scale / 2, center.y() - h * scale / 2,
            w * scale, h * scale);
    }
    return drawRect;
}
