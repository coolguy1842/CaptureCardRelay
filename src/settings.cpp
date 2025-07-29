#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <settings.hpp>
#include <string>

// TODO: add windows paths
std::string Settings::getSettingsPath() {
    std::string configPath;

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

Settings::Settings() {
    load();
}

Settings::~Settings() {
    save();
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
        return;
    }

    const char* name = SDL_GetCameraName(camera);
    if(name == nullptr) {
        clearValue("camera");
        return;
    }

    setValue("camera", name);
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
        return;
    }

    const char* name = SDL_GetAudioDeviceName(device);
    if(name == nullptr) {
        clearValue("recordingDevice");
        return;
    }

    setValue("recordingDevice", name);
}

int clampVolume(int volume) {
    return std::max(0, std::min(150, volume));
}

int Settings::getVolume() {
    std::optional<std::string> volume = getValue("volume");
    if(!volume.has_value()) {
        volume = "100";
    }

    return clampVolume(std::atoi(volume->c_str()));
}

void Settings::setVolume(int volume) {
    setValue("volume", std::to_string(clampVolume(volume)));
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