#include <application.hpp>
#include <cstdlib>

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_stdinc.h"
#include "settings.hpp"

void Application::initAudioPlaybackDevices() {
    int playbackDeviceCount            = 0;
    SDL_AudioDeviceID* playbackDevices = SDL_GetAudioPlaybackDevices(&playbackDeviceCount);

    if(playbackDevices == nullptr || playbackDeviceCount <= 0) {
        SDL_Log("Couldn't enumerate playback devices or none plugged in: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    for(int i = 0; i < playbackDeviceCount; i++) {
        m_playbackDevices.push_back(playbackDevices[i]);
    }

    SDL_free(playbackDevices);
}

void Application::openAudioPlaybackDevice() {
    if(m_audioPlayback.device != 0) {
        closeAudioPlaybackDevice();
    }

    m_audioPlayback.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if(m_audioPlayback.device == 0) {
        SDL_Log("Couldn't open playback device: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    SDL_Log("Opened playback device: %s", SDL_GetAudioDeviceName(m_audioPlayback.device));
    SDL_GetAudioDeviceFormat(m_audioPlayback.device, &m_audioPlayback.spec, &m_audioPlayback.bufferSize);

    m_audioPlayback.stream = SDL_CreateAudioStream(&m_audioPlayback.spec, &m_audioPlayback.spec);
    SDL_BindAudioStream(m_audioPlayback.device, m_audioPlayback.stream);
    SDL_SetAudioStreamGain(m_audioPlayback.stream, Settings::get()->getVolume() / 100.0f);

    m_audioPlayback.buffer = (Uint8*)malloc(m_audioPlayback.bufferSize);
}

void Application::closeAudioPlaybackDevice() {
    if(m_audioPlayback.device != 0) {
        SDL_CloseAudioDevice(m_audioPlayback.device);
    }

    if(m_audioPlayback.stream != nullptr) {
        SDL_DestroyAudioStream(m_audioPlayback.stream);
    }

    if(m_audioPlayback.buffer != nullptr) {
        free(m_audioPlayback.buffer);
    }

    m_audioPlayback.bufferSize = 0;
}