#include <Config.hpp>
#include <MainWindow.hpp>
#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    Config::close();
    return app.exec();
}