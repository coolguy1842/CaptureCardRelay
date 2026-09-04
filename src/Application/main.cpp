#include <application.hpp>
#include <cmath>
#include <cstddef>
#include <damase_ttf.hpp>
#include <format>
#include <settings.hpp>

#define CLAY_IMPLEMENTATION
#include <clay.h>

void HandleClayErrors(Clay_ErrorData errorData) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", errorData.errorText.chars);
}

Application::Application(const char* settingsPath)
    : m_settings(settingsPath)
    , m_width(800)
    , m_height(600)
    , m_frameLimiter(true) {
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CAMERA)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialize SDL: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    if((m_window = SDL_CreateWindow("Capture Card Relay", m_width, m_height, SDL_WINDOW_RESIZABLE)) == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create window: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    setFullscreen(m_settings.isFullscreen());
    if((m_renderData.renderer = SDL_CreateGPURenderer(NULL, m_window)) != NULL) {
        SDL_GPUDevice* gpu = SDL_GetGPURendererDevice(m_renderData.renderer);
        if(gpu == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Created renderer with GPU device, but GPU somehow null: %s", SDL_GetError());
            setShouldQuit();

            return;
        }

        SDL_PropertiesID props = SDL_GetGPUDeviceProperties(gpu);
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
        if((m_renderData.renderer = SDL_CreateRenderer(m_window, NULL)) == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create renderer: %s", SDL_GetError());
            setShouldQuit();

            return;
        }

        SDL_Log("Created renderer.");
    }

    if(!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialise SDL_ttf: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    if((m_renderData.textEngine = TTF_CreateRendererTextEngine(m_renderData.renderer)) == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine from renderer: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    m_renderData.fonts.push_back(TTF_OpenFontIO(SDL_IOFromConstMem(damase_v2, sizeof(damase_v2)), true, 40.0f));
    if(m_renderData.fonts.back() == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't open font: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    m_volume     = m_settings.getVolume();
    m_volumeText = std::format("{}%", m_volume);

    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_AUDIO_DEVICE_ADDED, SDL_EVENT_AUDIO_DEVICE_ADDED);
    SDL_FlushEvents(SDL_EVENT_CAMERA_DEVICE_ADDED, SDL_EVENT_CAMERA_DEVICE_ADDED);

    openCamera();
    openAudioPlaybackDevice();
    openAudioRecordingDevice();
    if(getShouldQuit()) {
        return;
    }

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory    = Clay_Arena{
           .capacity = totalMemorySize,
           .memory   = (char*)malloc(totalMemorySize)
    };

    Clay_Initialize(clayMemory, Clay_Dimensions(m_width, m_height), Clay_ErrorHandler(HandleClayErrors));
    Clay_SetMeasureTextFunction(SDL_MeasureText, &m_renderData.fonts);

    m_settings.displayModeChanged.connect<&Application::updateCameraDisplayMode>(this);
    m_settings.scaleModeChanged.connect<&Application::updateCameraScaleMode>(this);
    m_settings.pixelFormatChanged.connect<&Application::updateCameraPixelFormat>(this);
    m_settings.frameLimitInfoChanged.connect<&Application::updateFrameLimiter>(this);

    updateFrameLimiter(m_settings.getFrameLimitInfo());

    m_pointerCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    m_defaultCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
}

Application::~Application() {
    setShouldQuit(true);

    closeCamera();
    closeAudioRecordingDevice();
    closeAudioPlaybackDevice();

    SDL_DestroyCursor(m_pointerCursor);
    SDL_DestroyCursor(m_defaultCursor);

    for(auto font : m_renderData.fonts) {
        if(font == nullptr) {
            continue;
        }

        TTF_CloseFont(font);
    }

    TTF_DestroyRendererTextEngine(m_renderData.textEngine);

    SDL_DestroyRenderer(m_renderData.renderer);
    SDL_DestroyWindow(m_window);

    SDL_Quit();
}

void Application::updateCameraDisplayMode(CameraDisplayMode) { updateCameraDisplayRect(); }
void Application::updateCameraScaleMode(SDL_ScaleMode) { updateCameraTexture(); }
void Application::updateCameraPixelFormat(PixelFormat) { updateCameraTexture(); }

void Application::updateFrameLimiter(FrameLimitInfo info) {
    m_frameLimitInfo = info;
    m_fpsText        = std::format("{} FPS", info.fps);

    switch(info.type) {
    case FRAME_LIMIT_CAMERA:
        if(m_camera.device == nullptr) {
            // set to vsync as fallback
            m_frameLimiter.setFPSLimit(0);
            SDL_SetRenderVSync(m_renderData.renderer, 1);

            break;
        }

        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(m_camera.spec.framerate_numerator / static_cast<float>(m_camera.spec.framerate_denominator));

        break;
    case FRAME_LIMIT_VSYNC:
        m_frameLimiter.setFPSLimit(0);
        SDL_SetRenderVSync(m_renderData.renderer, 1);

        break;
    case FRAME_LIMIT_VSYNC_ADAPTIVE:
        m_frameLimiter.setFPSLimit(0);
        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

        break;
    case FRAME_LIMIT_FPS:
        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(info.fps);

        break;
    case FRAME_LIMIT_NONE:
    default:
        SDL_SetRenderVSync(m_renderData.renderer, SDL_RENDERER_VSYNC_DISABLED);
        m_frameLimiter.setFPSLimit(0.0f);

        break;
    }
}

bool Application::loop() {
    if(getShouldQuit()) {
        return false;
    }

    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        handleEvent(&event);
    }

    update();
    render();

    return !getShouldQuit();
}

void Application::update() {
    if(m_shouldHideCursor && SDL_CursorVisible() && std::chrono::system_clock::now() >= m_showCursorExpire) {
        SDL_HideCursor();
    }

    updateUI();
}

void Application::render() {
    SDL_SetRenderDrawColor(m_renderData.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderData.renderer);

    if(m_camera.approved) {
        renderCameraToTexture();
        SDL_RenderTexture(m_renderData.renderer, m_camera.texture, NULL, &m_camera.displayRect);
    }

    Clay_RenderCommandArray commands = buildUI();
    SDL_Clay_RenderClayCommands(&m_renderData, &commands);

    m_frameLimiter.limit(true);
    SDL_RenderPresent(m_renderData.renderer);
    m_frameLimiter.limit(false);
}

void Application::changeStatus(std::string text, std::chrono::milliseconds timeToExpire) {
    m_status.text   = text;
    m_status.expire = std::chrono::system_clock::now() + timeToExpire;
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

void Application::updateVolume(bool showStatus) {
    if(showStatus) {
        changeStatus(std::format("Volume: {}%", m_volume), std::chrono::milliseconds(1500));
    }

    if(m_audioPlayback.stream == nullptr) {
        return;
    }

    // exponential volume function
    float volume         = m_volume / 100.0f;
    float adjustedVolume = std::powf(volume, 3.0f);

    SDL_SetAudioStreamGain(m_audioPlayback.stream, adjustedVolume);
}

bool Application::getShouldQuit() const { return m_shouldQuit; }
void Application::setShouldQuit(bool shouldQuit) { m_shouldQuit = shouldQuit; }