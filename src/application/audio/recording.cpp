#include <application.hpp>
#include <settings.hpp>

void Application::onRecordingCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) { ((Application*)userdata)->recordingCallbackHandler(stream, additional_amount, total_amount); }
void Application::recordingCallbackHandler(SDL_AudioStream* stream, int additional_amount, int total_amount) {
    while(SDL_GetAudioStreamAvailable(stream) >= (int)(audioBufferSize * sizeof(Uint16))) {
        std::array<Uint16, audioBufferSize> buffer;
        SDL_GetAudioStreamData(stream, buffer.data(), buffer.size() * sizeof(Uint16));

        m_audioMutex.lock();
        m_audioBuffers.push(buffer);
        m_audioMutex.unlock();
    }

    SDL_Log("Audio queue: %zu", m_audioBuffers.size());
}

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

    m_audioRecording.device = SDL_OpenAudioDevice(deviceID, &m_audioSpec);
    if(m_audioRecording.device == 0) {
        SDL_Log("Couldn't open recording device: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    SDL_Log("Opened recording device: %s", SDL_GetAudioDeviceName(m_audioRecording.device));
    SDL_GetAudioDeviceFormat(m_audioRecording.device, &m_audioRecording.spec, &m_audioRecording.bufferSize);

    m_audioRecording.stream = SDL_CreateAudioStream(&m_audioRecording.spec, &m_audioSpec);
    SDL_BindAudioStream(m_audioRecording.device, m_audioRecording.stream);

    m_audioRecording.buffer = (Uint8*)malloc(m_audioRecording.bufferSize);

    SDL_SetAudioStreamPutCallback(m_audioRecording.stream, &Application::onRecordingCallback, this);

    while(!m_audioBuffers.empty()) {
        m_audioBuffers.pop();
    }
}

void Application::closeAudioRecordingDevice() {
    if(m_audioRecording.device != 0) {
        SDL_CloseAudioDevice(m_audioRecording.device);
    }

    if(m_audioRecording.stream != nullptr) {
        SDL_DestroyAudioStream(m_audioRecording.stream);
    }

    if(m_audioRecording.buffer != nullptr) {
        free(m_audioRecording.buffer);
    }

    m_audioRecording.bufferSize = 0;
}