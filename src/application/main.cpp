#include <application.hpp>
#include <cmath>
#include <damase_ttf.hpp>
#include <format>
#include <settings.hpp>

#define CLAY_IMPLEMENTATION
#include <clay.h>

void HandleClayErrors(Clay_ErrorData errorData) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", errorData.errorText.chars);
}

Application::Application()
    : m_width(800)
    , m_height(600)
    , m_cameraData(new CustomElementData{
          .type   = CUSTOM_ELEMENT_TYPE_CAMERA,
          .camera = CameraData{
              .displayMode = Settings::get()->getDisplayMode(),
          },
      }) {
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CAMERA)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialize SDL: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    if(!SDL_CreateWindowAndRenderer(
           "Capture Card Relay",
           m_width,
           m_height,
           SDL_WINDOW_RESIZABLE,
           &m_window,
           &m_renderData.renderer
       )) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialize SDL: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    if(!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
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

    m_volume     = Settings::get()->getVolume();
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

    if(Settings::get()->isFullscreen()) {
        SDL_SetWindowFullscreen(m_window, true);
    }

    Settings::get()->displayModeChanged.connect([this](CameraDisplayMode mode) {
        if(m_cameraData == nullptr) {
            return;
        }

        m_cameraData->camera.displayMode = mode;
    });

    m_pointerCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    m_defaultCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
}

Application::~Application() {
    setShouldQuit(true);

    closeCamera();
    closeAudioRecordingDevice();
    closeAudioPlaybackDevice();

    for(auto font : m_renderData.fonts) {
        if(font == nullptr) {
            continue;
        }

        TTF_CloseFont(font);
    }

    SDL_DestroyRenderer(m_renderData.renderer);
    SDL_DestroyWindow(m_window);

    SDL_Quit();
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

    uint32_t buttons = SDL_GetMouseState(&m_cursorX, &m_cursorY);

    m_mouseClicked = buttons & SDL_BUTTON_LMASK && !m_mouseHeld;
    m_mouseHeld    = buttons & SDL_BUTTON_LMASK;

    if(!m_mouseHeld && m_slidingVolume) {
        m_slidingVolume        = false;
        m_volumeSnapped        = false;
        m_volumeInSnapRange    = false;
        m_volumeFreeDuringSnap = false;

        Settings::get()->setVolume(m_volume);
    }

    Clay_SetPointerState(
        Clay_Vector2{ .x = m_cursorX, .y = m_cursorY },
        m_mouseHeld
    );

    static uint64_t prev = SDL_GetPerformanceCounter();
    uint64_t now         = SDL_GetPerformanceCounter();

    float deltaTime = static_cast<double>(((now - prev) * 1000 / static_cast<float>(SDL_GetPerformanceFrequency())));
    prev            = now;

    Clay_UpdateScrollContainers(!m_slidingVolume, Clay_Vector2{ m_mouseWheelX, m_mouseWheelY }, deltaTime);

    m_mouseWheelX = 0.0f;
    m_mouseWheelY = 0.0f;
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
        Settings::get()->setVolume(volume);
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