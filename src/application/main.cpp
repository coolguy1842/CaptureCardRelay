#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <application.hpp>
#include <cmath>
#include <damase_ttf.hpp>
#include <settings.hpp>
#include <vector>

#include "clay_renderer_SDL3.hpp"

#define CLAY_IMPLEMENTATION
#include <clay.h>

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

Application::Application()
    : m_shouldQuit(false)
    , m_width(800)
    , m_height(600)
    , m_cameraData(new CustomElementData{
          .type   = CUSTOM_ELEMENT_TYPE_CAMERA,
          .camera = { nullptr, nullptr }
}) {
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CAMERA)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
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
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        setShouldQuit();

        return;
    }

    if(!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
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
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        setShouldQuit();

        return;
    }

    initCameras();
    initAudioPlaybackDevices();
    initAudioRecordingDevices();
    if(getShouldQuit()) {
        return;
    }

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
    if(SDL_CursorVisible() && std::chrono::system_clock::now() >= m_showCursorExpire) {
        SDL_HideCursor();
    }
}

Uint32 Application::statusStep() {
    if(!m_status.animationReverse) {
        if(m_status.animationProgress < 1.0f) {
            m_status.animationProgress = std::clamp(m_status.animationProgress + 0.01f, 0.0f, 1.0f);
        }

        if(std::chrono::system_clock::now() >= m_status.expire) {
            m_status.animationReverse = true;
        }

        return 1;
    }

    if(m_status.animationProgress > 0.0f) {
        m_status.animationProgress = std::clamp(m_status.animationProgress - 0.01f, 0.0f, 1.0f);

        return 1;
    }

    m_status.text.clear();
    return 0;
}

Uint32 Application::onStatusStepCallback(void* userdata, SDL_TimerID timerID, Uint32 interval) {
    Application* app = reinterpret_cast<Application*>(userdata);
    if(app->statusStep() == 0) {
        app->m_statusStepTimer = 0;
        return 0;
    }

    return 1;
}

void Application::changeStatus(std::string text, std::chrono::milliseconds timeToExpire) {
    m_status.text   = text;
    m_status.expire = std::chrono::system_clock::now() + timeToExpire;

    m_status.animationReverse = false;

    if(m_statusStepTimer == 0) {
        m_statusStepTimer = SDL_AddTimer(1000 / 60, &Application::onStatusStepCallback, this);
    }
}

void Application::updateVolume() {
    // exponential volume function
    float volume         = Settings::get()->getVolume() / 100.0f;
    float adjustedVolume = std::powf(volume, 3.0f);

    SDL_SetAudioStreamGain(m_audioPlayback.stream, adjustedVolume);
}

bool Application::getShouldQuit() const { return m_shouldQuit; }
void Application::setShouldQuit(bool shouldQuit) { m_shouldQuit = shouldQuit; }