#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SonicFlow Player");
    QApplication::setOrganizationName("SonicFlow");
    QApplication::setWindowIcon(QIcon(":/assets/assets/app_icon.png"));

    MainWindow window;
    window.show();
    return app.exec();
}
