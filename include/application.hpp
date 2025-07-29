#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL_render.h"

class Application {
public:
    // if true, keep event handler, else remove from handlers
    using EventHandler = std::function<bool(SDL_Event* event, void* extraData)>;

    Application();
    virtual ~Application();

    bool loop();

    bool getShouldQuit() const;
    void setShouldQuit(bool shouldQuit);

protected:
    virtual void update();
    virtual void render();

    void handleEvent(SDL_Event* event);
    void registerEventHandler(SDL_EventType type, const EventHandler& handler, void* extraData = nullptr);

private:
    void initCameras();

    void openCamera();
    void closeCamera();

    void initAudioPlaybackDevices();

    void openAudioPlaybackDevice();
    void closeAudioPlaybackDevice();

    void initAudioRecordingDevices();

    void openAudioRecordingDevice();
    void closeAudioRecordingDevice();

    void changeStatus(std::string text, std::chrono::milliseconds timeToExpire);

private:
    bool m_shouldQuit;

    SDL_Window* m_window     = nullptr;
    SDL_Renderer* m_renderer = nullptr;

    TTF_Font* m_font = nullptr;

    struct {
        std::string text;
        std::chrono::time_point<std::chrono::system_clock> expire;

        SDL_Texture* texture;
    } m_status;

    bool m_fontInitialized;

    int m_width;
    int m_height;

    struct {
        SDL_Camera* device;
        SDL_CameraSpec spec;
        bool approved;

        SDL_Texture* texture = nullptr;
    } m_camera;

    struct {
        SDL_AudioDeviceID device = 0;
        SDL_AudioSpec spec;

        SDL_AudioStream* stream = nullptr;

        int bufferSize;
        Uint8* buffer;
    } m_audioPlayback;

    struct {
        SDL_AudioDeviceID device = 0;
        SDL_AudioSpec spec;

        SDL_AudioStream* stream = nullptr;
    } m_audioRecording;

    std::vector<SDL_CameraID> m_cameras;

    std::vector<SDL_AudioDeviceID> m_playbackDevices;
    std::vector<SDL_AudioDeviceID> m_recordingDevices;

    std::unordered_map<SDL_EventType, std::vector<std::pair<EventHandler, void*>>> m_eventHandlers;
};

#endif