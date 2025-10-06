#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <application.hpp>
#include <chrono>
#include <cstring>
#include <iterator>
#include <utility>
#include <vector>

#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_camera.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "settings.hpp"

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
    case SDL_EVENT_MOUSE_MOTION:
        Clay_SetPointerState(
            Clay_Vector2{
                event->motion.x,
                event->motion.y,
            },
            event->motion.state & SDL_BUTTON_LMASK
        );

        SDL_ShowCursor();

        if(m_cursorHideTimer != 0) {
            SDL_RemoveTimer(m_cursorHideTimer);
            m_cursorHideTimer = 0;
        }

        m_cursorHideTimer = SDL_AddTimer(1000, &Application::onCursorHideCallback, this);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        Clay_SetPointerState(
            Clay_Vector2{
                event->button.x,
                event->button.y,
            },
            event->button.button == SDL_BUTTON_LEFT
        );

        break;
    case SDL_EVENT_MOUSE_WHEEL:
        Clay_UpdateScrollContainers(true, Clay_Vector2{ event->wheel.x, event->wheel.y }, 0.01f);

        break;
    case SDL_EVENT_CAMERA_DEVICE_REMOVED:
        openCamera();

        break;
    case SDL_EVENT_AUDIO_DEVICE_REMOVED:
        openAudioPlaybackDevice();
        openAudioRecordingDevice();

    case SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED:
    case SDL_EVENT_AUDIO_DEVICE_ADDED:
        if(m_audioRecording.stream != nullptr) {
            SDL_SetAudioStreamFormat(m_audioRecording.stream, &m_audioRecording.spec, &m_audioSpec);
        }

        break;
    case SDL_EVENT_KEY_DOWN:
        switch(event->key.key) {
        case SDLK_LEFT: {
            size_t idx = 0;
            if(m_cameraData->camera.device != nullptr) {
                auto it = std::find(m_cameras.begin(), m_cameras.end(), SDL_GetCameraID(m_cameraData->camera.device));
                if(it != m_cameras.end()) {
                    idx = std::distance(m_cameras.begin(), it);
                }
                else {
                    idx = -1;
                }
            }

            idx = (idx + 1) % m_cameras.size();
            Settings::get()->setSelectedCamera(m_cameras[idx]);
            openCamera();

            changeStatus(std::string("Camera: ") + SDL_GetCameraName(m_cameras[idx]), std::chrono::milliseconds(1500));

            break;
        }
        case SDLK_RIGHT: {
            size_t idx = 0;
            if(m_audioRecording.device != 0) {
                std::string name = SDL_GetAudioDeviceName(m_audioRecording.device);
                auto it          = std::find_if(m_recordingDevices.begin(), m_recordingDevices.end(), [name](SDL_AudioDeviceID id) {
                    return name == SDL_GetAudioDeviceName(id);
                });

                if(it != m_recordingDevices.end()) {
                    idx = std::distance(m_recordingDevices.begin(), it);
                }
                else {
                    idx = -1;
                }
            }

            idx = (idx + 1) % m_recordingDevices.size();
            Settings::get()->setSelectedRecordingDevice(m_recordingDevices[idx]);
            openAudioRecordingDevice();

            changeStatus(std::string("Recording Device: ") + SDL_GetAudioDeviceName(m_recordingDevices[idx]), std::chrono::milliseconds(1500));

            break;
        }
        case SDLK_UP:
            Settings::get()->setVolume(std::min(Settings::get()->getVolume() + 5, 150));
            changeStatus(std::string("Volume: ") + std::to_string(Settings::get()->getVolume()) + "%", std::chrono::milliseconds(1500));

            updateVolume();

            break;
        case SDLK_DOWN:
            Settings::get()->setVolume(std::max(Settings::get()->getVolume() - 5, 0));

            changeStatus(std::string("Volume: ") + std::to_string(Settings::get()->getVolume()) + "%", std::chrono::milliseconds(1500));
            updateVolume();

            break;
        case SDLK_F12:
            Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());

            break;
        default: break;
        }
        break;
    }
}

void Application::registerEventHandler(SDL_EventType type, const Application::EventHandler& handler, void* extraData) {
    if(m_eventHandlers.find(type) == m_eventHandlers.end()) {
        m_eventHandlers[type] = {};
    }

    m_eventHandlers[type].push_back(std::make_pair(handler, extraData));
}