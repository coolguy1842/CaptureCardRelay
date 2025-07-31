#include <application.hpp>
#include <cmath>
#include <damase_ttf.hpp>
#include <settings.hpp>

Application::Application()
    : m_shouldQuit(false)
    , m_fontInitialized(false)
    , m_width(800)
    , m_height(600) {
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CAMERA)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    if(!SDL_CreateWindowAndRenderer(
           "Capture Card Relay",
           m_width,
           m_height,
           SDL_WINDOW_RESIZABLE,
           &m_window,
           &m_renderer
       )) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return;
    }

    if(TTF_Init()) {
        m_font = TTF_OpenFontIO(SDL_IOFromConstMem(damase_v2, sizeof(damase_v2)), true, 40.0f);
        if(m_font != nullptr) {
            m_fontInitialized = true;
        }
        else {
            SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        }
    }
    else {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
    }

    initCameras();
    initAudioPlaybackDevices();
    initAudioRecordingDevices();
    if(getShouldQuit()) return;

    openCamera();
    openAudioPlaybackDevice();
    openAudioRecordingDevice();
    if(getShouldQuit()) return;
}

Application::~Application() {
    setShouldQuit(true);

    closeCamera();
    closeAudioRecordingDevice();
    closeAudioPlaybackDevice();

    if(m_font != nullptr) {
        TTF_CloseFont(m_font);
    }

    SDL_DestroyRenderer(m_renderer);
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
    if(m_status.expire <= std::chrono::system_clock::now() && m_status.texture != nullptr) {
        SDL_DestroyTexture(m_status.texture);
    }
}

void Application::render() {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderer);

    if(m_camera.device != nullptr && m_camera.approved) {
        SDL_Surface* frame = SDL_AcquireCameraFrame(m_camera.device, nullptr);
        if(frame != nullptr) {
            SDL_UpdateTexture(m_camera.texture, nullptr, frame->pixels, frame->pitch);
            SDL_ReleaseCameraFrame(m_camera.device, frame);
        }

        float windowWidth = m_width, windowHeight = m_height;
        float cameraWidth = m_camera.spec.width, cameraHeight = m_camera.spec.height;

        float windowAspect = windowWidth / windowHeight;
        float cameraAspect = cameraWidth / cameraHeight;

        SDL_FRect destRect = { 0, 0, windowWidth, windowHeight };
        if(cameraAspect > windowAspect) {
            destRect.h = windowWidth / cameraAspect;
            destRect.y = (windowHeight - destRect.h) / 2.0f;
        }
        else {
            destRect.w = windowHeight * cameraAspect;
            destRect.x = (windowWidth - destRect.w) / 2.0f;
        }

        if(m_camera.texture != nullptr) {
            SDL_RenderTexture(m_renderer, m_camera.texture, nullptr, &destRect);
        }

        if(m_status.texture != nullptr) {
            float textWidth = m_status.texture->w, textHeight = m_status.texture->h;

            SDL_FRect rect = { (destRect.x + destRect.w) - textWidth, (destRect.y + destRect.h) - textHeight, textWidth, textHeight };
            SDL_RenderTexture(m_renderer, m_status.texture, nullptr, &rect);
        }
    }

    SDL_RenderPresent(m_renderer);
}

void Application::changeStatus(std::string text, std::chrono::milliseconds timeToExpire) {
    if(!m_fontInitialized) {
        return;
    }

    if(m_status.texture != nullptr) {
        SDL_DestroyTexture(m_status.texture);
    }

    m_status.text   = text;
    m_status.expire = std::chrono::system_clock::now() + timeToExpire;

    SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), text.size(), SDL_Color{ 255, 255, 255, 255 });
    if(surface != nullptr) {
        m_status.texture = SDL_CreateTextureFromSurface(m_renderer, surface);

        SDL_DestroySurface(surface);
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