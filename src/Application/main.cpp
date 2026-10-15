#include <application.hpp>
#include <cmath>
#include <damase_ttf.hpp>
#include <format>
#include <settings.hpp>

#define CLAY_IMPLEMENTATION
#include <clay.h>

namespace {
void HandleClayErrors(Clay_ErrorData errorData) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", errorData.errorText.chars);
}
}; // namespace

Application::Application(const std::optional<std::string>& settingsPath)
    : m_settings(settingsPath)
    , m_audioBufferSizeBytes(0)
    , m_width(800)
    , m_height(600)
    , m_frameLimiter(false) {
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CAMERA)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FATAL: Couldn't initialize SDL: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    m_window = SDL_CreateWindow("Capture Card Relay", m_width, m_height, SDL_WINDOW_RESIZABLE);
    if(m_window == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FATAL: Couldn't create window: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    setFullscreen(m_settings.isFullscreen());

    m_renderData.renderer = SDL_CreateGPURenderer(nullptr, m_window);
    if(m_renderData.renderer != nullptr) {
        SDL_GPUDevice* gpu = SDL_GetGPURendererDevice(m_renderData.renderer);
        if(gpu == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FATAL: Created renderer with GPU device, but GPU somehow null: %s", SDL_GetError());
            setShouldQuit();

            return;
        }

        const SDL_PropertiesID props = SDL_GetGPUDeviceProperties(gpu);
        if(props != 0) {
            const char* name = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, "");
            SDL_Log("Created renderer with GPU: %s", name);
            SDL_Log("\n");
        }
        else {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get GPU name: %s", SDL_GetError());
        }
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create GPU renderer, using SDL_CreateRenderer: %s", SDL_GetError());

        m_renderData.renderer = SDL_CreateRenderer(m_window, nullptr);
        if(m_renderData.renderer == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FATAL: Couldn't create renderer: %s", SDL_GetError());
            setShouldQuit();

            return;
        }

        SDL_Log("Created renderer.");
    }

    if(!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FATAL: Couldn't initialise SDL_ttf: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    m_renderData.textEngine = TTF_CreateRendererTextEngine(m_renderData.renderer);
    if(m_renderData.textEngine == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FATAL: Failed to create text engine from renderer: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    m_renderData.fonts.push_back(TTF_OpenFontIO(SDL_IOFromConstMem(reinterpret_cast<const void*>(damase_v2), sizeof(damase_v2)), true, 40.0f));
    if(m_renderData.fonts.back() == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "FATAL: Couldn't open font: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    m_volume     = m_settings.getVolume();
    m_volumeText = std::format("{}%", m_volume);

    openCamera();

#ifdef ENABLE_PIPEWIRE
    initPipewire();

    if(!m_usingPipewire) {
#endif
        SDL_InitSubSystem(SDL_INIT_AUDIO);

        openAudioPlaybackDevice();
        openAudioRecordingDevice();
#ifdef ENABLE_PIPEWIRE
    }
#endif

    if(getShouldQuit()) {
        return;
    }

    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_AUDIO_DEVICE_ADDED, SDL_EVENT_AUDIO_DEVICE_ADDED);
    SDL_FlushEvents(SDL_EVENT_CAMERA_DEVICE_ADDED, SDL_EVENT_CAMERA_DEVICE_ADDED);

    const uint32_t arenaSize = Clay_MinMemorySize();
    m_clayMemory             = std::make_unique<std::vector<char>>(static_cast<size_t>(arenaSize));

    const Clay_Arena clayMemory = {
        .capacity = m_clayMemory->size(),
        .memory   = m_clayMemory->data(),
    };

    SDL_GetWindowSize(m_window, &m_width, &m_height);

    Clay_Initialize(clayMemory, { static_cast<float>(m_width), static_cast<float>(m_height) }, { HandleClayErrors });
    Clay_SetMeasureTextFunction(SDL_MeasureText, &m_renderData.fonts);

    m_settings.displayModeChanged.connect<&Application::updateCameraDisplayMode>(this);
    m_settings.scaleModeChanged.connect<&Application::updateCameraScaleMode>(this);
    m_settings.pixelFormatChanged.connect<&Application::updateCameraPixelFormat>(this);

#ifndef __EMSCRIPTEN__
    m_settings.frameLimitInfoChanged.connect<&Application::updateFrameLimiter>(this);
    updateFrameLimiter(m_settings.getFrameLimitInfo());
#else
    SDL_SetRenderVSync(m_renderData.renderer, 1);
#endif

    m_pointerCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    m_defaultCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);

    // init status position for animations as clay wont be called every frame
    buildUI();
}

Application::~Application() {
    setShouldQuit(true);

    closeCamera();
    closeAudioRecordingDevice();
    closeAudioPlaybackDevice();

    SDL_DestroyCursor(m_pointerCursor);
    SDL_DestroyCursor(m_defaultCursor);

    for(auto* font : m_renderData.fonts) {
        if(font == nullptr) {
            continue;
        }

        TTF_CloseFont(font);
    }

    if(m_renderData.textEngine != nullptr) {
        TTF_DestroyRendererTextEngine(m_renderData.textEngine);
    }

    TTF_Quit();

    SDL_DestroyRenderer(m_renderData.renderer);
    SDL_DestroyWindow(m_window);

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_Quit();

    m_clayMemory.reset();
}

void Application::updateCameraDisplayMode(CameraDisplayMode /*mode*/) { updateCameraDisplayRect(); }
void Application::updateCameraScaleMode(SDL_ScaleMode /*scaleMode*/) { updateCameraTexture(); }
void Application::updateCameraPixelFormat(PixelFormat /*format*/) { updateCameraTexture(); }

#ifndef __EMSCRIPTEN__
namespace {
float getCameraRefreshRate(SDL_CameraSpec spec) {
    return static_cast<float>(spec.framerate_numerator) / static_cast<float>(spec.framerate_denominator);
}
}; // namespace

void Application::updateFrameLimiter(FrameLimitInfo info) {
    m_frameLimitInfo = info;
    m_fpsText        = std::format("{} FPS", info.fps);

    switch(info.type) {
    case FRAME_LIMIT_CAMERA:
        if(m_camera.device == nullptr || !m_camera.approved) {
            break;
        }

        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(getCameraRefreshRate(m_camera.spec));

        return;
    case FRAME_LIMIT_CAMERA_X_1_5:
        if(m_camera.device == nullptr || !m_camera.approved) {
            break;
        }

        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(getCameraRefreshRate(m_camera.spec) * 1.5f);

        return;
    case FRAME_LIMIT_CAMERA_X_2_0:
        if(m_camera.device == nullptr || !m_camera.approved) {
            break;
        }

        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(getCameraRefreshRate(m_camera.spec) * 2.0f);

        return;
    case FRAME_LIMIT_VSYNC:
        m_frameLimiter.setFPSLimit(0);
        SDL_SetRenderVSync(m_renderData.renderer, 1);

        return;
    case FRAME_LIMIT_VSYNC_ADAPTIVE:
        m_frameLimiter.setFPSLimit(0);
        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

        return;
    case FRAME_LIMIT_FPS:
        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(info.fps);

        return;
    case FRAME_LIMIT_NONE:
    default:
        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(0);

        return;
    }

    // set to vsync as fallback
    m_frameLimiter.setFPSLimit(0);
    SDL_SetRenderVSync(m_renderData.renderer, 1);
}
#endif

SDL_AppResult Application::loop() {
    update();
    render();

    return SDL_APP_CONTINUE;
}

void Application::update() {
#ifdef ENABLE_PIPEWIRE
    if(m_usingPipewire) {
        m_pipewire.core->run_once();
    }
#endif

    if(!m_settingsActive && m_shouldHideCursor && SDL_CursorVisible() && std::chrono::system_clock::now() >= m_showCursorExpire) {
        SDL_HideCursor();
    }

    if(m_shouldRenderClay) {
        updateUI();
    }
}

void Application::checkShouldRenderClay() {
    m_shouldRenderClay = m_statusActive || m_settingsActive || m_showFrametime || Clay_IsDebugModeEnabled();
}

void Application::render() {
    SDL_SetRenderDrawColor(m_renderData.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderData.renderer);

    if(m_camera.approved) {
        renderCameraToTexture();
        SDL_RenderTexture(m_renderData.renderer, m_camera.texture, nullptr, &m_camera.displayRect);
    }

    if(m_shouldRenderClay) {
        Clay_RenderCommandArray commands = buildUI();
        SDL_Clay_RenderClayCommands(&m_renderData, &commands);

        checkShouldRenderClay();
    }
#ifdef DEBUG
    else {
        m_frameLimiter.frameTime();
    }

    // frametime display will consume hasStableChanged, so it will only print while that is not showing
    if(m_frameLimiter.hasStableChanged()) {
        SDL_Log("FPS: %d | min: %0.02f | max: %0.02f", static_cast<int>(1000 / m_frameLimiter.stableFrameTimeAverage()), m_frameLimiter.stableFrameTimeMin(), m_frameLimiter.stableFrameTimeMax());
    }
#endif

    m_frameLimiter.limit(true);
    SDL_RenderPresent(m_renderData.renderer);
    m_frameLimiter.limit(false);
}

void Application::changeStatus(const std::string& text, std::chrono::milliseconds timeToExpire) {
    m_status.text   = text;
    m_status.expire = std::chrono::system_clock::now() + timeToExpire;

    m_statusActive = true;
    checkShouldRenderClay();
}

void Application::setFullscreen(bool fullscreen) {
    m_isFullscreen = fullscreen;
    SDL_SetWindowFullscreen(m_window, fullscreen);
}

void Application::setVolume(int volume, bool showStatus, bool save) {
    m_volume     = volume;
    m_volumeText = std::format("{}%", m_volume);

    if(save) {
        m_settings.setVolume(volume);
    }

    updateVolume(showStatus);
}

float Application::getCubicVolume() const {
    auto volume = static_cast<float>(m_volume) / 100.0f;
    return std::powf(volume, 3.0f);
}

void Application::updateVolume(bool showStatus) {
    if(showStatus) {
        changeStatus(std::format("Volume: {}%", m_volume), std::chrono::milliseconds(1500));
    }

#ifdef ENABLE_PIPEWIRE
    if(m_usingPipewire) {
        _updateVolumePipewire();
    }
#else
    // NOLINTNEXTLINE
    if(false) {}
#endif
    else {
        _updateVolumeSDL();
    }
}

bool Application::getShouldQuit() const { return m_shouldQuit; }
void Application::setShouldQuit(bool shouldQuit) { m_shouldQuit = shouldQuit; }