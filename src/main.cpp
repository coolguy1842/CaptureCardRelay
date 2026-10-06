#ifndef CAPTURECARDRELAY_VERSION
// fallback if not defined
#define CAPTURECARDRELAY_VERSION "1.0.0"
#endif

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <application.hpp>
#include <clay_renderer_SDL3.hpp>
#include <cstring>
#include <memory>
#include <settings.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void usage(int argc, char** argv) {
    const char* programName = "CaptureCardRelay";
    if(argc > 0) {
        programName = argv[0];
    }

    SDL_Log("Usage: %s [FILE]", programName);
    SDL_Log("\n");
    SDL_Log("With no FILE get options from default config file.");
    SDL_Log("  -h, --help  display this help and exit");
}

const char* settingsPath         = nullptr;
std::unique_ptr<Application> app = nullptr;

SDL_AppResult SDL_AppInit(void**, int argc, char** argv) {
    SDL_Log("Version %s", CAPTURECARDRELAY_VERSION);

    switch(argc) {
    case 1: break;
    case 2:
        if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            usage(argc, argv);
            return SDL_APP_SUCCESS;
        }

        settingsPath = argv[1];
        break;
    default:
        usage(argc, argv);
        return SDL_APP_FAILURE;
    }

    app = std::make_unique<Application>(settingsPath);
    if(app->getShouldQuit()) {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult) {
    // destroy text before, call after just to make sure
    SDL_Clay_Exit();
    app.reset();
    SDL_Clay_Exit();
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event) { return app->handleEvent(event); }
SDL_AppResult SDL_AppIterate(void*) { return app->loop(); }