#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__
#define SDL_MAIN_NOIMPL

#include <clay.h>

#include <clay_renderer_SDL3.hpp>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
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
    struct CameraInfo {
        SDL_CameraID id;
        const char* name;
    };

    struct RecordingDeviceInfo {
        SDL_AudioDeviceID id;
        const char* name;
    };

    struct AudioData {
        SDL_AudioDeviceID device = 0;
        SDL_AudioSpec spec;

        SDL_AudioStream* stream = nullptr;

        int bufferSize;
        Uint8* buffer;
    };

private:
    void initCameras();

    void openCamera();
    void closeCamera(bool lock = true);

    void setCamera(CameraInfo info);
    void setRecordingDevice(RecordingDeviceInfo info);

    void initAudioPlaybackDevices();

    void openAudioPlaybackDevice();
    void closeAudioPlaybackDevice();

    void initAudioRecordingDevices();

    void openAudioRecordingDevice();
    void closeAudioRecordingDevice();

    void changeStatus(std::string text, std::chrono::milliseconds timeToExpire);

    void setFullscreen(bool fullscreen = true);

    void setVolume(int volume, bool showStatus = true, bool save = true);
    void updateVolume(bool showStatus = true);

    void playbackCallbackHandler(SDL_AudioStream* stream, int additional_amount, int total_amount);
    void recordingCallbackHandler(SDL_AudioStream* stream, int additional_amount, int total_amount);

private:
    static void onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
    static void onRecordingCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

private:
    bool m_shouldQuit     = false;
    bool m_settingsActive = false;

    bool m_shouldHideCursor = true;
    bool m_mouseHeld        = false;
    bool m_mouseClicked     = false;

    bool m_isFullscreen = false;
    bool m_shiftHeld    = false;

    bool m_slidingVolume = false;

    std::chrono::time_point<std::chrono::system_clock> m_showCursorExpire;

    float m_cursorX = 0.0f;
    float m_cursorY = 0.0f;

    float m_mouseWheelX = 0.0f;
    float m_mouseWheelY = 0.0f;

    const Clay_ElementId m_invalidDropdown = CLAY_ID("__InvalidID__");
    Clay_ElementId m_activeDropdown        = m_invalidDropdown;

    SDL_Window* m_window = nullptr;
    Clay_SDL3RendererData m_renderData;

    SDL_Cursor* m_pointerCursor = nullptr;
    SDL_Cursor* m_defaultCursor = nullptr;
    SDL_Cursor* m_currentCursor = nullptr;

    std::mutex m_streamMutex;
    std::mutex m_audioMutex;

    static constexpr SDL_AudioSpec m_audioSpec = { SDL_AUDIO_S16, 2, 48000 };
    static constexpr size_t audioBufferSize    = 64;
    static constexpr size_t maxAudioBuffers    = 64;

    std::list<std::array<Uint16, audioBufferSize>> m_audioBuffers;

    struct {
        std::string text = "";
        std::optional<std::chrono::time_point<std::chrono::system_clock>> expire;
    } m_status;

    int m_width;
    int m_height;

    AudioData m_audioPlayback;
    AudioData m_audioRecording;

    int m_volume = 100;
    std::string m_volumeText;

    bool m_volumeSnapped        = false;
    bool m_volumeFreeDuringSnap = false;
    bool m_volumeInSnapRange    = false;

    std::vector<CameraInfo> m_cameras;

    std::vector<SDL_AudioDeviceID> m_playbackDevices;
    std::vector<RecordingDeviceInfo> m_recordingDevices;

    std::shared_ptr<CustomElementData> m_cameraData;

    CameraInfo m_currentCamera                   = { .id = 0, .name = "(null)" };
    RecordingDeviceInfo m_currentRecordingDevice = { .id = 0, .name = "(null)" };

    std::unordered_map<SDL_EventType, std::vector<std::pair<EventHandler, void*>>> m_eventHandlers;
};

#endif