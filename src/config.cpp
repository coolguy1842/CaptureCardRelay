#include <Config.hpp>
#include <QMediaDevices>

Config::Config()
    : m_settings("CaptureCardRelay", "settings") {
}

Config::~Config() {
}

QCameraDevice Config::preferredCamera() const {
    QVariant value = m_settings.value("preferredCamera");
    if(value.isNull() || !value.isValid()) {
        return QCameraDevice();
    }

    QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for(QCameraDevice& camera : videoInputs) {
        if(camera.description() == value) {
            return camera;
        }
    }

    return QCameraDevice();
}

void Config::setPreferredCamera(QCameraDevice camera) {
    if(camera.isNull()) {
        if(m_settings.contains("preferredCamera")) {
            m_settings.remove("preferredCamera");
            emit preferredCameraChanged(camera);
        }

        return;
    }

    if(m_settings.value("preferredCamera").value<QString>() != camera.description()) {
        m_settings.setValue("preferredCamera", camera.description());
        emit preferredCameraChanged(camera);
    }
}

QAudioDevice Config::preferredInput() const {
    QVariant value = m_settings.value("preferredInput");
    if(value.isNull() || !value.isValid()) {
        return QAudioDevice();
    }

    QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    for(QAudioDevice& input : inputs) {
        if(input.description() == value) {
            return input;
        }
    }

    return QAudioDevice();
}

void Config::setPreferredInput(QAudioDevice input) {
    if(input.isNull()) {
        if(m_settings.contains("preferredInput")) {
            m_settings.remove("preferredInput");
            emit preferredInputChanged(input);
        }

        return;
    }

    if(m_settings.value("preferredInput").value<QString>() != input.description()) {
        m_settings.setValue("preferredInput", input.description());
        emit preferredInputChanged(input);
    }
}

double clampedVolume(double volume) {
    return std::clamp(volume, 0.0, 1.0);
}

double Config::volume() const {
    return clampedVolume(m_settings.value("volume", 1.0).value<double>());
}

void Config::setVolume(double volume) {
    volume = clampedVolume(volume);
    if(m_settings.value("volume", 1.0).value<double>() == volume) {
        return;
    }

    m_settings.setValue("volume", volume);
    emit volumeChanged(volume);
}

static Config* instance = nullptr;
Config* Config::get() {
    if(instance == nullptr) {
        instance = new Config();
    }

    return instance;
}

void Config::close() {
    if(instance != nullptr) {
        return;
    }

    delete instance;
}