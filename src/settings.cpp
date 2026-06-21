#include <filesystem>
#include <fstream>
#include <settings.hpp>
#include <string>

const CameraDisplayMode DEFAULT_MODE = CameraDisplayMode::CONTAIN;

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

Settings::Settings() { load(); }
Settings::~Settings() { save(); }

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

    SDL_CameraID camera;
    for(int i = 0; i < cameraCount; i++) {
        camera = cameras[i];

        const char* name = SDL_GetCameraName(camera);
        if(name != nullptr && strcmp(name, selectedName.value().c_str()) == 0) {
            SDL_free(cameras);
            return camera;
        }
    }

    camera = cameras[0];
    SDL_free(cameras);

    return camera;
}

void Settings::setSelectedCamera(SDL_CameraID camera) {
    if(camera == 0) {
        clearValue("camera");
        selectedCameraChanged(camera);

        return;
    }

    const char* name = SDL_GetCameraName(camera);
    if(name == nullptr) {
        clearValue("camera");
        selectedCameraChanged(camera);

        return;
    }

    setValue("camera", name);
    selectedCameraChanged(camera);
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
        clearValue("recordingDevice");
        selectedRecordingDeviceChanged(device);

        return;
    }

    const char* name = SDL_GetAudioDeviceName(device);
    if(name == nullptr) {
        clearValue("recordingDevice");
        selectedRecordingDeviceChanged(device);

        return;
    }

    setValue("recordingDevice", name);
    selectedRecordingDeviceChanged(device);
}

int clampVolume(int volume) { return std::max(MIN_VOLUME, std::min(MAX_VOLUME, volume)); }
int Settings::getVolume() { return clampVolume(std::atoi(getValue("volume").value_or("100").c_str())); }
void Settings::setVolume(int volume) {
    volume = clampVolume(volume);

    setValue("volume", std::to_string(volume));
    volumeChanged(volume);
}

bool Settings::isFullscreen() { return getValue("fullscreen").value_or("false") == "true"; }
void Settings::setFullscreen(bool fullscreen) {
    setValue("fullscreen", fullscreen ? "true" : "false");
    fullscreenChanged(fullscreen);
}

CameraDisplayMode Settings::getDisplayMode() {
    int mode = std::atoi(getValue("displayMode").value_or(std::to_string(DEFAULT_MODE)).c_str());

    switch(mode) {
    case CameraDisplayMode::CONTAIN:
    case CameraDisplayMode::COVER:
    case CameraDisplayMode::FILL:
    case CameraDisplayMode::NONE:
        return static_cast<CameraDisplayMode>(mode);
    default:
        setDisplayMode(DEFAULT_MODE);
        return DEFAULT_MODE;
    }
}

void Settings::setDisplayMode(CameraDisplayMode mode) {
    setValue("displayMode", std::to_string(mode));
    displayModeChanged(mode);
}

std::optional<std::string> Settings::getValue(std::string key) {
    if(m_cache.find(key) == m_cache.end()) {
        return std::nullopt;
    }

    return m_cache[key];
}

void Settings::setValue(std::string key, std::string value) {
    m_cache[key] = value;

    save();
}

void Settings::clearValue(std::string key) {
    auto it = m_cache.find(key);
    if(it != m_cache.end()) {
        m_cache.erase(it);
    }

    save();
}

void Settings::load() {
    std::ifstream file(getSettingsPath());

    for(std::string line; std::getline(file, line);) {
        size_t pos = line.find(":");
        if(pos == std::string::npos) {
            continue;
        }

        std::string key   = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        m_cache[key] = value;
    }

    file.close();
}

void Settings::save() {
    std::ofstream file(getSettingsPath(), std::ios_base::out | std::ios_base::trunc);

    for(auto pair : m_cache) {
        file << pair.first + ":" + pair.second + "\n";
    }

    file.close();
}

static Settings* instance = nullptr;
Settings* Settings::get() {
    if(instance == nullptr) {
        instance = new Settings();
    }

    return instance;
}

void Settings::close() {
    if(instance == nullptr) {
        return;
    }

    delete instance;
}