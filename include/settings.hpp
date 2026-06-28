#ifndef __SETTINGS_HPP__
#define __SETTINGS_HPP__

#include <SDL3/SDL.h>
#include <clay_renderer_SDL3.hpp>

#include <rocket.hpp>
#include <string>
#include <unordered_map>

#define MAX_VOLUME 150
#define MIN_VOLUME 0

#define MAX_FPS 250.0f
#define MIN_FPS 0.0f

enum FrameLimitType {
    // limits to the cameras refresh rate
    FRAME_LIMIT_CAMERA,
    FRAME_LIMIT_VSYNC,
    FRAME_LIMIT_VSYNC_ADAPTIVE,
    // limits to user specified frame rate, 0 is unlimited
    FRAME_LIMIT_FPS,
    FRAME_LIMIT_NONE,
};

struct FrameLimitInfo {
    FrameLimitType type;
    // only available if type is FPS
    float fps;
};

class Settings {
public:
    ~Settings();

    static Settings* get();
    static void close();

    SDL_CameraID getSelectedCamera();
    void setSelectedCamera(SDL_CameraID camera);

    SDL_AudioDeviceID getSelectedRecordingDevice();
    void setSelectedRecordingDevice(SDL_AudioDeviceID recordingDevice);

    // 0 to 150
    int getVolume();
    void setVolume(int volume);

    bool isFullscreen();
    void setFullscreen(bool fullscreen = true);

    CameraDisplayMode getDisplayMode();
    void setDisplayMode(CameraDisplayMode mode);

    FrameLimitInfo getFrameLimitInfo();
    void setFrameLimitInfo(FrameLimitInfo info);

public:
    rocket::thread_safe_signal<void(SDL_CameraID)> selectedCameraChanged;
    rocket::thread_safe_signal<void(SDL_AudioDeviceID)> selectedRecordingDeviceChanged;
    rocket::thread_safe_signal<void(int)> volumeChanged;
    rocket::thread_safe_signal<void(bool)> fullscreenChanged;
    rocket::thread_safe_signal<void(CameraDisplayMode)> displayModeChanged;
    rocket::thread_safe_signal<void(FrameLimitInfo)> frameLimitInfoChanged;

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