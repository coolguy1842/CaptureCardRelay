#include <application.hpp>

// https://stackoverflow.com/a/57796299
template <typename T, size_t N>
constexpr auto make_array(T value) -> std::array<T, N> {
    std::array<T, N> a;

    for(auto& x : a) {
        x = value;
    }

    return a;
}

void Application::onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) { ((Application*)userdata)->playbackCallbackHandler(stream, additional_amount, total_amount); }
void Application::playbackCallbackHandler(SDL_AudioStream* stream, int additional_amount, int total_amount) {
    int totalBuffers = additional_amount / (audioBufferSize * sizeof(Uint16));
    if(m_audioBuffers.size() > maxAudioBuffers) {
        totalBuffers += (m_audioBuffers.size() - maxAudioBuffers) + 5;
    }

    const static auto emptyBuffer = make_array<Uint16, audioBufferSize>(0);

    auto lock = std::unique_lock(m_audioMutex);
    for(int i = 0; i <= totalBuffers; i++) {
        if(m_audioBuffers.empty()) {
            SDL_PutAudioStreamData(stream, emptyBuffer.data(), audioBufferSize * sizeof(Uint16));
            continue;
        }

        std::array<Uint16, audioBufferSize> buffer = m_audioBuffers.front();
        m_audioBuffers.pop_front();

        SDL_PutAudioStreamData(stream, buffer.data(), audioBufferSize * sizeof(Uint16));
    }
}

void Application::initAudioPlaybackDevices() {
    m_playbackDevices.clear();

    int playbackDeviceCount            = 0;
    SDL_AudioDeviceID* playbackDevices = SDL_GetAudioPlaybackDevices(&playbackDeviceCount);

    if(playbackDevices == nullptr) {
        SDL_Log("Couldn't enumerate playback devices: %s", SDL_GetError());

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
        SDL_Log("Couldn't open playback device: %s", SDL_GetError());
        return;
    }

    SDL_Log("Opened playback device: %s", SDL_GetAudioDeviceName(m_audioPlayback.device));
    SDL_GetAudioDeviceFormat(m_audioPlayback.device, &m_audioPlayback.spec, &m_audioPlayback.bufferSize);

    auto lock = std::unique_lock(m_streamMutex);

    m_audioPlayback.stream = SDL_CreateAudioStream(&m_audioSpec, &m_audioPlayback.spec);
    SDL_BindAudioStream(m_audioPlayback.device, m_audioPlayback.stream);
    updateVolume();

    m_audioPlayback.buffer = (Uint8*)malloc(m_audioPlayback.bufferSize);
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

    if(m_audioPlayback.buffer != nullptr) {
        free(m_audioPlayback.buffer);
        m_audioPlayback.buffer = nullptr;
    }

    m_audioPlayback.bufferSize = 0;
}