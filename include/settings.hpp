#ifndef __SETTINGS_HPP__
#define __SETTINGS_HPP__

#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
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

enum PixelFormat {
    // follows the cameras format, may not be compatible with your GPU
    PIXEL_FORMAT_CAMERA = SDL_PIXELFORMAT_UNKNOWN,
    PIXEL_FORMAT_RGB24  = SDL_PIXELFORMAT_RGB24,
};

struct FrameLimitInfo {
    FrameLimitType type;
    // only available if type is FPS
    float fps;
};

const char* pixelFormatName(SDL_PixelFormat format);
const char* colorspaceName(SDL_Colorspace colorspace);

class Settings {
public:
    Settings(const char* manualFile = NULL);
    ~Settings();

    bool canSetSelectedCamera() const;
    SDL_CameraID getSelectedCamera();
    void setSelectedCamera(SDL_CameraID camera);

    bool canSetRecordingDevice() const;
    SDL_AudioDeviceID getSelectedRecordingDevice();
    void setSelectedRecordingDevice(SDL_AudioDeviceID recordingDevice);

    bool canSetDisplayMode() const;
    CameraDisplayMode getDisplayMode();
    void setDisplayMode(CameraDisplayMode mode);

    // display format, camera is fastest, but may have issues, defaults to RGB24
    bool canSetPixelFormat() const;
    PixelFormat getPixelFormat();
    void setPixelFormat(PixelFormat format);

    bool canSetFrameLimitType() const;
    bool canSetFrameLimitFPS() const;
    FrameLimitInfo getFrameLimitInfo();
    void setFrameLimitInfo(FrameLimitInfo info);

    bool canSetFullscreen() const;
    bool isFullscreen();
    void setFullscreen(bool fullscreen = true);

    // 0 to 150
    bool canSetVolume() const;
    int getVolume();
    void setVolume(int volume);

    // cameras preferred specs, not what the display texture uses, internal settings that user can use from changing the file
    // colorspace has higher priority than format
    // SDL_COLORSPACE_UNKNOWN is equal to unset, and will not bias the program
    bool canSetPreferredColorspace() const;
    SDL_Colorspace getPreferredColorspace();
    void setPreferredColorspace(SDL_Colorspace colorspace);

    // SDL_PIXELFORMAT_UNKNOWN is equal to unset, and will not bias the program
    bool canSetPreferredPixelFormat() const;
    SDL_PixelFormat getPreferredPixelFormat();
    void setPreferredPixelFormat(SDL_PixelFormat colorspace);

public:
    rocket::thread_safe_signal<void(SDL_CameraID)> selectedCameraChanged;
    rocket::thread_safe_signal<void(SDL_AudioDeviceID)> selectedRecordingDeviceChanged;
    rocket::thread_safe_signal<void(int)> volumeChanged;
    rocket::thread_safe_signal<void(bool)> fullscreenChanged;
    rocket::thread_safe_signal<void(CameraDisplayMode)> displayModeChanged;
    rocket::thread_safe_signal<void(FrameLimitInfo)> frameLimitInfoChanged;
    rocket::thread_safe_signal<void(PixelFormat)> pixelFormatChanged;
    rocket::thread_safe_signal<void(SDL_Colorspace)> preferredColorspaceChanged;
    rocket::thread_safe_signal<void(SDL_PixelFormat)> preferredPixelFormatChanged;

protected:
    std::optional<std::string> getValue(std::string key);
    // if the value was locked then false is returned
    bool setValue(std::string key, std::string value);
    bool valueLocked(std::string key) const;

    // same as setValue
    bool clearValue(std::string key);

    void loadLocked(std::string path);
    void load();

    void save();

private:
    std::string getSettingsPath();

    // priority is given to locked, locked options can't change
    std::unordered_map<std::string, std::string> m_lockedCache;
    std::unordered_map<std::string, std::string> m_cache;
};

#endif