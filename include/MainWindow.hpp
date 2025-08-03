#ifndef __MAIN_WINDOW_HPP__
#define __MAIN_WINDOW_HPP__

#include <QAudioDevice>
#include <QAudioInput>
#include <QAudioOutput>
#include <QCamera>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLabel>
#include <QMediaCaptureSession>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVideoWidget>
#include <QWidget>

class CustomGraphicsView : public QGraphicsView {
    Q_OBJECT;

public:
    using QGraphicsView::QGraphicsView;

protected:
    virtual void mouseMoveEvent(QMouseEvent* event);

signals:
    void onMouseMove(QPoint point);
};

class MainWindow : public QWidget {
    Q_OBJECT;

public:
    MainWindow();

    void setStatus(QString status);

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void resizeEvent(QResizeEvent* event);

    void updateStatusPositions();

    void setStatusVisible(bool visible);
    void showStatus();
    void hideStatus();

private slots:
    void updateCamera();
    void updateInput();
    void updateVolume();

    void updatePositions();

    void handleMouseMove(QPoint pos);

private:
    QMediaCaptureSession m_captureSession;

    QGraphicsScene m_graphicsScene;
    QGraphicsVideoItem m_cameraVideo;
    CustomGraphicsView m_graphicsView;

    QCamera m_camera;
    QAudioInput m_input;
    QAudioOutput m_output;

    QGraphicsProxyWidget* m_statusProxy;
    QPropertyAnimation m_statusAnimation;
    QTimer m_statusTimer;
    QLabel m_status;

    QTimer m_cursorHideTimer;

    bool m_firstCamera        = true;
    bool m_firstInput         = true;
    bool m_firstVolume        = true;
    bool m_overlayBrushActive = false;
};

#endif