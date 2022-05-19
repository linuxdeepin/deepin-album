#ifndef AlbumControl_H
#define AlbumControl_H

#include <QObject>

#include "unionimage/unionimage_global.h"


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

public slots:

private :

    DBImgInfoList m_infoList;
};

#endif // AlbumControl_H
