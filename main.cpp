#include "application.h"
#include "mainwindow.h"
#include <DMainWindow>
#include <DWidgetUtil>
#include <DApplicationSettings>
#include <DLog>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    Application::loadDXcbPlugin();
    Application a(argc, argv);

    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    //  a.setAttribute(Qt::AA_EnableHighDpiScaling);
    //a.setAttribute(Qt::AA_ForceRasterWidgets);
    a.setOrganizationName("deepin");
    a.setApplicationName("deepin-album");

    qputenv("DTK_USE_SEMAPHORE_SINGLEINSTANCE", "1");
    if (!DGuiApplicationHelper::instance()->setSingleInstance(a.applicationName(), DGuiApplicationHelper::UserScope)) {
        exit(0);
    }

    //save theme
    DApplicationSettings savetheme;


    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();


//    QImage* pimg = new QImage();

//    if(argc > 1)
//    {
//        for(int i = 0; i < argc - 1; i++)
//        {
//            if(!pimg->load(argv[i + 1]))
//            {
//                exit(0);
//            }
//        }
//    }

    MainWindow w;
    w.resize(1300, 848);
    w.show();

    Dtk::Widget::moveToCenter(&w);
    return a.exec();
}
