#include <application.hpp>
#include <chrono>

void Application::handleEvent(SDL_Event* event) {
    switch(event->type) {
    case SDL_EVENT_QUIT:
        setShouldQuit(true);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        m_width  = event->window.data1;
        m_height = event->window.data2;

        updateCameraDisplayRect();
        Clay_SetLayoutDimensions(Clay_Dimensions{
            static_cast<float>(m_width),
            static_cast<float>(m_height),
        });

        break;
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        if(!m_settings.canSetFullscreen()) {
            break;
        }

        m_isFullscreen = true;
        m_settings.setFullscreen(true);

        break;
    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        if(!m_settings.canSetFullscreen()) {
            break;
        }

        m_isFullscreen = false;
        m_settings.setFullscreen(false);

        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        m_mouseHeld        = event->type != SDL_EVENT_MOUSE_BUTTON_UP;
        m_shouldHideCursor = !m_mouseHeld;

        goto cursorMain;
    case SDL_EVENT_MOUSE_MOTION:
        m_cursorX = event->motion.x;
        m_cursorY = event->motion.y;

    cursorMain:
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

        SDL_CameraSpec spec;
        if(!SDL_GetCameraFormat(m_camera.device, &spec)) {
            m_camera.spec = {};
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Camera approved but failed to get format");
            break;
        }

        m_camera.spec = spec;
        SDL_Log("Camera spec: %dx%d@%0.2f - Format: %s - Colorspace: %s", spec.width, spec.height, spec.framerate_numerator / static_cast<float>(spec.framerate_denominator), pixelFormatName(spec.format), colorspaceName(spec.colorspace));

        m_camera.approved = true;
        updateCameraTexture();

        break;
    case SDL_EVENT_CAMERA_DEVICE_DENIED:
        SDL_Log("Camera %s was rejected", SDL_GetCameraName(SDL_GetCameraID(m_camera.device)));

        closeCamera();
        updateFrameLimiter(m_frameLimitInfo);

        break;
    case SDL_EVENT_AUDIO_DEVICE_ADDED:
    case SDL_EVENT_AUDIO_DEVICE_REMOVED:
        openAudioPlaybackDevice();
        openAudioRecordingDevice();

        // fall through
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
            if(m_cameras.empty() || !m_settings.canSetSelectedCamera()) {
                break;
            }

            {
                auto lock = std::unique_lock(m_camera.mutex);
                if(!m_camera.approved && m_camera.device != nullptr) {
                    closeCamera(false);
                }
            }

            initCameras();
            if(m_camera.device != nullptr) {
                SDL_CameraID currentCamera = SDL_GetCameraID(m_camera.device);

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
            if(m_recordingDevices.empty() || !m_settings.canSetRecordingDevice()) {
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
            if(!m_settings.canSetVolume()) {
                break;
            }

            setVolume(std::min(m_settings.getVolume() + 5, 150));
            updateVolume();

            break;
        case SDLK_DOWN:
            if(!m_settings.canSetVolume()) {
                break;
            }

            setVolume(std::max(m_settings.getVolume() - 5, 0));
            updateVolume();

            break;
        case SDLK_F11:
            if(!m_settings.canSetFullscreen()) {
                break;
            }

            setFullscreen(!m_isFullscreen);
            break;
#ifdef DEBUG
        case SDLK_F12:
            Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
            m_showFrametime = Clay_IsDebugModeEnabled();

            break;
        case SDLK_F3:
            m_showFrametime = !m_showFrametime;

            break;
#endif
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
