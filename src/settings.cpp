#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <settings.hpp>
#include <string>
#include <unordered_map>

const CameraDisplayMode DEFAULT_DISPLAY_MODE = DISPLAY_MODE_CONTAIN;

const FrameLimitType DEFAULT_FRAME_LIMIT_TYPE = FRAME_LIMIT_CAMERA;
const float DEFAULT_FRAME_LIMIT_FPS           = 0.0f;

const PixelFormat DEFAULT_PIXEL_FORMAT = PIXEL_FORMAT_RGB24;

constexpr uint64_t hash(std::string_view str) {
    uint64_t hash = 0;
    for(char c : str) {
        hash = (hash * 131) + static_cast<uint64_t>(c);
    }
    return hash;
}

constexpr uint64_t operator""_hash(const char* str, size_t len) {
    return hash(std::string_view(str, len));
}

const char* pixelFormatName(SDL_PixelFormat format) {
    switch(format) {
    case SDL_PIXELFORMAT_UNKNOWN:       return "SDL_PIXELFORMAT_UNKNOWN";
    case SDL_PIXELFORMAT_INDEX1LSB:     return "SDL_PIXELFORMAT_INDEX1LSB";
    case SDL_PIXELFORMAT_INDEX1MSB:     return "SDL_PIXELFORMAT_INDEX1MSB";
    case SDL_PIXELFORMAT_INDEX2LSB:     return "SDL_PIXELFORMAT_INDEX2LSB";
    case SDL_PIXELFORMAT_INDEX2MSB:     return "SDL_PIXELFORMAT_INDEX2MSB";
    case SDL_PIXELFORMAT_INDEX4LSB:     return "SDL_PIXELFORMAT_INDEX4LSB";
    case SDL_PIXELFORMAT_INDEX4MSB:     return "SDL_PIXELFORMAT_INDEX4MSB";
    case SDL_PIXELFORMAT_INDEX8:        return "SDL_PIXELFORMAT_INDEX8";
    case SDL_PIXELFORMAT_RGB332:        return "SDL_PIXELFORMAT_RGB332";
    case SDL_PIXELFORMAT_XRGB4444:      return "SDL_PIXELFORMAT_XRGB4444";
    case SDL_PIXELFORMAT_XBGR4444:      return "SDL_PIXELFORMAT_XBGR4444";
    case SDL_PIXELFORMAT_XRGB1555:      return "SDL_PIXELFORMAT_XRGB1555";
    case SDL_PIXELFORMAT_XBGR1555:      return "SDL_PIXELFORMAT_XBGR1555";
    case SDL_PIXELFORMAT_ARGB4444:      return "SDL_PIXELFORMAT_ARGB4444";
    case SDL_PIXELFORMAT_RGBA4444:      return "SDL_PIXELFORMAT_RGBA4444";
    case SDL_PIXELFORMAT_ABGR4444:      return "SDL_PIXELFORMAT_ABGR4444";
    case SDL_PIXELFORMAT_BGRA4444:      return "SDL_PIXELFORMAT_BGRA4444";
    case SDL_PIXELFORMAT_ARGB1555:      return "SDL_PIXELFORMAT_ARGB1555";
    case SDL_PIXELFORMAT_RGBA5551:      return "SDL_PIXELFORMAT_RGBA5551";
    case SDL_PIXELFORMAT_ABGR1555:      return "SDL_PIXELFORMAT_ABGR1555";
    case SDL_PIXELFORMAT_BGRA5551:      return "SDL_PIXELFORMAT_BGRA5551";
    case SDL_PIXELFORMAT_RGB565:        return "SDL_PIXELFORMAT_RGB565";
    case SDL_PIXELFORMAT_BGR565:        return "SDL_PIXELFORMAT_BGR565";
    case SDL_PIXELFORMAT_RGB24:         return "SDL_PIXELFORMAT_RGB24";
    case SDL_PIXELFORMAT_BGR24:         return "SDL_PIXELFORMAT_BGR24";
    case SDL_PIXELFORMAT_XRGB8888:      return "SDL_PIXELFORMAT_XRGB8888";
    case SDL_PIXELFORMAT_RGBX8888:      return "SDL_PIXELFORMAT_RGBX8888";
    case SDL_PIXELFORMAT_XBGR8888:      return "SDL_PIXELFORMAT_XBGR8888";
    case SDL_PIXELFORMAT_BGRX8888:      return "SDL_PIXELFORMAT_BGRX8888";
    case SDL_PIXELFORMAT_ARGB8888:      return "SDL_PIXELFORMAT_ARGB8888";
    case SDL_PIXELFORMAT_RGBA8888:      return "SDL_PIXELFORMAT_RGBA8888";
    case SDL_PIXELFORMAT_ABGR8888:      return "SDL_PIXELFORMAT_ABGR8888";
    case SDL_PIXELFORMAT_BGRA8888:      return "SDL_PIXELFORMAT_BGRA8888";
    case SDL_PIXELFORMAT_XRGB2101010:   return "SDL_PIXELFORMAT_XRGB2101010";
    case SDL_PIXELFORMAT_XBGR2101010:   return "SDL_PIXELFORMAT_XBGR2101010";
    case SDL_PIXELFORMAT_ARGB2101010:   return "SDL_PIXELFORMAT_ARGB2101010";
    case SDL_PIXELFORMAT_ABGR2101010:   return "SDL_PIXELFORMAT_ABGR2101010";
    case SDL_PIXELFORMAT_RGB48:         return "SDL_PIXELFORMAT_RGB48";
    case SDL_PIXELFORMAT_BGR48:         return "SDL_PIXELFORMAT_BGR48";
    case SDL_PIXELFORMAT_RGBA64:        return "SDL_PIXELFORMAT_RGBA64";
    case SDL_PIXELFORMAT_ARGB64:        return "SDL_PIXELFORMAT_ARGB64";
    case SDL_PIXELFORMAT_BGRA64:        return "SDL_PIXELFORMAT_BGRA64";
    case SDL_PIXELFORMAT_ABGR64:        return "SDL_PIXELFORMAT_ABGR64";
    case SDL_PIXELFORMAT_RGB48_FLOAT:   return "SDL_PIXELFORMAT_RGB48_FLOAT";
    case SDL_PIXELFORMAT_BGR48_FLOAT:   return "SDL_PIXELFORMAT_BGR48_FLOAT";
    case SDL_PIXELFORMAT_RGBA64_FLOAT:  return "SDL_PIXELFORMAT_RGBA64_FLOAT";
    case SDL_PIXELFORMAT_ARGB64_FLOAT:  return "SDL_PIXELFORMAT_ARGB64_FLOAT";
    case SDL_PIXELFORMAT_BGRA64_FLOAT:  return "SDL_PIXELFORMAT_BGRA64_FLOAT";
    case SDL_PIXELFORMAT_ABGR64_FLOAT:  return "SDL_PIXELFORMAT_ABGR64_FLOAT";
    case SDL_PIXELFORMAT_RGB96_FLOAT:   return "SDL_PIXELFORMAT_RGB96_FLOAT";
    case SDL_PIXELFORMAT_BGR96_FLOAT:   return "SDL_PIXELFORMAT_BGR96_FLOAT";
    case SDL_PIXELFORMAT_RGBA128_FLOAT: return "SDL_PIXELFORMAT_RGBA128_FLOAT";
    case SDL_PIXELFORMAT_ARGB128_FLOAT: return "SDL_PIXELFORMAT_ARGB128_FLOAT";
    case SDL_PIXELFORMAT_BGRA128_FLOAT: return "SDL_PIXELFORMAT_BGRA128_FLOAT";
    case SDL_PIXELFORMAT_ABGR128_FLOAT: return "SDL_PIXELFORMAT_ABGR128_FLOAT";
    case SDL_PIXELFORMAT_YV12:          return "SDL_PIXELFORMAT_YV12";
    case SDL_PIXELFORMAT_IYUV:          return "SDL_PIXELFORMAT_IYUV";
    case SDL_PIXELFORMAT_YUY2:          return "SDL_PIXELFORMAT_YUY2";
    case SDL_PIXELFORMAT_UYVY:          return "SDL_PIXELFORMAT_UYVY";
    case SDL_PIXELFORMAT_YVYU:          return "SDL_PIXELFORMAT_YVYU";
    case SDL_PIXELFORMAT_NV12:          return "SDL_PIXELFORMAT_NV12";
    case SDL_PIXELFORMAT_NV21:          return "SDL_PIXELFORMAT_NV21";
    case SDL_PIXELFORMAT_P010:          return "SDL_PIXELFORMAT_P010";
    case SDL_PIXELFORMAT_EXTERNAL_OES:  return "SDL_PIXELFORMAT_EXTERNAL_OES";
    case SDL_PIXELFORMAT_MJPG:          return "SDL_PIXELFORMAT_MJPG";
    default:                            return "SDL_PIXELFORMAT_UNKNOWN";
    }
}

const char* colorspaceName(SDL_Colorspace colorspace) {
    switch(colorspace) {
    case SDL_COLORSPACE_SRGB:           return "SDL_COLORSPACE_SRGB";
    case SDL_COLORSPACE_SRGB_LINEAR:    return "SDL_COLORSPACE_SRGB_LINEAR";
    case SDL_COLORSPACE_HDR10:          return "SDL_COLORSPACE_HDR10";
    case SDL_COLORSPACE_JPEG:           return "SDL_COLORSPACE_JPEG";
    case SDL_COLORSPACE_BT601_LIMITED:  return "SDL_COLORSPACE_BT601_LIMITED";
    case SDL_COLORSPACE_BT601_FULL:     return "SDL_COLORSPACE_BT601_FULL";
    case SDL_COLORSPACE_BT709_LIMITED:  return "SDL_COLORSPACE_BT709_LIMITED";
    case SDL_COLORSPACE_BT709_FULL:     return "SDL_COLORSPACE_BT709_FULL";
    case SDL_COLORSPACE_BT2020_LIMITED: return "SDL_COLORSPACE_BT2020_LIMITED";
    case SDL_COLORSPACE_BT2020_FULL:    return "SDL_COLORSPACE_BT2020_FULL";
    default:                            return "SDL_COLORSPACE_UNKNOWN";
    }
}

SDL_PixelFormat pixelFormatFromName(const char* name) {
    switch(hash(name)) {
    case "SDL_PIXELFORMAT_UNKNOWN"_hash:       return SDL_PIXELFORMAT_UNKNOWN;
    case "SDL_PIXELFORMAT_INDEX1LSB"_hash:     return SDL_PIXELFORMAT_INDEX1LSB;
    case "SDL_PIXELFORMAT_INDEX1MSB"_hash:     return SDL_PIXELFORMAT_INDEX1MSB;
    case "SDL_PIXELFORMAT_INDEX2LSB"_hash:     return SDL_PIXELFORMAT_INDEX2LSB;
    case "SDL_PIXELFORMAT_INDEX2MSB"_hash:     return SDL_PIXELFORMAT_INDEX2MSB;
    case "SDL_PIXELFORMAT_INDEX4LSB"_hash:     return SDL_PIXELFORMAT_INDEX4LSB;
    case "SDL_PIXELFORMAT_INDEX4MSB"_hash:     return SDL_PIXELFORMAT_INDEX4MSB;
    case "SDL_PIXELFORMAT_INDEX8"_hash:        return SDL_PIXELFORMAT_INDEX8;
    case "SDL_PIXELFORMAT_RGB332"_hash:        return SDL_PIXELFORMAT_RGB332;
    case "SDL_PIXELFORMAT_XRGB4444"_hash:      return SDL_PIXELFORMAT_XRGB4444;
    case "SDL_PIXELFORMAT_XBGR4444"_hash:      return SDL_PIXELFORMAT_XBGR4444;
    case "SDL_PIXELFORMAT_XRGB1555"_hash:      return SDL_PIXELFORMAT_XRGB1555;
    case "SDL_PIXELFORMAT_XBGR1555"_hash:      return SDL_PIXELFORMAT_XBGR1555;
    case "SDL_PIXELFORMAT_ARGB4444"_hash:      return SDL_PIXELFORMAT_ARGB4444;
    case "SDL_PIXELFORMAT_RGBA4444"_hash:      return SDL_PIXELFORMAT_RGBA4444;
    case "SDL_PIXELFORMAT_ABGR4444"_hash:      return SDL_PIXELFORMAT_ABGR4444;
    case "SDL_PIXELFORMAT_BGRA4444"_hash:      return SDL_PIXELFORMAT_BGRA4444;
    case "SDL_PIXELFORMAT_ARGB1555"_hash:      return SDL_PIXELFORMAT_ARGB1555;
    case "SDL_PIXELFORMAT_RGBA5551"_hash:      return SDL_PIXELFORMAT_RGBA5551;
    case "SDL_PIXELFORMAT_ABGR1555"_hash:      return SDL_PIXELFORMAT_ABGR1555;
    case "SDL_PIXELFORMAT_BGRA5551"_hash:      return SDL_PIXELFORMAT_BGRA5551;
    case "SDL_PIXELFORMAT_RGB565"_hash:        return SDL_PIXELFORMAT_RGB565;
    case "SDL_PIXELFORMAT_BGR565"_hash:        return SDL_PIXELFORMAT_BGR565;
    case "SDL_PIXELFORMAT_RGB24"_hash:         return SDL_PIXELFORMAT_RGB24;
    case "SDL_PIXELFORMAT_BGR24"_hash:         return SDL_PIXELFORMAT_BGR24;
    case "SDL_PIXELFORMAT_XRGB8888"_hash:      return SDL_PIXELFORMAT_XRGB8888;
    case "SDL_PIXELFORMAT_RGBX8888"_hash:      return SDL_PIXELFORMAT_RGBX8888;
    case "SDL_PIXELFORMAT_XBGR8888"_hash:      return SDL_PIXELFORMAT_XBGR8888;
    case "SDL_PIXELFORMAT_BGRX8888"_hash:      return SDL_PIXELFORMAT_BGRX8888;
    case "SDL_PIXELFORMAT_ARGB8888"_hash:      return SDL_PIXELFORMAT_ARGB8888;
    case "SDL_PIXELFORMAT_RGBA8888"_hash:      return SDL_PIXELFORMAT_RGBA8888;
    case "SDL_PIXELFORMAT_ABGR8888"_hash:      return SDL_PIXELFORMAT_ABGR8888;
    case "SDL_PIXELFORMAT_BGRA8888"_hash:      return SDL_PIXELFORMAT_BGRA8888;
    case "SDL_PIXELFORMAT_XRGB2101010"_hash:   return SDL_PIXELFORMAT_XRGB2101010;
    case "SDL_PIXELFORMAT_XBGR2101010"_hash:   return SDL_PIXELFORMAT_XBGR2101010;
    case "SDL_PIXELFORMAT_ARGB2101010"_hash:   return SDL_PIXELFORMAT_ARGB2101010;
    case "SDL_PIXELFORMAT_ABGR2101010"_hash:   return SDL_PIXELFORMAT_ABGR2101010;
    case "SDL_PIXELFORMAT_RGB48"_hash:         return SDL_PIXELFORMAT_RGB48;
    case "SDL_PIXELFORMAT_BGR48"_hash:         return SDL_PIXELFORMAT_BGR48;
    case "SDL_PIXELFORMAT_RGBA64"_hash:        return SDL_PIXELFORMAT_RGBA64;
    case "SDL_PIXELFORMAT_ARGB64"_hash:        return SDL_PIXELFORMAT_ARGB64;
    case "SDL_PIXELFORMAT_BGRA64"_hash:        return SDL_PIXELFORMAT_BGRA64;
    case "SDL_PIXELFORMAT_ABGR64"_hash:        return SDL_PIXELFORMAT_ABGR64;
    case "SDL_PIXELFORMAT_RGB48_FLOAT"_hash:   return SDL_PIXELFORMAT_RGB48_FLOAT;
    case "SDL_PIXELFORMAT_BGR48_FLOAT"_hash:   return SDL_PIXELFORMAT_BGR48_FLOAT;
    case "SDL_PIXELFORMAT_RGBA64_FLOAT"_hash:  return SDL_PIXELFORMAT_RGBA64_FLOAT;
    case "SDL_PIXELFORMAT_ARGB64_FLOAT"_hash:  return SDL_PIXELFORMAT_ARGB64_FLOAT;
    case "SDL_PIXELFORMAT_BGRA64_FLOAT"_hash:  return SDL_PIXELFORMAT_BGRA64_FLOAT;
    case "SDL_PIXELFORMAT_ABGR64_FLOAT"_hash:  return SDL_PIXELFORMAT_ABGR64_FLOAT;
    case "SDL_PIXELFORMAT_RGB96_FLOAT"_hash:   return SDL_PIXELFORMAT_RGB96_FLOAT;
    case "SDL_PIXELFORMAT_BGR96_FLOAT"_hash:   return SDL_PIXELFORMAT_BGR96_FLOAT;
    case "SDL_PIXELFORMAT_RGBA128_FLOAT"_hash: return SDL_PIXELFORMAT_RGBA128_FLOAT;
    case "SDL_PIXELFORMAT_ARGB128_FLOAT"_hash: return SDL_PIXELFORMAT_ARGB128_FLOAT;
    case "SDL_PIXELFORMAT_BGRA128_FLOAT"_hash: return SDL_PIXELFORMAT_BGRA128_FLOAT;
    case "SDL_PIXELFORMAT_ABGR128_FLOAT"_hash: return SDL_PIXELFORMAT_ABGR128_FLOAT;
    case "SDL_PIXELFORMAT_YV12"_hash:          return SDL_PIXELFORMAT_YV12;
    case "SDL_PIXELFORMAT_IYUV"_hash:          return SDL_PIXELFORMAT_IYUV;
    case "SDL_PIXELFORMAT_YUY2"_hash:          return SDL_PIXELFORMAT_YUY2;
    case "SDL_PIXELFORMAT_UYVY"_hash:          return SDL_PIXELFORMAT_UYVY;
    case "SDL_PIXELFORMAT_YVYU"_hash:          return SDL_PIXELFORMAT_YVYU;
    case "SDL_PIXELFORMAT_NV12"_hash:          return SDL_PIXELFORMAT_NV12;
    case "SDL_PIXELFORMAT_NV21"_hash:          return SDL_PIXELFORMAT_NV21;
    case "SDL_PIXELFORMAT_P010"_hash:          return SDL_PIXELFORMAT_P010;
    case "SDL_PIXELFORMAT_EXTERNAL_OES"_hash:  return SDL_PIXELFORMAT_EXTERNAL_OES;
    case "SDL_PIXELFORMAT_MJPG"_hash:          return SDL_PIXELFORMAT_MJPG;
    default:                                   return SDL_PIXELFORMAT_UNKNOWN;
    }
}

SDL_Colorspace colorspaceFromName(const char* name) {
    switch(hash(name)) {
    case "SDL_COLORSPACE_SRGB"_hash:           return SDL_COLORSPACE_SRGB;
    case "SDL_COLORSPACE_SRGB_LINEAR"_hash:    return SDL_COLORSPACE_SRGB_LINEAR;
    case "SDL_COLORSPACE_HDR10"_hash:          return SDL_COLORSPACE_HDR10;
    case "SDL_COLORSPACE_JPEG"_hash:           return SDL_COLORSPACE_JPEG;
    case "SDL_COLORSPACE_BT601_LIMITED"_hash:  return SDL_COLORSPACE_BT601_LIMITED;
    case "SDL_COLORSPACE_BT601_FULL"_hash:     return SDL_COLORSPACE_BT601_FULL;
    case "SDL_COLORSPACE_BT709_LIMITED"_hash:  return SDL_COLORSPACE_BT709_LIMITED;
    case "SDL_COLORSPACE_BT709_FULL"_hash:     return SDL_COLORSPACE_BT709_FULL;
    case "SDL_COLORSPACE_BT2020_LIMITED"_hash: return SDL_COLORSPACE_BT2020_LIMITED;
    case "SDL_COLORSPACE_BT2020_FULL"_hash:    return SDL_COLORSPACE_BT2020_FULL;
    default:                                   return SDL_COLORSPACE_UNKNOWN;
    }
}

std::string Settings::getSettingsPath() {
    std::string configPath;

#ifdef _WIN32
    // https://stackoverflow.com/a/62314965
    PWSTR tmpPath;
    HRESULT ret = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &tmpPath);

    if(ret != S_OK) {
        CoTaskMemFree(tmpPath);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get AppData directory.");

        exit(1);
    }

    // https://stackoverflow.com/a/3999597
    size_t tmpPathSize = wcslen(tmpPath);

    int pathSize = WideCharToMultiByte(CP_UTF8, 0, tmpPath, (int)tmpPathSize, NULL, 0, NULL, NULL);
    configPath   = std::string(pathSize, 0);

    WideCharToMultiByte(CP_UTF8, 0, tmpPath, (int)tmpPathSize, configPath.data(), pathSize, NULL, NULL);
    CoTaskMemFree(tmpPath);
#else
    if(std::getenv("XDG_CONFIG_HOME") != nullptr) {
        configPath = std::getenv("XDG_CONFIG_HOME");
    }
    else if(std::getenv("HOME") != nullptr) {
        configPath = std::string(std::getenv("HOME")) + "/.config";
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get home config directory.");

        exit(1);
    }
#endif

    configPath += "/CaptureCardRelay";
    if(!std::filesystem::exists(configPath)) {
        if(!std::filesystem::create_directory(configPath)) {
            SDL_Log("Failed to create config directory: %s", configPath.c_str());

            exit(1);
        }
    }

    configPath += "/settings";
    return configPath;
}

Settings::Settings(const char* manualFile) {
    if(manualFile != nullptr) {
        loadLocked(manualFile);
    }

    load();
}

Settings::~Settings() { save(); }

bool Settings::canSetSelectedCamera() const {
    static bool canSet = !valueLocked("camera");
    return canSet;
}

SDL_CameraID Settings::getSelectedCamera() {
    int cameraCount       = 0;
    SDL_CameraID* cameras = SDL_GetCameras(&cameraCount);

    if(cameras == nullptr || cameraCount == 0) {
        return 0;
    }

    std::optional<std::string> selectedName = getValue("camera");
    if(!selectedName.has_value()) {
        SDL_CameraID camera = cameras[0];
        SDL_free(cameras);

        return camera;
    }

    SDL_CameraID camera = 0;
    for(int i = 0; i < cameraCount; i++) {
        camera = cameras[i];

        const char* name = SDL_GetCameraName(camera);
        if(name != nullptr && strcmp(name, selectedName.value().c_str()) == 0) {
            break;
        }
    }

    SDL_free(cameras);
    return camera;
}

void Settings::setSelectedCamera(SDL_CameraID camera) {
    if(camera == 0) {
        if(clearValue("camera")) {
            selectedCameraChanged(camera);
        }

        return;
    }

    const char* name = SDL_GetCameraName(camera);
    if(name == nullptr) {
        if(clearValue("camera")) {
            selectedCameraChanged(camera);
        }

        return;
    }

    if(setValue("camera", name)) {
        selectedCameraChanged(camera);
    }
}

bool Settings::canSetRecordingDevice() const {
    static bool canSet = !valueLocked("recordingDevice");
    return canSet;
}

SDL_AudioDeviceID Settings::getSelectedRecordingDevice() {
    int recordingDeviceCount   = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioRecordingDevices(&recordingDeviceCount);

    if(devices == nullptr || recordingDeviceCount == 0) {
        return 0;
    }

    std::optional<std::string> selectedName = getValue("recordingDevice");
    if(!selectedName.has_value()) {
        SDL_free(devices);
        return 0;
    }

    SDL_AudioDeviceID device;
    for(int i = 0; i < recordingDeviceCount; i++) {
        device = devices[i];

        const char* name = SDL_GetAudioDeviceName(device);
        if(name != nullptr && strcmp(name, selectedName.value().c_str()) == 0) {
            SDL_free(devices);
            return device;
        }
    }

    device = devices[0];
    SDL_free(devices);

    return device;
}

void Settings::setSelectedRecordingDevice(SDL_AudioDeviceID device) {
    if(device == 0) {
        if(clearValue("recordingDevice")) {
            selectedRecordingDeviceChanged(device);
        }

        return;
    }

    const char* name = SDL_GetAudioDeviceName(device);
    if(name == nullptr) {
        if(clearValue("recordingDevice")) {
            selectedRecordingDeviceChanged(device);
        }

        return;
    }

    if(setValue("recordingDevice", name)) {
        selectedRecordingDeviceChanged(device);
    }
}

bool Settings::canSetDisplayMode() const {
    static bool canSet = !valueLocked("displayMode");
    return canSet;
}

CameraDisplayMode Settings::getDisplayMode() {
    int mode = std::atoi(getValue("displayMode").value_or(std::to_string(DEFAULT_DISPLAY_MODE)).c_str());

    switch(mode) {
    case DISPLAY_MODE_CONTAIN:
    case DISPLAY_MODE_COVER:
    case DISPLAY_MODE_FILL:
    case DISPLAY_MODE_NONE:
        return static_cast<CameraDisplayMode>(mode);
    default:
        setDisplayMode(DEFAULT_DISPLAY_MODE);
        return DEFAULT_DISPLAY_MODE;
    }
}

void Settings::setDisplayMode(CameraDisplayMode mode) {
    if(setValue("displayMode", std::to_string(mode))) {
        displayModeChanged(mode);
    }
}

bool Settings::canSetPixelFormat() const {
    static bool canSet = !valueLocked("pixelFormat");
    return canSet;
}

PixelFormat Settings::getPixelFormat() {
    int mode = std::atoi(getValue("pixelFormat").value_or(std::to_string(DEFAULT_PIXEL_FORMAT)).c_str());

    switch(mode) {
    case PIXEL_FORMAT_CAMERA:
    case PIXEL_FORMAT_RGB24:
        return static_cast<PixelFormat>(mode);
    default:
        setPixelFormat(DEFAULT_PIXEL_FORMAT);
        return DEFAULT_PIXEL_FORMAT;
    }
}

void Settings::setPixelFormat(PixelFormat format) {
    if(setValue("pixelFormat", std::to_string(format))) {
        pixelFormatChanged(format);
    }
}

bool Settings::canSetFrameLimitType() const {
    static bool canSet = !valueLocked("frameLimitType");
    return canSet;
}

bool Settings::canSetFrameLimitFPS() const {
    static bool canSet = !valueLocked("frameLimitFPS");
    return canSet;
}

FrameLimitInfo Settings::getFrameLimitInfo() {
    FrameLimitInfo info;

    switch(std::atoi(getValue("frameLimitType").value_or(std::to_string(DEFAULT_FRAME_LIMIT_TYPE)).c_str())) {
    case FRAME_LIMIT_CAMERA:         info.type = FRAME_LIMIT_CAMERA; break;
    case FRAME_LIMIT_VSYNC:          info.type = FRAME_LIMIT_VSYNC; break;
    case FRAME_LIMIT_VSYNC_ADAPTIVE: info.type = FRAME_LIMIT_VSYNC_ADAPTIVE; break;
    case FRAME_LIMIT_FPS:            info.type = FRAME_LIMIT_FPS; break;
    case FRAME_LIMIT_NONE:           info.type = FRAME_LIMIT_NONE; break;
    default:                         info.type = DEFAULT_FRAME_LIMIT_TYPE; break;
    }

    float fps = std::atof(getValue("frameLimitFPS").value_or(std::to_string(DEFAULT_FRAME_LIMIT_FPS)).c_str());
    info.fps  = std::clamp(fps, MIN_FPS, MAX_FPS);

    return info;
}

void Settings::setFrameLimitInfo(FrameLimitInfo info) {
    // use bitwise or to make both get evaluated
    if(
        (int)setValue("frameLimitType", std::to_string(info.type)) |
        setValue("frameLimitFPS", std::to_string(info.fps))
    ) {
        frameLimitInfoChanged(getFrameLimitInfo());
    }
}

bool Settings::canSetFullscreen() const {
    static bool canSet = !valueLocked("fullscreen");
    return canSet;
}

bool Settings::isFullscreen() { return getValue("fullscreen").value_or("false") == "true"; }
void Settings::setFullscreen(bool fullscreen) {
    if(setValue("fullscreen", fullscreen ? "true" : "false")) {
        fullscreenChanged(fullscreen);
    }
}

int clampVolume(int volume) { return std::max(MIN_VOLUME, std::min(MAX_VOLUME, volume)); }

bool Settings::canSetVolume() const {
    static bool canSet = !valueLocked("volume");
    return canSet;
}

int Settings::getVolume() { return clampVolume(std::atoi(getValue("volume").value_or("100").c_str())); }
void Settings::setVolume(int volume) {
    volume = clampVolume(volume);

    if(setValue("volume", std::to_string(volume))) {
        volumeChanged(volume);
    }
}

bool Settings::canSetPreferredColorspace() const {
    static bool canSet = !valueLocked("preferredColorspace");
    return canSet;
}

SDL_Colorspace Settings::getPreferredColorspace() { return colorspaceFromName(getValue("preferredColorspace").value_or(colorspaceName(SDL_COLORSPACE_UNKNOWN)).c_str()); }
void Settings::setPreferredColorspace(SDL_Colorspace colorspace) {
    if(setValue("preferredColorspace", colorspaceName(colorspace))) {
        preferredColorspaceChanged(colorspace);
    }
}

bool Settings::canSetPreferredPixelFormat() const {
    static bool canSet = !valueLocked("preferredPixelFormat");
    return canSet;
}

SDL_PixelFormat Settings::getPreferredPixelFormat() { return pixelFormatFromName(getValue("preferredPixelFormat").value_or(pixelFormatName(SDL_PIXELFORMAT_UNKNOWN)).c_str()); }
void Settings::setPreferredPixelFormat(SDL_PixelFormat format) {
    if(setValue("preferredPixelFormat", pixelFormatName(format))) {
        preferredPixelFormatChanged(format);
    }
}

std::optional<std::string> Settings::getValue(std::string key) {
    auto lockedIt  = m_lockedCache.find(key);
    auto regularIt = m_cache.find(key);

    if(lockedIt != m_lockedCache.end()) {
        return lockedIt->second;
    }
    else if(regularIt != m_cache.end()) {
        return regularIt->second;
    }

    return std::nullopt;
}

bool Settings::setValue(std::string key, std::string value) {
    if(valueLocked(key)) {
        return false;
    }

    auto it = m_cache.find(key);
    if(it == m_cache.end() || it->second != value) {
        m_cache[key] = value;
        save();
    }

    return true;
}

bool Settings::valueLocked(std::string key) const {
    return m_lockedCache.contains(key);
}

bool Settings::clearValue(std::string key) {
    if(valueLocked(key)) {
        return false;
    }

    auto it = m_cache.find(key);
    if(it != m_cache.end()) {
        m_cache.erase(it);
        save();
    }

    return true;
}

void _load(std::unordered_map<std::string, std::string>& cache, std::string path) {
    std::ifstream file(path);
    if(!file.is_open()) {
        return;
    }

    for(std::string line; std::getline(file, line);) {
        size_t pos = line.find_first_of(":");
        if(pos == std::string::npos) {
            continue;
        }

        std::string key   = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        cache[key]        = value;
    }
}

void Settings::loadLocked(std::string path) { _load(m_lockedCache, path); }
void Settings::load() { _load(m_cache, getSettingsPath()); }

void Settings::save() {
    std::ofstream file(getSettingsPath(), std::ios_base::out | std::ios_base::trunc);

    for(const auto& pair : m_cache) {
        file << pair.first + ":" + pair.second + "\n";
    }

    file.close();
}