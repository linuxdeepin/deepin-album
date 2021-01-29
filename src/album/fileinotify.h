#ifndef FILEINOTIFY_H
#define FILEINOTIFY_H

#include <QThread>
#include <QObject>
#include <QMutex>
#include <QMap>
#include <QTimer>

class FileInotify : public QThread
{
    Q_OBJECT
public:
    explicit FileInotify(QObject *parent = nullptr);
    ~FileInotify() override;

    bool isVaild();
    //添加和删除监控
    void addWather(const QString &path);
    void removeWatcher(const QString &path);

    void clear();
    //获取监控目录所有照片
    void getAllPicture();
    //文件数量改变
//    void fileNumChange(); //预留，暂未使用
    //启动时加载一次
    void pathLoadOnce();

public slots:
    //发送插入
    void onNeedSendPictures();

protected:
    void run() override;

private:
    int  m_handleId = -1;
    int m_wd = -1;
    bool m_running = false;
    QMutex m_mutex;
    QMap<QString, int> watchedDirId;
    QStringList m_allPic;       //目前所有照片
    QStringList m_newFile;      //当前新添加的
    QString m_currentDir;       //给定的当前监控路径
    QStringList  m_Supported;   //支持的格式
    QTimer * m_timer;
};

#endif // FILEINOTIFY_H
