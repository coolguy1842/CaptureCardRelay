#include <application.hpp>
#include <format>
#include <settings.hpp>
#include <vector>

void Application::onRecordingCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) { ((Application*)userdata)->recordingCallbackHandler(stream, additional_amount, total_amount); }
void Application::recordingCallbackHandler(SDL_AudioStream* stream, int additional_amount, int total_amount) {
    auto lock = std::unique_lock(m_audioMutex);

    while(SDL_GetAudioStreamAvailable(stream) >= (int)(audioBufferSize * sizeof(Uint16))) {
        std::array<Uint16, audioBufferSize> buffer;
        SDL_GetAudioStreamData(stream, buffer.data(), buffer.size() * sizeof(Uint16));

        m_audioBuffers.push_back(buffer);

        while(m_audioBuffers.size() > maxAudioBuffers) {
            m_audioBuffers.pop_front();
        }
    }
}

void Application::initAudioRecordingDevices() {
    m_recordingDevices.clear();

    int recordingDeviceCount            = 0;
    SDL_AudioDeviceID* recordingDevices = SDL_GetAudioRecordingDevices(&recordingDeviceCount);

    if(recordingDevices == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't enumerate recording devices: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    for(int i = 0; i < recordingDeviceCount; i++) {
        SDL_AudioDeviceID id = recordingDevices[i];
        const char* name     = SDL_GetAudioDeviceName(id);
        if(name == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get name of recording device with id: %d: %s", id, SDL_GetError());
            name = "";
        }

        m_recordingDevices.push_back({ id, name });
    }

    SDL_free(recordingDevices);
}

void Application::openAudioRecordingDevice() {
    if(m_audioRecording.device != 0) {
        closeAudioRecordingDevice();
    }

    initAudioRecordingDevices();
    if(m_recordingDevices.empty()) {
        return;
    }

    SDL_AudioDeviceID deviceID = Settings::get()->getSelectedRecordingDevice();
    if(deviceID == 0) {
        deviceID = SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
    }

    m_audioRecording.device = SDL_OpenAudioDevice(deviceID, &m_audioSpec);
    if(m_audioRecording.device == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't open recording device: %s", SDL_GetError());
        return;
    }

    const char* name = SDL_GetAudioDeviceName(m_audioRecording.device);
    if(name == nullptr) {
        name = "(null)";
    }

    SDL_Log("Opened recording device: %s", name);
    SDL_GetAudioDeviceFormat(m_audioRecording.device, &m_audioRecording.spec, &m_audioRecording.bufferSize);

    auto lock                = std::unique_lock(m_streamMutex);
    m_currentRecordingDevice = { .id = deviceID, .name = name };

    m_audioRecording.stream = SDL_CreateAudioStream(&m_audioRecording.spec, &m_audioSpec);
    SDL_BindAudioStream(m_audioRecording.device, m_audioRecording.stream);

    m_audioRecording.buffer = (Uint8*)malloc(m_audioRecording.bufferSize);

    SDL_SetAudioStreamPutCallback(m_audioRecording.stream, &Application::onRecordingCallback, this);

    auto lock2 = std::unique_lock(m_audioMutex);
    m_audioBuffers.clear();
}

void Application::closeAudioRecordingDevice() {
    auto lock                = std::unique_lock(m_streamMutex);
    m_currentRecordingDevice = { .id = 0, .name = "(null)" };

    if(m_audioRecording.stream != nullptr) {
        SDL_DestroyAudioStream(m_audioRecording.stream);
        m_audioRecording.stream = nullptr;
    }

    if(m_audioRecording.device != 0) {
        SDL_CloseAudioDevice(m_audioRecording.device);
        m_audioRecording.device = 0;
    }

    if(m_audioRecording.buffer != nullptr) {
        free(m_audioRecording.buffer);
        m_audioRecording.buffer = nullptr;
    }

    m_audioRecording.bufferSize = 0;
}

void Application::setRecordingDevice(Application::RecordingDeviceInfo info) {
    Settings::get()->setSelectedRecordingDevice(info.id);

    const char* deviceName = "(null)";
    if(info.id != 0) {
        if(info.name != nullptr) {
            deviceName = info.name;
        }

        openAudioRecordingDevice();
    }
    else {
        closeAudioRecordingDevice();
    }

    changeStatus(std::format("Recording Device: {}", deviceName), std::chrono::milliseconds(1500));
}