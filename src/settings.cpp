#include <SDL3/SDL_pixels.h>
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

std::string Settings::getSettingsPath() {
    std::string configPath;

#ifdef _WIN32
    if(std::getenv("XDG_CONFIG_HOME") == nullptr) {
        configPath = std::getenv("AppData");
    }
    else {
        SDL_Log("Couldn't get appdata directory.");
        exit(1);
    }
#else
    if(std::getenv("XDG_CONFIG_HOME") != nullptr) {
        configPath = std::getenv("XDG_CONFIG_HOME");
    }
    else if(std::getenv("HOME") != nullptr) {
        configPath = std::string(std::getenv("HOME")) + "/.config";
    }
    else {
        SDL_Log("Couldn't get home config directory.");

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