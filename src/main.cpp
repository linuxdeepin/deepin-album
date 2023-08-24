// Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "src/filecontrol.h"
#include "src/thumbnailload.h"
#include "src/cursortool.h"
#include "src/albumControl.h"

#include "src/imageengine/imagedataservice.h"
#include "thumbnailview/positioner.h"
#include "thumbnailview/rubberband.h"
#include "thumbnailview/mouseeventlistener.h"
#include "thumbnailview/eventgenerator.h"

#include "thumbnailview/roles.h"
#include "thumbnailview/imagedatamodel.h"
#include "thumbnailview/thumbnailmodel.h"
#include "types.h"
#include "thumbnailview/qimageitem.h"

#include "declarative/mousetrackitem.h"
#include "globalcontrol.h"
#include "globalstatus.h"
#include "imagedata/imageinfo.h"
#include "imagedata/imagesourcemodel.h"
#include "imagedata/imageprovider.h"

#include <dapplicationhelper.h>
#include <DApplication>

#include "config.h"

#include <DLog>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QScopedPointer>
#include <QQmlContext>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

// 此文件是QML应用的启动文件，一般无需修改
// 请在LauncherPlugin::main()中实现所需功能
int main(int argc, char *argv[])
{
    // TODO: 无 XDG_CURRENT_DESKTOP 变量时，将不会加载 deepin platformtheme 插件，会导致
    // 查找图标的接口无法调用 qt5integration 提供的插件，后续应当把图标查找相关的功能移到 dtkgui
    if (qEnvironmentVariableIsEmpty("XDG_CURRENT_DESKTOP")) {
        qputenv("XDG_CURRENT_DESKTOP", "Deepin");
    }

    qputenv("D_POPUP_MODE", "embed");
    // 注意:请不要管理 QGuiApplication 对象的生命周期！
    DApplication *app = new DApplication(argc, argv);
    app->loadTranslator();
    app->setApplicationLicense("GPLV3");
    app->setApplicationVersion(DApplication::buildVersion(VERSION));
    app->setAttribute(Qt::AA_UseHighDpiPixmaps);
    app->setOrganizationName("deepin");
    app->setApplicationName("deepin-album");
    app->setApplicationDisplayName(QObject::tr("Album"));
    app->setProductIcon(QIcon::fromTheme("deepin-album"));
    //app->setApplicationDescription(QObject::tr("Main", "Album is a fashion manager for viewing and organizing photos and videos."));
    app->setWindowIcon(QIcon::fromTheme("deepin-album"));

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    QQmlApplicationEngine engine;

    if (!DGuiApplicationHelper::instance()->setSingleInstance(app->applicationName(), DGuiApplicationHelper::UserScope)) {
        exit(0);
    }

    // 配置文件加载
    LibConfigSetter::instance()->loadConfig(imageViewerSpace::ImgViewerTypeAlbum);

    // 请在此处注册需要导入到QML中的C++类型
    // 例如： engine.rootContext()->setContextProperty("Utils", new Utils);
    //后端缩略图加载
    LoadImage *load = new LoadImage();
    engine.rootContext()->setContextProperty("CodeImage", load);
    engine.addImageProvider(QLatin1String("ThumbnailImage"), load->m_pThumbnail);
    engine.addImageProvider(QLatin1String("viewImage"), load->m_viewLoad);
    // 后端多页图加载
    engine.addImageProvider(QLatin1String("multiimage"), load->m_multiLoad);

    engine.addImageProvider(QLatin1String("publisher"), load->m_publisher);
    engine.addImageProvider(QLatin1String("collectionPublisher"), load->m_collectionPublisher);
    engine.addImageProvider(QLatin1String("asynImageProvider"), load->m_asynImageProvider);

    engine.rootContext()->setContextProperty("asynImageProvider", load->m_asynImageProvider);
    engine.rootContext()->setContextProperty("publisher", load->m_publisher);

    engine.rootContext()->setContextProperty("imageDataService", ImageDataService::instance());

    QImageItem::initDamage();

    // QML全局单例
    GlobalControl control;
    engine.rootContext()->setContextProperty("GControl", &control);
    GlobalStatus status;
    engine.rootContext()->setContextProperty("GStatus", &status);

    FileControl fileControl;
    engine.rootContext()->setContextProperty("fileControl", &fileControl);

    // 光标位置查询工具
    CursorTool *cursorTool = new CursorTool();
    engine.rootContext()->setContextProperty("cursorTool", cursorTool);

    // 后端缩略图加载，由 QMLEngine 管理生命周期
    // 部分平台支持线程数较低时，使用同步加载
    ProviderCache *providerCache = nullptr;
    if (!GlobalControl::enableMultiThread()) {
        ImageProvider *imageProvider = new ImageProvider;
        engine.addImageProvider(QLatin1String("ImageLoad"), imageProvider);

        providerCache = static_cast<ProviderCache *>(imageProvider);
    } else {
        AsyncImageProvider *asyncImageProvider = new AsyncImageProvider;
        engine.addImageProvider(QLatin1String("ImageLoad"), asyncImageProvider);

        providerCache = static_cast<ProviderCache *>(asyncImageProvider);
    }

    ThumbnailProvider *multiImageLoad = new ThumbnailProvider;
    engine.addImageProvider(QLatin1String("ThumbnailLoad"), multiImageLoad);

    // 关联各组件
    // 提交图片旋转信息到文件，覆写文件
    QObject::connect(
        &control, &GlobalControl::requestRotateImage, &fileControl, &FileControl::rotateImageFile, Qt::DirectConnection);
    // 图片旋转时更新图像缓存
    QObject::connect(&control, &GlobalControl::requestRotateCacheImage, [&]() {
        providerCache->rotateImageCached(control.currentRotation(), control.currentSource().toLocalFile());
    });

    status.setEnableNavigation(fileControl.isEnableNavigation());
    QObject::connect(
        &status, &GlobalStatus::enableNavigationChanged, [&]() { fileControl.setEnableNavigation(status.enableNavigation()); });
    QObject::connect(&fileControl, &FileControl::imageRenamed, &control, &GlobalControl::renameImage);
    // 文件变更时清理缓存
    QObject::connect(&fileControl, &FileControl::imageFileChanged, [&](const QString &fileName) {
        providerCache->removeImageCache(fileName);
    });

    //设置为相册模式
    fileControl.setViewerType(imageViewerSpace::ImgViewerTypeAlbum);

    //禁止多开
    if (!fileControl.isCheckOnly()) {
        return 0;
    }

    //新增相册控制模块
    engine.rootContext()->setContextProperty("albumControl", AlbumControl::instance());

    char albumUri[] = "org.deepin.album";
    char imageViewerUri[] = "org.deepin.image.viewer";
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    qmlRegisterType<QAbstractItemModel>();
#else
    qmlRegisterAnonymousType<QAbstractItemModel>(albumUri, 0);
#endif
    // 相册qml插件
    qmlRegisterType<ImageDataModel>(albumUri, 1, 0, "ImageDataModel");
    qmlRegisterType<ThumbnailModel>(albumUri, 1, 0, "ThumbnailModel");
    qmlRegisterType<Positioner>(albumUri, 1, 0, "Positioner");
    qmlRegisterType<RubberBand>(albumUri, 1, 0, "RubberBand");
    qmlRegisterType<MouseEventListener>(albumUri, 1, 0, "MouseEventListener");
    qmlRegisterType<EventGenerator>(albumUri, 1, 0, "EventGenerator");
    qmlRegisterUncreatableType<Types>(albumUri, 1, 0, "Types", "Cannot instantiate the Types class");
    qmlRegisterUncreatableType<Roles>(albumUri, 1, 0, "Roles", "Cannot instantiate the Roles class");
    qmlRegisterType<QImageItem>(albumUri, 1, 0, "QImageItem");

    // 看图qml插件
    qmlRegisterType<ImageInfo>(imageViewerUri, 1, 0, "ImageInfo");
    qmlRegisterType<ImageSourceModel>(imageViewerUri, 1, 0, "ImageSourceModel");
    qmlRegisterType<MouseTrackItem>(imageViewerUri, 1, 0, "MouseTrackItem");
    qmlRegisterUncreatableType<Types>(imageViewerUri, 1, 0, "Types", "Types only use for define");

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app->exec();
}
