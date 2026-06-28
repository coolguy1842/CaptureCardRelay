#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__
#define SDL_MAIN_NOIMPL

#include <clay.h>

#include <clay_renderer_SDL3.hpp>
#include <frame_limiter.hpp>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <settings.hpp>
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
    // UI related
    const Clay_ElementId fpsSliderTrackID          = CLAY_ID("FPSSliderTrack");
    const Clay_ElementId volumeSliderTrackID       = CLAY_ID("VolumeSliderTrack");
    const Clay_TextElementConfig defaultTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0xFF, 0xFF, 0xFF, 0xFF },
        .fontSize  = 24,
    });

    const Clay_TextElementConfig selectedTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0x9F, 0x9F, 0x9F, 0xFF },
        .fontSize  = static_cast<uint16_t>(defaultTextConfig.fontSize + 2),
    });

    const Clay_TextElementConfig hoveredTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0xBF, 0xBF, 0xBF, 0xFF },
        .fontSize  = static_cast<uint16_t>(defaultTextConfig.fontSize + 1),
    });

    bool Clay_MouseClicked();
    bool Clay_MouseHeld();
    // only if current element itself is clicked, not including children
    bool Clay_DirectlyClicked();

    void updateUI();
    void updateSettingsUI();

    void BuildCameraLabel(const CameraInfo& info);
    void BuildCameraSettings();

    void BuildRecordingDeviceLabel(const RecordingDeviceInfo& info);
    void BuildRecordingDeviceSettings();

    void BuildDisplayModeLabel(const CameraDisplayMode& displayMode);
    void BuildDisplayModeSettings();

    void BuildFrameLimitTypeLabel(const FrameLimitType& type);
    void BuildFrameLimiterSettings();

    void BuildFullscreenSettings();
    void BuildVolumeSettings();

    void BuildSettingsMenu();

    void BuildStatus();
    Clay_RenderCommandArray buildUI();

private:
    void initCameras();

    void openCamera();
    void closeCamera(bool lock = true);

    void setCamera(CameraInfo info);
    void setRecordingDevice(RecordingDeviceInfo info);

    void updateCameraDisplayMode(CameraDisplayMode mode);
    void updateFrameLimiter(FrameLimitInfo info);

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

    bool m_isFullscreen = false;
    bool m_shiftHeld    = false;

    bool m_slidingVolume = false;
    bool m_slidingFPS    = false;

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
    SDL_Cursor* m_nextCursor    = nullptr;

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

    FrameLimitInfo m_frameLimitInfo;
    float m_fpsSliderPosition = 0.0f;

    std::string m_fpsText;

    FrameLimiter m_frameLimiter;

    std::vector<CameraInfo> m_cameras;

    std::vector<SDL_AudioDeviceID> m_playbackDevices;
    std::vector<RecordingDeviceInfo> m_recordingDevices;

    std::shared_ptr<CustomElementData> m_cameraData;

    CameraInfo m_currentCamera                   = { .id = 0, .name = "(null)" };
    RecordingDeviceInfo m_currentRecordingDevice = { .id = 0, .name = "(null)" };

    std::unordered_map<SDL_EventType, std::vector<std::pair<EventHandler, void*>>> m_eventHandlers;
};

#endif