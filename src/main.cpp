#ifndef PROJECT_VERSION
// fallback if not defined
#define PROJECT_VERSION "1.0.0"
#endif

#include <application.hpp>
#include <clay_renderer_SDL3.hpp>
#include <cstring>
#include <memory>
#include <settings.hpp>

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

int main(int argc, char** argv) {
    SDL_Log("Version %s", PROJECT_VERSION);

    {
        const char* settingsPath = nullptr;

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

        std::shared_ptr<Application> app = std::make_shared<Application>(settingsPath);
        while(app->loop());
    }

    SDL_Clay_Exit();
    return 0;
}
