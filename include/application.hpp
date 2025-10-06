#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__
#define SDL_MAIN_NOIMPL

#include <clay.h>

#include <array>
#include <chrono>
#include <clay_renderer_SDL3.hpp>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Application {
public:
    // if true, keep event handler, else remove from handlers
    using EventHandler = std::function<bool(SDL_Event* event, void* extraData)>;

    Application();
    virtual ~Application();

    bool loop();

    bool getShouldQuit() const;
    void setShouldQuit(bool shouldQuit = true);

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
    void updateVolume();

    void playbackCallbackHandler(SDL_AudioStream* stream, int additional_amount, int total_amount);
    void recordingCallbackHandler(SDL_AudioStream* stream, int additional_amount, int total_amount);

    Uint32 statusStep();

private:
    static void onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
    static void onRecordingCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

    static Uint32 onStatusStepCallback(void* userdata, SDL_TimerID timerID, Uint32 interval);

private:
    bool m_shouldQuit;

    SDL_Window* m_window = nullptr;

    std::mutex m_audioMutex;
    Clay_SDL3RendererData m_renderData;

    static constexpr SDL_AudioSpec m_audioSpec = { SDL_AUDIO_S16, 2, 48000 };
    static constexpr size_t audioBufferSize    = 64;
    static constexpr size_t maxAudioBuffers    = 64;

    std::list<std::array<Uint16, audioBufferSize>> m_audioBuffers;

    struct {
        std::string text = "";
        std::chrono::time_point<std::chrono::system_clock> expire;

        float animationProgress = 0.0f;
        bool animationReverse   = false;
    } m_status;

    int m_width;
    int m_height;

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

        int bufferSize;
        Uint8* buffer;
    } m_audioRecording;

    SDL_TimerID m_statusStepTimer = 0;
    std::chrono::time_point<std::chrono::system_clock> m_showCursorExpire;

    std::vector<SDL_CameraID> m_cameras;

    std::vector<SDL_AudioDeviceID> m_playbackDevices;
    std::vector<SDL_AudioDeviceID> m_recordingDevices;

    std::shared_ptr<CustomElementData> m_cameraData;

    std::unordered_map<SDL_EventType, std::vector<std::pair<EventHandler, void*>>> m_eventHandlers;
};

#endif