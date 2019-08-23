#include "application.h"
#include "mainwindow.h"
#include <DMainWindow>
#include <DWidgetUtil>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    Application::loadDXcbPlugin();
    Application a(argc, argv);

    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    a.setAttribute(Qt::AA_EnableHighDpiScaling);
    a.setAttribute(Qt::AA_ForceRasterWidgets);

    MainWindow w;
    w.show();

    Dtk::Widget::moveToCenter(&w);
    return a.exec();
}
