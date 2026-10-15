#include <application.hpp>
#include <format>
#include <span>

void Application::onRecordingCallback(void* userdata, SDL_AudioStream* stream, int available, int totalAmount) { reinterpret_cast<Application*>(userdata)->recordingCallbackHandler(stream, available, totalAmount); }
void Application::recordingCallbackHandler(SDL_AudioStream* stream, int available, int /*totalAmount*/) {
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

        SDL_GetAudioStreamData(stream, frame->data(), m_audioBufferSizeBytes);

        m_audioBuffers.push(frame);
        available -= m_audioBufferSizeBytes;
    }
}

void Application::initAudioRecordingDevices() {
    std::vector<SDL_AudioDeviceID> recordingDevices;
    m_recordingDevices.clear();

    {
        int recordingDeviceCount               = 0;
        SDL_AudioDeviceID* recordingDevicesRaw = SDL_GetAudioRecordingDevices(&recordingDeviceCount);

        auto recordingDevicesSpan = std::span(recordingDevicesRaw, static_cast<size_t>(recordingDeviceCount));
        recordingDevices.assign(recordingDevicesSpan.begin(), recordingDevicesSpan.end());

        if(recordingDevicesRaw == nullptr) {
            SDL_free(recordingDevicesRaw);

            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't enumerate recording devices: %s", SDL_GetError());
            setShouldQuit(true);

            return;
        }

        SDL_free(recordingDevicesRaw);
    }

    for(const SDL_AudioDeviceID& device : recordingDevices) {
        const char* name = SDL_GetAudioDeviceName(device);

        if(name == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get name of recording device with id: %d: %s", device, SDL_GetError());
            name = "";
        }

        m_recordingDevices.push_back({ .id = device, .name = name });
    }
}

void Application::openAudioRecordingDevice() {
    // cleanup old data
    closeAudioRecordingDevice();
    initAudioRecordingDevices();

    if(m_recordingDevices.empty()) {
        return;
    }

    RecordingDeviceInfo deviceInfo = m_recordingDevices.empty() ? RecordingDeviceInfo{ .id = SDL_AUDIO_DEVICE_DEFAULT_RECORDING, .name = "(default)" } : m_recordingDevices.front();
    const std::string selectedName = m_settings.getSelectedRecordingDevice();
    for(const auto& device : m_recordingDevices) {
        if(selectedName == device.name) {
            deviceInfo = device;

            break;
        }
    }

    if(deviceInfo.id == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid recording device id found: 0");
        return;
    }

    m_audioRecording.device = SDL_OpenAudioDevice(deviceInfo.id, &m_audioSpec);
    if(m_audioRecording.device == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't open recording device: %s", SDL_GetError());
        return;
    }

    SDL_Log("Opened recording device: %s", deviceInfo.name);
    SDL_GetAudioDeviceFormat(m_audioRecording.device, &m_audioRecording.spec, nullptr);

    m_currentRecordingDevice = deviceInfo;

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