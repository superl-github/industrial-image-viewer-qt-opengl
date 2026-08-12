#pragma once

#include <QtCore/qglobal.h>
#include <QWidget>
#include <QSlider>

class CyPlaySlider : public QWidget {
    Q_OBJECT
public:
    CyPlaySlider(QWidget* parent = nullptr);
    ~CyPlaySlider();

signals:
    void valueChange(int value);
    void sliderMoved(int value);
    void sliderDrag(int value);
    void sliderRelease();

public:
    void setValue(int value);
    int value();

    void setTimeLableVisible(bool visio);
    bool timeLableisVisible();

    void setRate(int Rate);
    int Rate();


    void setRange(int min, int max);
    int maxmum();
    int minimum();

    void setHandleTracking(bool tracking);
    bool HandleTrack();

    void setTickInterval(int ti);
    int tickInterval();

    
    

private:
    void handleJump(int value);
    void handleMoved(int value);
    void handleJumpDone();

protected:
    class PrivateData;
    PrivateData* d;
};

class CustomSlider : public QSlider
{
    Q_OBJECT
public:
    CustomSlider(QWidget* parent = nullptr);
    ~CustomSlider();

signals:
    void handleJump(int value);
    void handleJumpDone();
    void handleMoved(int value);

public:
    void setHandleTrack(bool Track);
    bool HandleTrack() { return m_HandleTrack; };

protected:
    void mousePressEvent(QMouseEvent* e)override;
    void mouseMoveEvent(QMouseEvent* e)override;
    void mouseReleaseEvent(QMouseEvent* e)override;

private:
    bool m_HandleTrack;
    int m_LastMoveValue = -1;
};