#include <application.hpp>

// https://stackoverflow.com/a/57796299
template<typename T, size_t N>
constexpr auto make_array(T value) -> std::array<T, N> {
    std::array<T, N> a;

    for(auto& x : a) {
        x = value;
    }

    return a;
}

void Application::onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount) { ((Application*)userdata)->playbackCallbackHandler(stream, additionalAmount, totalAmount); }
void Application::playbackCallbackHandler(SDL_AudioStream* stream, int additionalAmount, int) {
    auto lock = std::unique_lock(m_audioMutex);

    size_t totalBuffers = static_cast<size_t>(additionalAmount) / (m_audioBufferSize * sizeof(m_emptyAudioBuffer[0]));
    for(size_t i = 0; i <= totalBuffers; i++) {
        if(m_audioBuffers.empty()) {
            SDL_PutAudioStreamData(stream, m_emptyAudioBuffer.data(), m_emptyAudioBuffer.size() * sizeof(m_emptyAudioBuffer[0]));
            continue;
        }

        std::vector<Uint16> buffer = m_audioBuffers.front();
        m_audioBuffers.pop_front();

        SDL_PutAudioStreamData(stream, buffer.data(), buffer.size() * sizeof(buffer[0]));
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
    SDL_GetAudioDeviceFormat(m_audioPlayback.device, &m_audioPlayback.spec, &m_audioPlayback.bufferSize);

    m_audioBufferSize  = static_cast<size_t>(m_audioPlayback.bufferSize / 8);
    m_emptyAudioBuffer = std::vector<Uint16>(m_audioBufferSize, 0);

    auto lock = std::unique_lock(m_streamMutex);

    m_audioPlayback.stream = SDL_CreateAudioStream(&m_audioSpec, &m_audioPlayback.spec);
    SDL_BindAudioStream(m_audioPlayback.device, m_audioPlayback.stream);

    updateVolume(false);

    m_audioPlayback.buffer = reinterpret_cast<Uint8*>(malloc(static_cast<size_t>(m_audioPlayback.bufferSize)));
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