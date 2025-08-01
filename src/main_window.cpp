#include <Config.hpp>
#include <MainWindow.hpp>
#include <QMediaDevices>

MainWindow::MainWindow()
    : m_graphicsScene(this)
    , m_graphicsView(&m_graphicsScene, this) {
    setStyleSheet("background-color: black;");
    setMouseTracking(true);

    m_graphicsView.setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_graphicsView.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView.setFrameStyle(QFrame::NoFrame);
    m_graphicsView.setMouseTracking(true);

    m_captureSession.setVideoOutput(&m_cameraVideo);
    m_captureSession.setAudioOutput(&m_output);
    m_captureSession.setAudioInput(&m_input);
    m_captureSession.setCamera(&m_camera);

    m_graphicsScene.addItem(&m_cameraVideo);

    m_statusProxy = m_graphicsScene.addWidget(&m_status);
    m_statusProxy->setZValue(1);

    m_statusAnimation.setTargetObject(&m_status);
    m_statusAnimation.setPropertyName("geometry");
    m_statusAnimation.setDuration(200);
    m_statusAnimation.setEasingCurve(QEasingCurve::InOutCubic);

    m_statusTimer.setInterval(1500);
    m_statusTimer.setSingleShot(true);

    m_status.setAttribute(Qt::WA_StyledBackground);
    m_status.setContentsMargins(5, 5, 5, 5);
    m_status.setStyleSheet("color: white; background-color: rgba(0, 0, 0, 0.5); border-radius: 8px;");

    m_status.hide();

    updateCamera();
    updateInput();
    updateVolume();
    updatePositions();

    connect(Config::get(), &Config::preferredCameraChanged, this, &MainWindow::updateCamera);
    connect(Config::get(), &Config::preferredInputChanged, this, &MainWindow::updateInput);
    connect(Config::get(), &Config::volumeChanged, this, &MainWindow::updateVolume);

    connect(&m_camera, &QCamera::cameraDeviceChanged, [&]() { Config::get()->setPreferredCamera(m_camera.cameraDevice()); });
    connect(&m_input, &QAudioInput::deviceChanged, [&]() { Config::get()->setPreferredInput(m_input.device()); });

    connect(&m_statusTimer, &QTimer::timeout, this, &MainWindow::hideStatus);
    connect(&m_statusAnimation, &QPropertyAnimation::finished, [this]() { m_status.setVisible(m_statusAnimation.direction() != QPropertyAnimation::Backward); });
}

void MainWindow::updateStatusPositions() {
    QSize size        = m_status.sizeHint();
    QRect endPosition = QRect(width() - (size.width() + 5), height() - (size.height() + 5), size.width(), size.height());

    m_statusAnimation.setStartValue(QRect(endPosition.x(), height(), endPosition.width(), endPosition.height()));
    m_statusAnimation.setEndValue(endPosition);

    if(m_statusAnimation.state() == QPropertyAnimation::Stopped && m_statusAnimation.direction() == QPropertyAnimation::Forward) {
        m_status.setGeometry(endPosition);
    }
}

void MainWindow::setStatusVisible(bool visible) {
    m_status.show();
    updateStatusPositions();

    m_statusAnimation.setDirection(visible ? QPropertyAnimation::Forward : QPropertyAnimation::Backward);

    if(m_statusAnimation.state() != QPropertyAnimation::Running) {
        m_statusAnimation.start();
    }
}

void MainWindow::showStatus() { setStatusVisible(true); }
void MainWindow::hideStatus() { setStatusVisible(false); }

void MainWindow::setStatus(QString status) {
    m_status.setText(status);

    m_statusTimer.stop();
    m_statusTimer.start();

    if(!m_status.isVisible() || m_statusAnimation.direction() == QPropertyAnimation::Backward) {
        showStatus();
    }

    updatePositions();
}

void MainWindow::updatePositions() {
    auto size = this->size();

    QRect rect(0, 0, size.width(), size.height());
    m_graphicsView.setSceneRect(rect);
    m_graphicsView.setGeometry(rect);

    m_cameraVideo.setSize(rect.size());
    updateStatusPositions();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    QWidget::keyPressEvent(event);

    switch(event->key()) {
    case Qt::Key_Left: {
        QCameraDevice device = m_camera.cameraDevice();

        auto devices = QMediaDevices::videoInputs();
        auto it      = std::find_if(devices.begin(), devices.end(), [device](QCameraDevice input) { return device.id() == input.id(); });
        if(it == devices.end()) {
            Config::get()->setPreferredCamera(devices[0]);
            return;
        }

        size_t idx              = std::distance(devices.begin(), it);
        QCameraDevice newDevice = devices[(idx + 1) % devices.size()];

        Config::get()->setPreferredCamera(newDevice);

        break;
    }
    case Qt::Key_Right: {
        QAudioDevice device = m_input.device();

        auto devices = QMediaDevices::audioInputs();
        auto it      = std::find_if(devices.begin(), devices.end(), [device](QAudioDevice input) { return device.id() == input.id(); });
        if(it == devices.end()) {
            Config::get()->setPreferredInput(devices[0]);
            return;
        }

        size_t idx             = std::distance(devices.begin(), it);
        QAudioDevice newDevice = devices[(idx + 1) % devices.size()];

        Config::get()->setPreferredInput(newDevice);

        break;
    }
    case Qt::Key_Up:
        Config::get()->setVolume(Config::get()->volume() + 0.02);
        setStatus(QString("Volume: %1%").arg((int)(Config::get()->volume() * 100)));

        break;
    case Qt::Key_Down:
        Config::get()->setVolume(Config::get()->volume() - 0.02f);
        setStatus(QString("Volume: %1%").arg((int)(Config::get()->volume() * 100)));

        break;
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    updatePositions();
}

void MainWindow::updateCamera() {
    auto camera = Config::get()->preferredCamera();
    if(camera.isNull()) {
        auto cameras = QMediaDevices::videoInputs();
        if(cameras.size() <= 0) {
            qWarning("No camera devices detected.");

            return;
        }

        camera = cameras[0];
    }

    if(m_camera.isActive()) {
        m_camera.stop();
    }

    m_camera.setCameraDevice(camera);
    m_camera.start();

    if(!m_firstCamera) {
        setStatus(QString("Camera device: %1").arg(camera.description()));
    }
    else {
        m_firstCamera = false;
    }
}

void MainWindow::updateInput() {
    auto input = Config::get()->preferredInput();
    if(input.isNull()) {
        auto inputs = QMediaDevices::audioInputs();
        if(inputs.size() <= 0) {
            qWarning("No input devices detected.");

            return;
        }

        input = inputs[0];
    }

    m_input.setDevice(input);

    if(!m_firstInput) {
        setStatus(QString("Input device: %1").arg(input.description()));
    }
    else {
        m_firstInput = false;
    }
}

void MainWindow::updateVolume() {
    // exponential volume curve as linear feels bad
    m_output.setVolume(std::pow(Config::get()->volume(), 3));

    if(!m_firstVolume) {
        setStatus(QString("Volume: %1%").arg((int)(Config::get()->volume() * 100)));
    }
    else {
        m_firstVolume = false;
    }
}
