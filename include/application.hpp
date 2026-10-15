#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__
#define SDL_MAIN_NOIMPL

#include <clay.h>

#include <clay_renderer_SDL3.hpp>
#include <frame_limiter.hpp>
#include <memory>
#include <optional>
#include <queue>
#include <settings.hpp>
#include <string>
#include <vector>

#ifdef ENABLE_PIPEWIRE
#include <rohrkabel/link/link.hpp>
#include <rohrkabel/node/node.hpp>
#include <rohrkabel/port/port.hpp>

#include <rohrkabel/metadata/events.hpp>
#include <rohrkabel/metadata/metadata.hpp>

#include <rohrkabel/registry/events.hpp>
#include <rohrkabel/registry/registry.hpp>
#include <rohrkabel/spa/pod/prop.hpp>
#endif

class Application {
public:
    explicit Application(const std::optional<std::string>& settingsPath);
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    virtual ~Application();

    SDL_AppResult loop();

    bool getShouldQuit() const;
    void setShouldQuit(bool shouldQuit = true);

    SDL_AppResult handleEvent(SDL_Event* event);

protected:
    virtual void update();
    virtual void render();

private:
    struct CameraInfo {
        SDL_CameraID id  = 0;
        const char* name = nullptr;
    };

    struct RecordingDeviceInfo {
        SDL_AudioDeviceID id = 0;
        const char* name     = nullptr;
    };

    struct AudioData {
        SDL_AudioDeviceID device = 0;
        SDL_AudioSpec spec       = {};

        SDL_AudioStream* stream = nullptr;
    };

#ifdef ENABLE_PIPEWIRE
    struct PipewireDevice {
        std::string name;
        uint32_t id;

        pipewire::node node;
    };

    struct PipewireData {
        std::shared_ptr<pipewire::main_loop> loop;
        std::shared_ptr<pipewire::context> context;
        std::shared_ptr<pipewire::core> core;
        std::optional<pipewire::registry> registry;
        std::optional<pipewire::metadata> metadata;

        std::shared_ptr<pipewire::registry_listener> registryListener;
        std::shared_ptr<pipewire::metadata_listener> metadataListener;

        struct {
            std::optional<pipewire::node> node;

            std::optional<pipewire::port> inFL;
            std::optional<pipewire::port> inFR;

            std::optional<pipewire::port> outFL;
            std::optional<pipewire::port> outFR;
        } virtualMic;

        std::string defaultSinkName;
        std::vector<PipewireDevice> sinks;
        std::vector<PipewireDevice> sources;
        std::vector<pipewire::port> ports;

        PipewireDevice* currentSource = nullptr;
        uint32_t lastSinkID           = static_cast<uint32_t>(-1);

        // cleanup the links when changing microphones
        std::vector<pipewire::link> links;
    } m_pipewire;

    bool m_usingPipewire = false;
#endif

private:
    // UI related
    const Clay_ElementId fpsSliderTrackID          = CLAY_ID("FPSSliderTrack");
    const Clay_ElementId volumeSliderTrackID       = CLAY_ID("VolumeSliderTrack");
    const Clay_TextElementConfig defaultTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0xFF, 0xFF, 0xFF, 0xFF },
        .fontSize  = 24,
    });

    const Clay_TextElementConfig lockedTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0x8F, 0x8F, 0x8F, 0xFF },
        .fontSize  = defaultTextConfig.fontSize,
    });

    const Clay_TextElementConfig selectedTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0x9F, 0x9F, 0x9F, 0xFF },
        .fontSize  = defaultTextConfig.fontSize,
    });

    const Clay_TextElementConfig hoveredTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0xBF, 0xBF, 0xBF, 0xFF },
        .fontSize  = defaultTextConfig.fontSize,
    });

    bool m_showFrametime = false;

    bool m_slidingFPS    = false;
    bool m_slidingVolume = false;

    bool m_volumeSnapped        = false;
    bool m_volumeFreeDuringSnap = false;
    bool m_volumeInSnapRange    = false;

    static bool Clay_MouseClicked();
    static bool Clay_MouseHeld();
    // only if current element itself is clicked, not including children
    static bool Clay_DirectlyClicked();

    static bool Clay_MouseReleasedNow();
    static bool Clay_MouseReleased();

    void updateUI();
    void updateSettingsUI();

    uint32_t m_currentScrollBar           = 0;
    SDL_FPoint m_currentScrollClickOrigin = { 0.0f, 0.0f };
    void Build_ScrollBar(Clay_ElementId id, bool vertical = true);

    void BuildCameraLabel(const CameraInfo& info);
    void BuildCameraSettings();

    void BuildPixelFormatLabel(const PixelFormat& format);
    void BuildPixelFormatSettings();

    void BuildSDLRecordingDeviceLabel(const RecordingDeviceInfo& info);
#ifdef ENABLE_PIPEWIRE
    void BuildPipewireRecordingDeviceLabel(const PipewireDevice& device);
#endif

    void BuildRecordingDeviceSettings();

    void BuildDisplayModeLabel(const CameraDisplayMode& displayMode);
    void BuildDisplayModeSettings();

    void BuildScaleModeLabel(const SDL_ScaleMode& scaleMode);
    void BuildScaleModeSettings();

    void BuildFrameLimitTypeLabel(const FrameLimitType& type);
    void BuildFrameLimiterSettings();

    void BuildFullscreenSettings();
    void BuildVolumeSettings();

    void BuildSettingsMenu();

    void BuildStatus();
    Clay_RenderCommandArray buildUI();

    void checkShouldRenderClay();

private:
    void initCameras();

    void openCamera();
    void closeCamera();

    void setCamera(CameraInfo info);
    void setRecordingDeviceSDL(RecordingDeviceInfo info);

#ifdef ENABLE_PIPEWIRE
    void setRecordingDevicePipewire(const PipewireDevice& device);
#endif

    void updateCameraDisplayRect();
    void updateCameraTexture();
    void cleanupCameraTexture();
    // should be called every frame
    void renderCameraToTexture();

    void updateCameraDisplayMode(CameraDisplayMode mode);
    void updateCameraScaleMode(SDL_ScaleMode mode);
    void updateCameraPixelFormat(PixelFormat format);

#ifndef __EMSCRIPTEN__
    void updateFrameLimiter(FrameLimitInfo info);
#endif

#ifdef ENABLE_PIPEWIRE
    void initPipewire();
    void updatePipewireLink();
    void cleanupPipewire();

    void onPipewireGlobal(const pipewire::global& global);
    void onPipewireGlobalRemoved(uint32_t id);

    int onPipewireMetadataProperty(const char* key, const pipewire::metadata_property& property);
#endif

    void openAudioPlaybackDevice();
    void closeAudioPlaybackDevice();

    void initAudioRecordingDevices();

    void openAudioRecordingDevice();
    void closeAudioRecordingDevice();

    void changeStatus(const std::string& text, std::chrono::milliseconds timeToExpire);

    void setFullscreen(bool fullscreen = true);

    void setVolume(int volume, bool showStatus = true, bool save = true);

    float getCubicVolume() const;
    void _updateVolumeSDL();
#ifdef ENABLE_PIPEWIRE
    void _updateVolumePipewire();
#endif

    void updateVolume(bool showStatus = true);

    void playbackCallbackHandler(SDL_AudioStream* stream, int needed, int totalAmount);
    void recordingCallbackHandler(SDL_AudioStream* stream, int available, int totalAmount);

private:
    static void onPlaybackCallback(void* userdata, SDL_AudioStream* stream, int needed, int totalAmount);
    static void onRecordingCallback(void* userdata, SDL_AudioStream* stream, int available, int totalAmount);

private:
    Settings m_settings;

    bool m_shouldQuit     = false;
    bool m_settingsActive = false;
    bool m_statusActive   = false;

    bool m_shouldHideCursor = true;
    bool m_mouseHeld        = false;

    bool m_isFullscreen = false;
    bool m_shiftHeld    = false;

    bool m_shouldRenderClay = false;

    std::string m_frameTimeText;
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

    using AudioType  = float;
    using AudioFrame = std::shared_ptr<std::vector<AudioType>>;

    static constexpr SDL_AudioSpec m_audioSpec = { SDL_AUDIO_F32, 2, 48000 };
    static constexpr size_t maxAudioBuffers    = 32;

    std::queue<AudioFrame> m_audioBuffers;
    std::queue<AudioFrame> m_freeAudioBuffers;

    AudioFrame m_emptyAudioBuffer;
    int m_audioBufferSizeBytes;

    struct {
        std::string text;
        std::optional<std::chrono::time_point<std::chrono::system_clock>> expire;
    } m_status;

    int m_width;
    int m_height;

    AudioData m_audioPlayback;
    AudioData m_audioRecording;

    int m_volume = 100;
    std::string m_volumeText;

    FrameLimiter m_frameLimiter;
#ifndef __EMSCRIPTEN__
    FrameLimitInfo m_frameLimitInfo = { .type = FrameLimitType::FRAME_LIMIT_NONE, .fps = 0.0f };
#endif

    float m_fpsSliderPosition = 0.0f;
    std::string m_fpsText;

    std::vector<CameraInfo> m_cameras;
    std::vector<SDL_AudioDeviceID> m_playbackDevices;
    std::vector<RecordingDeviceInfo> m_recordingDevices;

    struct {
        SDL_Camera* device   = nullptr;
        SDL_Texture* texture = nullptr;
        SDL_CameraSpec spec  = {};

        bool approved         = false;
        SDL_FRect displayRect = { 0.0f, 0.0f };

        std::vector<Uint8> pixels;
        int pitch = 0;
    } m_camera;

    CameraInfo m_currentCamera                   = { .id = 0, .name = "(null)" };
    RecordingDeviceInfo m_currentRecordingDevice = { .id = 0, .name = "(null)" };

    // to stop memory leak from displaying
    std::unique_ptr<std::vector<char>> m_clayMemory;
};

#endif