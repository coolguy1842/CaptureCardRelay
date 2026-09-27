#include <SDL3/SDL_audio.h>
#include <application.hpp>
#include <memory>

void Application::onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount) { ((Application*)userdata)->playbackCallbackHandler(stream, additionalAmount, totalAmount); }
void Application::playbackCallbackHandler(SDL_AudioStream* stream, int needed, int) {
    while(needed >= m_audioBufferSizeBytes) {
        needed -= m_audioBufferSizeBytes;

        if(m_audioBuffers.empty()) {
            SDL_PutAudioStreamData(stream, m_emptyAudioBuffer.get(), m_audioBufferSizeBytes);
            continue;
        }

        AudioFrame buffer = m_audioBuffers.front();
        m_audioBuffers.pop();

        SDL_PutAudioStreamData(stream, buffer.get(), m_audioBufferSizeBytes);

        m_freeAudioBuffers.push(buffer);
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
    const int frameSize    = static_cast<int>(SDL_AUDIO_FRAMESIZE(m_audioSpec));
    m_audioBufferSizeBytes = frameSize * (playbackBufferSize / 8);
#pragma GCC diagnostic pop

    while(!m_audioBuffers.empty()) {
        m_audioBuffers.pop();
    }

    while(!m_freeAudioBuffers.empty()) {
        m_freeAudioBuffers.pop();
    }

    m_emptyAudioBuffer = std::shared_ptr<AudioType[]>(new AudioType[static_cast<size_t>(m_audioBufferSizeBytes)]{});
    for(size_t i = 0; i < maxAudioBuffers; i++) {
        m_freeAudioBuffers.push(std::shared_ptr<AudioType[]>(new AudioType[static_cast<size_t>(m_audioBufferSizeBytes)]));
    }

    m_audioPlayback.stream = SDL_CreateAudioStream(&m_audioSpec, &m_audioPlayback.spec);
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

    m_emptyAudioBuffer = {};
    while(!m_audioBuffers.empty()) {
        m_audioBuffers.pop();
    }

    while(!m_freeAudioBuffers.empty()) {
        m_freeAudioBuffers.pop();
    }
}