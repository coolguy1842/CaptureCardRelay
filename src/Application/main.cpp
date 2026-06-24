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
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialize SDL: %s\n", SDL_GetError());
        setShouldQuit();

        return;
    }

    if((m_window = SDL_CreateWindow("Capture Card Relay", m_width, m_height, SDL_WINDOW_RESIZABLE)) == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create window: %s\n", SDL_GetError());
        setShouldQuit();

        return;
    }

    if((m_renderData.renderer = SDL_CreateGPURenderer(NULL, m_window)) == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create renderer: %s\n", SDL_GetError());
        setShouldQuit();

        return;
    }

    SDL_GPUDevice* gpu = SDL_GetGPURendererDevice(m_renderData.renderer);
    if(gpu == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Created renderer with GPU device, but GPU somehow null: %s\n", SDL_GetError());
        setShouldQuit();

        return;
    }

    SDL_PropertiesID props = SDL_GetGPUDeviceProperties(gpu);
    if(props != 0) {
        const char* name = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, "");
        SDL_Log("Created renderer with GPU: %s\n\n", name);
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get GPU name: %s\n\n", SDL_GetError());
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

    SDL_DestroyCursor(m_pointerCursor);
    SDL_DestroyCursor(m_defaultCursor);

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

    updateUI();
}

void Application::render() {
    SDL_SetRenderDrawColor(m_renderData.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderData.renderer);

    Clay_RenderCommandArray commands = buildUI();
    SDL_Clay_RenderClayCommands(&m_renderData, &commands);

    SDL_RenderPresent(m_renderData.renderer);
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