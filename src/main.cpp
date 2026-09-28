#ifndef PROJECT_VERSION
// fallback if not defined
#define PROJECT_VERSION "1.0.0"
#endif

#include <application.hpp>
#include <clay_renderer_SDL3.hpp>
#include <cstring>
#include <memory>
#include <settings.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
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

const char* settingsPath = nullptr;
std::unique_ptr<Application> app;

void loop() {
    if(app == nullptr) {
        app = std::make_unique<Application>(settingsPath);
    }

#ifdef __EMSCRIPTEN__
    if(!app->loop()) {
        app.reset();

        throw std::runtime_error("user quit");
    }
#else
    while(app->loop()) {}
#endif

    app.reset();
}

int main(int argc, char** argv) {
    SDL_Log("Version %s", PROJECT_VERSION);

    switch(argc) {
    case 1: break;
    case 2:
        if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            usage(argc, argv);
            return 0;
        }

        settingsPath = argv[1];
        break;
    default:
        usage(argc, argv);
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_run_script("alert('This is a demo for the program. Audio (very bad), and video have limited quality on browsers. Expect worse quality, press O for options. Additionally WebGPU is not supported, frame rates will be very low.')");
    emscripten_set_main_loop(loop, 0, true);
#else
    loop();
#endif

    SDL_Clay_Exit();

    return 0;
}