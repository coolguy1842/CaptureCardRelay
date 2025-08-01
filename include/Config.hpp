#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <QAudioDevice>
#include <QCameraDevice>
#include <QObject>
#include <QSettings>
#include <optional>

class Config : public QObject {
    Q_OBJECT;

    Q_PROPERTY(QCameraDevice preferredCamera READ preferredCamera WRITE setPreferredCamera NOTIFY preferredCameraChanged);
    Q_PROPERTY(QAudioDevice preferredInput READ preferredInput WRITE setPreferredInput NOTIFY preferredInputChanged);

    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged);

public:
    ~Config();

    static Config* get();
    static void close();

    QCameraDevice preferredCamera() const;
    void setPreferredCamera(QCameraDevice camera);

    QAudioDevice preferredInput() const;
    void setPreferredInput(QAudioDevice input);

    double volume() const;
    void setVolume(double volume);

signals:
    void preferredCameraChanged(QCameraDevice);
    void preferredInputChanged(QAudioDevice);

    void volumeChanged(double);

private:
    Config();

    QSettings m_settings;
};

#endif