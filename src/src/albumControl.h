#ifndef AlbumControl_H
#define AlbumControl_H

#include <QObject>
#include <QUrl>
#include "unionimage/unionimage.h"


class AlbumControl : public QObject
{
    Q_OBJECT
public:
    explicit AlbumControl(QObject *parent = nullptr);
    ~  AlbumControl();

    //获得全部导入的DBImgInfoList
    Q_INVOKABLE void getAllInfos();

    //获得全部路径
    Q_INVOKABLE QStringList getAllPaths();

    //导入图片
    Q_INVOKABLE void importAllImagesAndVideos(const QList< QUrl >& paths);

public slots:

private :

    DBImgInfoList m_infoList;
};

#endif // AlbumControl_H
