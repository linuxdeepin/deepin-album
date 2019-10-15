#include "application.h"
#include "mainwindow.h"
#include <DMainWindow>
#include <DWidgetUtil>
#include <DApplicationSettings>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    Application::loadDXcbPlugin();
    Application a(argc, argv);

    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    a.setAttribute(Qt::AA_EnableHighDpiScaling);
    a.setAttribute(Qt::AA_ForceRasterWidgets);
    a.setOrganizationName("deepin");
    a.setApplicationName("deepin-album");

    qputenv("DTK_USE_SEMAPHORE_SINGLEINSTANCE", "1");
    if(!a.setSingleInstance(a.applicationName(), DApplication::UserScope))
    {
        exit(0);
    }

    //save theme
    DApplicationSettings savetheme;

    MainWindow w;
    w.show();

    Dtk::Widget::moveToCenter(&w);
    return a.exec();
}
