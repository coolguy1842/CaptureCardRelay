#ifndef CAPTURECARDRELAY_VERSION
// fallback if not defined
#define CAPTURECARDRELAY_VERSION "1.0.0"
#endif

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <application.hpp>
#include <clay_renderer_SDL3.hpp>
#include <settings.hpp>
#include <span>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {
std::unique_ptr<Application> app = nullptr;

void usage(std::span<char*> args) {
    const char* programName = "CaptureCardRelay";
    if(!args.empty()) {
        programName = args.front();
    }

    SDL_Log("Usage: %s [FILE]", programName);
    SDL_Log("\n");
    SDL_Log("With no FILE get options from default config file.");
    SDL_Log("  -h, --help  display this help and exit");
}

}; // namespace

SDL_AppResult SDL_AppInit(void** /*userData*/, int argc, char** argv) {
    const std::span<char*> args(argv, static_cast<size_t>(argc));
    SDL_Log("Version %s", CAPTURECARDRELAY_VERSION);

    std::optional<std::string> settingsPath;
    switch(argc) {
    case 1: break;
    case 2:
        settingsPath = args[1];
        if(settingsPath != "-h" && settingsPath != "--help") {
            usage(args);
            return SDL_APP_SUCCESS;
        }

        break;
    default:
        usage(args);
        return SDL_APP_FAILURE;
    }

    app = std::make_unique<Application>(settingsPath);
    if(app->getShouldQuit()) {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* /*userData*/, SDL_AppResult /*result*/) {
    // destroy text before, call after just to make sure
    SDL_Clay_Exit();
    app.reset();
    SDL_Clay_Exit();
}

SDL_AppResult SDL_AppEvent(void* /*userData*/, SDL_Event* event) { return app->handleEvent(event); }
SDL_AppResult SDL_AppIterate(void* /*userData*/) { return app->loop(); }