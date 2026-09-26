#include <application.hpp>

void Application::onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount) { ((Application*)userdata)->playbackCallbackHandler(stream, additionalAmount, totalAmount); }
void Application::playbackCallbackHandler(SDL_AudioStream* stream, int needed, int) {
    if(m_audioBuffers.empty()) {
        return;
    }

#ifndef __EMSCRIPTEN__
    auto lock = std::unique_lock(m_audioMutex);
#endif

    while(needed > 0) {
        const int bytes = std::min(needed, m_audioBufferSizeBytes);
        if(m_currentBuffers == 0) {
            SDL_PutAudioStreamDataNoCopy(stream, m_emptyBuffer.get(), bytes, NULL, NULL);
        }
        else {
            std::unique_ptr<AudioType[]> buffer = std::move(m_audioBuffers.front());
            m_audioBuffers.pop_front();
            m_currentBuffers--;

            SDL_PutAudioStreamDataNoCopy(stream, buffer.get(), bytes, NULL, NULL);
            m_audioBuffers.push_back(std::move(buffer));
        }

        needed -= bytes;
    }
}

void Application::initAudioPlaybackDevices() {
    m_playbackDevices.clear();

    int playbackDeviceCount            = 0;
    SDL_AudioDeviceID* playbackDevices = SDL_GetAudioPlaybackDevices(&playbackDeviceCount);

    if(playbackDevices == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't enumerate playback devices: %s", SDL_GetError());

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

    initAudioPlaybackDevices();

    m_audioPlayback.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_audioSpec);
    if(m_audioPlayback.device == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't open playback device: %s", SDL_GetError());
        return;
    }

    SDL_Log("Opened playback device: %s", SDL_GetAudioDeviceName(m_audioPlayback.device));

    int playbackBufferSize = 0;
    SDL_GetAudioDeviceFormat(m_audioPlayback.device, &m_audioPlayback.spec, &playbackBufferSize);

    auto lock  = std::unique_lock(m_streamMutex);
    auto lock2 = std::unique_lock(m_audioMutex);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
    m_audioBufferSizeBytes = static_cast<int>(SDL_AUDIO_FRAMESIZE(m_audioPlayback.spec)) * playbackBufferSize;
#pragma GCC diagnostic pop

    m_audioBuffers.clear();
    m_currentBuffers = 0;

    for(size_t i = 0; i < maxAudioBuffers; i++) {
        m_audioBuffers.push_back(std::unique_ptr<AudioType[]>(new AudioType[static_cast<size_t>(m_audioBufferSizeBytes)]));
    }

    m_emptyBuffer = std::unique_ptr<AudioType[]>(new AudioType[static_cast<size_t>(m_audioBufferSizeBytes)]);
    memset(m_emptyBuffer.get(), 0, static_cast<size_t>(m_audioBufferSizeBytes));

    m_audioPlayback.stream = SDL_CreateAudioStream(&m_audioPlayback.spec, &m_audioPlayback.spec);
    SDL_BindAudioStream(m_audioPlayback.device, m_audioPlayback.stream);

    updateVolume(false);
    SDL_SetAudioStreamGetCallback(m_audioPlayback.stream, &Application::onPlaybackCallback, this);
}

void Application::closeAudioPlaybackDevice() {
    auto lock = std::unique_lock(m_streamMutex);

    if(m_audioPlayback.stream != nullptr) {
        SDL_DestroyAudioStream(m_audioPlayback.stream);
        m_audioPlayback.stream = nullptr;
    }

    if(m_audioPlayback.device != 0) {
        SDL_CloseAudioDevice(m_audioPlayback.device);
        m_audioPlayback.device = 0;
    }

    auto lock2 = std::unique_lock(m_audioMutex);
    m_audioBuffers.clear();
}