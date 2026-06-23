#include <application.hpp>
#include <chrono>
#include <settings.hpp>

void Application::handleEvent(SDL_Event* event) {
    auto it = m_eventHandlers.find((SDL_EventType)event->type);
    if(it != m_eventHandlers.end()) {
        std::vector<std::pair<EventHandler, void*>> handlers = it->second;

        for(auto it = handlers.begin(); it != handlers.end();) {
            std::pair<EventHandler, void*> handler = *it;
            if(!(handler.first(event, handler.second))) {
                it = handlers.erase(it);
                continue;
            }

            it++;
        }
    }

    switch(event->type) {
    case SDL_EVENT_QUIT:
        setShouldQuit(true);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        m_width  = event->window.data1;
        m_height = event->window.data2;

        Clay_SetLayoutDimensions(Clay_Dimensions{
            static_cast<float>(m_width),
            static_cast<float>(m_height),
        });

        break;
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        m_isFullscreen = true;
        Settings::get()->setFullscreen(true);

        break;
    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        m_isFullscreen = false;
        Settings::get()->setFullscreen(false);

        break;
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        m_shouldHideCursor = event->type != SDL_EVENT_MOUSE_BUTTON_DOWN;
        m_showCursorExpire = std::chrono::system_clock::now() + std::chrono::milliseconds(1000);

        SDL_ShowCursor();

        break;
    case SDL_EVENT_MOUSE_WHEEL:
        m_mouseWheelX += event->wheel.y * m_shiftHeld;
        m_mouseWheelY += event->wheel.y * !m_shiftHeld;

        break;
    case SDL_EVENT_CAMERA_DEVICE_ADDED:
    case SDL_EVENT_CAMERA_DEVICE_REMOVED:
        openCamera();

        break;
    case SDL_EVENT_CAMERA_DEVICE_APPROVED:
        SDL_Log("Opened camera: %s", m_currentCamera.name);
        if(!SDL_GetCameraFormat(m_cameraData->camera.device, &m_cameraData->camera.spec)) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Camera approved but failed to get format");
            break;
        }

        m_cameraData->camera.approved = true;
        break;
    case SDL_EVENT_CAMERA_DEVICE_DENIED:
        SDL_Log("Camera %s was rejected", SDL_GetCameraName(SDL_GetCameraID(m_cameraData->camera.device)));
        closeCamera();

        break;
    case SDL_EVENT_AUDIO_DEVICE_ADDED:
    case SDL_EVENT_AUDIO_DEVICE_REMOVED:
        openAudioPlaybackDevice();
        openAudioRecordingDevice();

    case SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED: {
        auto lock = std::unique_lock(m_streamMutex);
        if(m_audioRecording.stream != nullptr) {
            SDL_SetAudioStreamFormat(m_audioRecording.stream, &m_audioRecording.spec, &m_audioSpec);
        }

        break;
    }
    case SDL_EVENT_KEY_DOWN:
        switch(event->key.key) {
        case SDLK_LEFT: {
            if(m_cameras.empty()) {
                break;
            }

            {
                auto lock = std::unique_lock(m_cameraData->camera.mutex);
                if(!m_cameraData->camera.approved && m_cameraData->camera.device != nullptr) {
                    closeCamera(false);
                }
            }

            initCameras();

            if(m_cameraData->camera.device != nullptr) {
                SDL_CameraID currentCamera = SDL_GetCameraID(m_cameraData->camera.device);

                auto it = std::find_if(m_cameras.begin(), m_cameras.end(), [currentCamera](const auto& info) { return info.id == currentCamera; });
                if(it == m_cameras.end()) {
                    it = m_cameras.begin();
                }

                if(it != m_cameras.end()) {
                    it = it + 1;
                    if(it == m_cameras.end()) {
                        it = m_cameras.begin();
                    }

                    setCamera(*it);
                }
                else {
                    setCamera({ .id = 0, .name = "" });
                }
            }

            break;
        }
        case SDLK_RIGHT: {
            if(m_recordingDevices.empty()) {
                break;
            }

            if(m_audioRecording.device != 0) {
                auto it = std::find_if(m_recordingDevices.begin(), m_recordingDevices.end(), [this](const auto& info) { return m_currentRecordingDevice.id == info.id; });

                if(it == m_recordingDevices.end()) {
                    it = m_recordingDevices.begin();
                }

                if(it != m_recordingDevices.end()) {
                    it = it + 1;
                    if(it == m_recordingDevices.end()) {
                        it = m_recordingDevices.begin();
                    }

                    setRecordingDevice(*it);
                }
                else {
                    setRecordingDevice({ .id = 0, .name = "" });
                }
            }

            break;
        }
        case SDLK_UP:
            setVolume(std::min(Settings::get()->getVolume() + 5, 150));
            goto volumeStatus;
        case SDLK_DOWN:
            setVolume(std::max(Settings::get()->getVolume() - 5, 0));
            goto volumeStatus;
        volumeStatus:
            updateVolume();

            break;
        case SDLK_F11: {
            setFullscreen(!m_isFullscreen);
            break;
        }
        case SDLK_F12:
            Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());

            break;
        case SDLK_O:
            m_settingsActive = !m_settingsActive;
            m_activeDropdown = m_invalidDropdown;

            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            m_shiftHeld = true;
            break;
        default: break;
        }
        break;
    case SDL_EVENT_KEY_UP:
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        m_shiftHeld = false;
        break;
    default: break;
    }
}

void Application::registerEventHandler(SDL_EventType type, const Application::EventHandler& handler, void* extraData) {
    if(m_eventHandlers.find(type) == m_eventHandlers.end()) {
        m_eventHandlers[type] = {};
    }

    m_eventHandlers[type].push_back(std::make_pair(handler, extraData));
}