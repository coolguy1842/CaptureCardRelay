#include <application.hpp>

#include "SDL3/SDL_audio.h"
#include "settings.hpp"

void Application::initAudioRecordingDevices() {
    int recordingDeviceCount            = 0;
    SDL_AudioDeviceID* recordingDevices = SDL_GetAudioRecordingDevices(&recordingDeviceCount);

    if(recordingDevices == nullptr || recordingDeviceCount <= 0) {
        SDL_Log("Couldn't enumerate recording devices or none plugged in: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    for(int i = 0; i < recordingDeviceCount; i++) {
        m_recordingDevices.push_back(recordingDevices[i]);
    }

    SDL_free(recordingDevices);
}

void Application::openAudioRecordingDevice() {
    if(m_audioRecording.device != 0) {
        closeAudioRecordingDevice();
    }

    SDL_AudioDeviceID deviceID = Settings::get()->getSelectedRecordingDevice();
    if(deviceID == 0) {
        deviceID = SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
    }

    m_audioRecording.device = SDL_OpenAudioDevice(deviceID, nullptr);
    if(m_audioRecording.device == 0) {
        SDL_Log("Couldn't open recording device: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    SDL_Log("Opened recording device: %s", SDL_GetAudioDeviceName(m_audioRecording.device));
    SDL_GetAudioDeviceFormat(m_audioRecording.device, &m_audioRecording.spec, nullptr);

    m_audioRecording.stream = SDL_CreateAudioStream(&m_audioRecording.spec, &m_audioPlayback.spec);
    SDL_BindAudioStream(m_audioRecording.device, m_audioRecording.stream);
}

void Application::closeAudioRecordingDevice() {
    if(m_audioRecording.device != 0) {
        SDL_CloseAudioDevice(m_audioRecording.device);
    }

    if(m_audioRecording.stream != nullptr) {
        SDL_DestroyAudioStream(m_audioRecording.stream);
    }
}