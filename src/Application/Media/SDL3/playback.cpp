#include <application.hpp>

void Application::onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int needed, int totalAmount) { reinterpret_cast<Application*>(userdata)->playbackCallbackHandler(stream, needed, totalAmount); }
void Application::playbackCallbackHandler(SDL_AudioStream* stream, int needed, int /*totalAmount*/) { m_audioRingBuffer.pop(stream, needed); }

void Application::openAudioPlaybackDevice() {
#ifdef ENABLE_PIPEWIRE
    if(m_usingPipewire) {
        return;
    }
#endif

    if(m_audioPlayback.device != 0) {
        closeAudioPlaybackDevice();
    }

    m_audioPlayback.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &m_audioSpec);
    if(m_audioPlayback.device == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't open playback device: %s", SDL_GetError());
        return;
    }

    SDL_Log("Opened playback device: %s", SDL_GetAudioDeviceName(m_audioPlayback.device));

    int playbackBufferSize = 0;
    SDL_GetAudioDeviceFormat(m_audioPlayback.device, &m_audioPlayback.spec, &playbackBufferSize);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
    const int frameSize = static_cast<int>(SDL_AUDIO_FRAMESIZE(m_audioSpec));
    m_audioRingBuffer.setSize(static_cast<size_t>(frameSize) * (playbackBufferSize / 8), maxAudioBuffers);
#pragma GCC diagnostic pop

    m_audioPlayback.stream = SDL_CreateAudioStream(&m_audioSpec, &m_audioPlayback.spec);
    SDL_BindAudioStream(m_audioPlayback.device, m_audioPlayback.stream);

    _updateVolumeSDL();
    SDL_SetAudioStreamGetCallback(m_audioPlayback.stream, &Application::onPlaybackCallback, this);
}

void Application::closeAudioPlaybackDevice() {
    if(m_audioPlayback.stream != nullptr) {
        SDL_DestroyAudioStream(m_audioPlayback.stream);
        m_audioPlayback.stream = nullptr;
    }

    if(m_audioPlayback.device != 0) {
        SDL_CloseAudioDevice(m_audioPlayback.device);
        m_audioPlayback.device = 0;
    }

    m_audioRingBuffer.setSize(0, 0);
}

void Application::_updateVolumeSDL() {
    if(m_audioPlayback.stream == nullptr) {
        return;
    }

    SDL_SetAudioStreamGain(m_audioPlayback.stream, getCubicVolume());
}