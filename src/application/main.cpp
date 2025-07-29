#include <application.hpp>
#include <chrono>
#include <cstdlib>
#include <damase_ttf.hpp>

#include "SDL3/SDL_render.h"

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
    closeCamera();

    closeAudioRecordingDevice();
    closeAudioPlaybackDevice();

    if(m_font != nullptr) {
        TTF_CloseFont(m_font);
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
    if(m_status.expire <= std::chrono::system_clock::now() && m_status.texture != nullptr) {
        SDL_DestroyTexture(m_status.texture);
    }

    int dataAvailable, dataLength;
    if(
        m_audioPlayback.stream == nullptr ||
        m_audioRecording.stream == nullptr ||
        (dataAvailable = SDL_GetAudioStreamAvailable(m_audioRecording.stream)) <= 0
    ) {
        return;
    }

    if((dataLength = SDL_GetAudioStreamData(m_audioRecording.stream, m_audioPlayback.buffer, std::min(m_audioPlayback.bufferSize, dataAvailable))) != -1) {
        SDL_PutAudioStreamData(m_audioPlayback.stream, m_audioPlayback.buffer, dataLength);
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

        if(m_camera.texture != nullptr) {
            int widthBorder  = std::abs(m_width - m_camera.spec.width) / 2;
            int heightBorder = std::abs(m_height - m_camera.spec.height) / 2;
            SDL_FRect rect{ (float)widthBorder, (float)heightBorder, (float)m_camera.texture->w, (float)m_camera.texture->h };

            SDL_RenderTexture(m_renderer, m_camera.texture, nullptr, &rect);
        }
    }

    if(m_camera.device != nullptr && m_camera.approved && m_status.texture != nullptr) {
        int widthBorder  = std::abs(m_width - m_camera.spec.width) / 2;
        int heightBorder = std::abs(m_height - m_camera.spec.height) / 2;

        SDL_FRect rect{ (float)m_width - (widthBorder + m_status.texture->w), (float)m_height - (heightBorder + m_status.texture->h), (float)m_status.texture->w, (float)m_status.texture->h };
        SDL_RenderTexture(m_renderer, m_status.texture, nullptr, &rect);
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

bool Application::getShouldQuit() const { return m_shouldQuit; }
void Application::setShouldQuit(bool shouldQuit) { m_shouldQuit = shouldQuit; }