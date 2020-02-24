#include "application.h"
#include "mainwindow.h"
#include "dtktest.h"
#include "imageengine/imageengineapi.h"
#include <DMainWindow>
#include <DWidgetUtil>
#include <DApplicationSettings>
#include <DLog>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

QUrl UrlInfo(QString path)
{
    QUrl url;
    // Just check if the path is an existing file.
    if (QFile::exists(path)) {
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
        return url;
    }

    const auto match = QRegularExpression(QStringLiteral(":(\\d+)(?::(\\d+))?:?$")).match(path);

    if (match.isValid()) {
        // cut away line/column specification from the path.
        path.chop(match.capturedLength());
    }

    // make relative paths absolute using the current working directory
    // prefer local file, if in doubt!
    url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);

    // in some cases, this will fail, e.g.
    // assume a local file and just convert it to an url.
    if (!url.isValid()) {
        // create absolute file path, we will e.g. pass this over dbus to other processes
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
    }
    return url;
}

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

    ImageEngineApi::instance(&a);
    MainWindow w;

    QCommandLineParser parser;
    parser.process(a);

    QStringList urls;
    QStringList arguments = parser.positionalArguments();

    QString filepath = "";
    bool bneedexit = true;
    for (const QString &path : arguments) {
        filepath = UrlInfo(path).toLocalFile();


        QFileInfo info(filepath);
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
        qDebug() << info.filePath() << "&&&&&&&&&&&&&&" << "mt" << mt.name() << "mt1" << mt1.name();

        QString str = info.suffix().toLower();
        if (str.isEmpty()) {
            if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
                if (utils::image::supportedImageFormats().contains("*." + str, Qt::CaseInsensitive)) {
                    bneedexit = false;
                    break;
                } else if (str.isEmpty()) {
                    bneedexit = false;
                    break;
                }
            }
        } else {
            if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
                if (utils::image::supportedImageFormats().contains("*." + str, Qt::CaseInsensitive)) {
                    bneedexit = false;
                    break;
                }
            }
        }
//        if (!filepath.endsWith("jpg") &&
//                !filepath.endsWith("jpeg") &&
//                !filepath.endsWith("bmp") &&
//                !filepath.endsWith("png") &&
//                !filepath.endsWith("ppm") &&
//                !filepath.endsWith("xbm") &&
//                !filepath.endsWith("xpm") &&
//                !filepath.endsWith("gif")) {
//            exit(0);
//        }
    }

    if ("" != filepath && bneedexit) {
        exit(0);
    }

    if (!DGuiApplicationHelper::instance()->setSingleInstance(a.applicationName(), DGuiApplicationHelper::UserScope)) {
        exit(0);
    }


    if (!bneedexit) {
        if (bfirstopen) {
            bfirstandviewimage = true;
        }
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

//    DtkTest w;
//    w.resize(1300, 848);
    w.show();
    Dtk::Widget::moveToCenter(&w);

    if (bneedexit)
        bfirstopen = false;
    return a.exec();
}
