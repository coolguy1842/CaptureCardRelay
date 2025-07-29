#include <application.hpp>
#include <settings.hpp>

int main(int argc, char** argv) {
    Application* app = new Application();
    while(app->loop());

    delete app;
    Settings::close();

    return 0;
}
