// Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "src/filecontrol.h"
#include "src/thumbnailload.h"
#include "src/cursortool.h"
#include "src/albumControl.h"

#include <dapplicationhelper.h>
#include <DApplication>

#include "config.h"

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
    app->setOrganizationName("deepin");
    app->setApplicationName("deepin-album");
    app->setApplicationDisplayName(QObject::tr("Album"));
    app->setProductIcon(QIcon::fromTheme("deepin-album"));
    //app->setApplicationDescription(QObject::tr("Main", "Album is a fashion manager for viewing and organizing photos and videos."));
    app->setWindowIcon(QIcon::fromTheme("deepin-album"));

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

    FileControl *fileControl = new FileControl();
    engine.rootContext()->setContextProperty("fileControl", fileControl);
    // 关联文件处理（需要保证优先处理，onImageFileChanged已做多线程安全）
    QObject::connect(fileControl, &FileControl::requestImageFileChanged,
    load, [&](const QString & filePath, bool isMultiImage, bool isExist) {
        // 更新缓存信息
        load->onImageFileChanged(filePath, isMultiImage, isExist);
        // 处理完成后加载图片
        emit fileControl->imageFileChanged(filePath, isMultiImage, isExist);
    });

    // 光标位置查询工具
    CursorTool *cursorTool = new CursorTool();
    engine.rootContext()->setContextProperty("cursorTool", cursorTool);

    //禁止多开
    if (!fileControl->isCheckOnly()) {
        return 0;
    }

    //新增相册控制模块
    engine.rootContext()->setContextProperty("albumControl", AlbumControl::instance());

    //设置为相册模式
    fileControl->setViewerType(imageViewerSpace::ImgViewerTypeAlbum);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app->exec();
}
