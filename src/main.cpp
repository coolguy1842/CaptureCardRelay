#include <application.hpp>
#include <cstring>
#include <memory>
#include <settings.hpp>

void usage(int argc, char** argv) {
    SDL_Log("Usage: %s [FILE]\n\nWith no FILE get options from default config file.\n  -h, --help  display this help and exit", argv[0]);
}

int main(int argc, char** argv) {
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

    return 0;
}
