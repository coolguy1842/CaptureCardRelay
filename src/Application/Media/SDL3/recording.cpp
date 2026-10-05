#include <application.hpp>
#include <format>

void Application::onRecordingCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount) { ((Application*)userdata)->recordingCallbackHandler(stream, additionalAmount, totalAmount); }
void Application::recordingCallbackHandler(SDL_AudioStream* stream, int available, int) {
    while(available >= m_audioBufferSizeBytes) {
        AudioFrame frame;

        if(!m_freeAudioBuffers.empty()) {
            frame = m_freeAudioBuffers.front();
            m_freeAudioBuffers.pop();
        }
        else {
            // take oldest from audio buffers
            frame = m_audioBuffers.front();
            m_audioBuffers.pop();
        }

        SDL_GetAudioStreamData(stream, frame.get(), m_audioBufferSizeBytes);

        m_audioBuffers.push(frame);
        available -= m_audioBufferSizeBytes;
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
    // cleanup old data
    closeAudioRecordingDevice();
    initAudioRecordingDevices();

    if(m_recordingDevices.empty()) {
        return;
    }

    SDL_AudioDeviceID deviceID = 0;

    int recordingDeviceCount   = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioRecordingDevices(&recordingDeviceCount);

    if(devices == nullptr || recordingDeviceCount == 0) {
        deviceID = 0;
    }
    else {
        const std::string selectedName = m_settings.getSelectedRecordingDevice();

        if(!selectedName.empty()) {
            for(int i = 0; i < recordingDeviceCount; i++) {
                const SDL_AudioDeviceID device = devices[i];

                const char* name = SDL_GetAudioDeviceName(device);
                if(name != nullptr && selectedName == name) {
                    deviceID = device;
                    break;
                }
            }
        }
        else {
            deviceID = devices[0];
        }
    }

    SDL_free(devices);

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
    SDL_GetAudioDeviceFormat(m_audioRecording.device, &m_audioRecording.spec, NULL);

    m_currentRecordingDevice = { .id = deviceID, .name = name };

    m_audioRecording.stream = SDL_CreateAudioStream(&m_audioRecording.spec, &m_audioSpec);
    SDL_BindAudioStream(m_audioRecording.device, m_audioRecording.stream);

    SDL_SetAudioStreamPutCallback(m_audioRecording.stream, &Application::onRecordingCallback, this);
}

void Application::closeAudioRecordingDevice() {
    m_currentRecordingDevice = { .id = 0, .name = "(null)" };

    if(m_audioRecording.stream != nullptr) {
        SDL_DestroyAudioStream(m_audioRecording.stream);
        m_audioRecording.stream = nullptr;
    }

    if(m_audioRecording.device != 0) {
        SDL_CloseAudioDevice(m_audioRecording.device);
        m_audioRecording.device = 0;
    }
}

void Application::setRecordingDeviceSDL(Application::RecordingDeviceInfo info) {
    m_settings.setSelectedRecordingDevice(info.name);

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