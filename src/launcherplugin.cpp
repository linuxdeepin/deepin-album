/*
 * Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QDebug>
#include <DApplication>

#include "src/filecontrol.h"
#include "src/thumbnailload.h"
#include "src/albumControl.h"
#include "launcherplugin.h"

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

LauncherPlugin::LauncherPlugin(QObject *parent)
    : QObject(parent)
{

}

LauncherPlugin::~LauncherPlugin()
{

}

int LauncherPlugin::main(QGuiApplication *app, QQmlApplicationEngine *engine)
{
    // 请在此处注册需要导入到QML中的C++类型
    // 例如： engine->rootContext()->setContextProperty("Utils", new Utils);
    //后端缩略图加载
    LoadImage *load = new LoadImage();
    engine->rootContext()->setContextProperty("CodeImage", load);
    engine->addImageProvider(QLatin1String("ThumbnailImage"), load->m_pThumbnail);
    engine->addImageProvider(QLatin1String("viewImage"), load->m_viewLoad);
    engine->addImageProvider(QLatin1String("publisher"), load->m_publisher);
    engine->addImageProvider(QLatin1String("collectionPublisher"), load->m_collectionPublisher);
    engine->rootContext()->setContextProperty("publisher", load->m_publisher);

    FileControl *fileControl = new FileControl();
    engine->rootContext()->setContextProperty("fileControl", fileControl);

    //新增相册控制模块
    AlbumControl *albumControl = new AlbumControl();
    engine->rootContext()->setContextProperty("albumControl", albumControl);

    //设置为相册模式
    fileControl->setViewerType(imageViewerSpace::ImgViewerTypeAlbum);
//    if (!fileControl->isCheckOnly()) {
//        return 0;
//    }
    engine->load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine->rootObjects().isEmpty())
        return -1;

    app->setWindowIcon(QIcon::fromTheme("deepin-album"));
    app->setApplicationDisplayName(QObject::tr("Album"));
    app->setApplicationName("deepin-album");
    app->setApplicationDisplayName(QObject::tr("Album"));

    return app->exec();
}

QGuiApplication *LauncherPlugin::createApplication(int &argc, char **argv)
{
    // 与 DAppLoader 的关系为：该函数会由 DAppLoader::createApplication() 调用，
    // 然后返回值作为 DAppLoader::exec() 的一个参数，接着 DAppLoader::exec() 会调用
    // LauncherPlugin::main()，最终启动整个程序显示界面。
    // 重写此接口的目的：
    // 1.可以使用自己创建的 QGuiApplication 对象；
    // 2.可以在创建 QGuiApplication 之前为程序设置一些属性（如使用
    //   QCoreApplication::setAttribute 禁用屏幕缩放）；
    // 3.可以添加一些在 QGuiApplication 构造过程中才需要的环境变量；

    // TODO: 无 XDG_CURRENT_DESKTOP 变量时，将不会加载 deepin platformtheme 插件，会导致
    // 查找图标的接口无法调用 qt5integration 提供的插件，后续应当把图标查找相关的功能移到 dtkgui
    if (qEnvironmentVariableIsEmpty("XDG_CURRENT_DESKTOP")) {
        qputenv("XDG_CURRENT_DESKTOP", "Deepin");
    }
    qputenv("D_POPUP_MODE", "embed");
    // 注意:请不要管理 QGuiApplication 对象的生命周期！
    DApplication *a = new DApplication(argc, argv);
    a->loadTranslator();
    a->setApplicationLicense("GPLV3");
    a->setApplicationVersion("1.0.0");
    a->setOrganizationName("deepin");
    a->setApplicationName("deepin-album");
    a->setApplicationDisplayName(QObject::tr("Album"));
    a->setProductIcon(QIcon::fromTheme("deepin-album"));
    a->setApplicationDescription(tr("Main", "Album is a fashion manager for viewing and organizing photos and videos."));
    a->loadTranslator();

    return a;
}
