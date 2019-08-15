#include "mainwindow.h"
#include <DApplication>
#include <DMainWindow>
#include <DWidgetUtil>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication::loadDXcbPlugin();
    QApplication a(argc, argv);
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    a.setAttribute(Qt::AA_EnableHighDpiScaling);
    a.setAttribute(Qt::AA_ForceRasterWidgets);

    MainWindow w;
    w.show();

    Dtk::Widget::moveToCenter(&w);
    return a.exec();
}
