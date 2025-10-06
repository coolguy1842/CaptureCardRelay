#ifndef __SETTINGS_HPP__
#define __SETTINGS_HPP__

#include <SDL3/SDL.h>

#include <optional>
#include <string>
#include <unordered_map>

class Settings {
public:
    SDL_CameraID getSelectedCamera();
    void setSelectedCamera(SDL_CameraID camera);

    SDL_AudioDeviceID getSelectedRecordingDevice();
    void setSelectedRecordingDevice(SDL_AudioDeviceID recordingDevice);

    // 0 to 150
    int getVolume();
    void setVolume(int volume);

    bool isFullscreen();
    void setFullscreen(bool fullscreen = true);

    static Settings* get();
    static void close();

    ~Settings();

protected:
    std::optional<std::string> getValue(std::string key);
    void setValue(std::string key, std::string value);

    void clearValue(std::string key);

    void load();
    void save();

private:
    std::string getSettingsPath();
    std::unordered_map<std::string, std::string> m_cache;

    Settings();
};

#endif