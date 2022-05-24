#ifndef AlbumControl_H
#define AlbumControl_H

#include <QObject>
#include <QUrl>
#include "unionimage/unionimage.h"
#include "dbmanager/dbmanager.h"


class AlbumControl : public QObject
{
    Q_OBJECT
public:

    enum TimeLineEnum{
        All  = 0,
        Year = 1,
        Month  = 2,
        Day = 3
    };

    explicit AlbumControl(QObject *parent = nullptr);
    ~  AlbumControl();

    //获得全部导入的DBImgInfoList
    Q_INVOKABLE void getAllInfos();

    //获得全部导入的路径
    Q_INVOKABLE QStringList getAllPaths();

    //导入图片，导入图片接口
    Q_INVOKABLE void importAllImagesAndVideos(const QList< QUrl >& paths);

    //获得全部创建时间线
    Q_INVOKABLE QStringList getAllTimelinesTitle();

    //获得某一创建时间线的全部路径
    Q_INVOKABLE QStringList getTimelinesTitlePaths(const QString &titleName);

    //获得年创建时间线
    Q_INVOKABLE QStringList getYearTimelinesTitle();

    //获得月创建时间线
    Q_INVOKABLE QStringList getMonthTimelinesTitle();

    //获得日创建时间线
    Q_INVOKABLE QStringList getDayTimelinesTitle();

    //获得全部已经导入
    Q_INVOKABLE QStringList getAllImportTimelinesTitle();

    //获得某一导入时间的全部路径
    Q_INVOKABLE QStringList getImportTimelinesTitlePaths(const QString &titleName);

    //获得图片和视频总数
    Q_INVOKABLE int getCount();

    //将文件放进最近删除(添加)
    Q_INVOKABLE void insertTrash(const QList< QUrl > &paths);

    //将文件放进收藏中(添加)
    Q_INVOKABLE void insertCollection(const QList< QUrl > &paths);

    //新建相册
    Q_INVOKABLE void createAlbum(const QString &newName);

    //获得所有的自定义相册id
    Q_INVOKABLE QList < int > getAllCustomAlbumId();

    //获得所有的自定义相册名称
    Q_INVOKABLE QList < QString > getAllCustomAlbumName();

    //根据自定义相册id获取相册名称
    Q_INVOKABLE QString getCustomAlbumByUid(const int &index);

    //判断当前图片是否已收藏
    Q_INVOKABLE bool photoHaveFavorited(const QString &path);

public :
    QString getDeleteFullPath(const QString &hash, const QString &fileName);

    //获得最近删除的文件
    DBImgInfoList getTrashInfos();

    //获得收藏文件
    DBImgInfoList getCollectionInfos();

    //获得画板文件
    DBImgInfoList getDrawInfos();

    //获得截图录屏文件
    DBImgInfoList getScreenCaptureInfos();

    //获得相机文件
    DBImgInfoList getCameraInfos();

    //新相册名称
    const QString getNewAlbumName(const QString &baseName);

    //获得日月年所有创建时间线  0所有 1年 2月 3日
    QStringList getTimelinesTitle(TimeLineEnum timeEnum);

public slots:

private :
    DBImgInfoList m_infoList;  //全部已导入

    //时间线数据
    QList < QDateTime > m_timelines; //所有创建时间线
    QMap < QString, DBImgInfoList > m_timeLinePathsMap;  //每个创建时间线的路径

    //已导入（合集）数据
    QList < QDateTime > m_importTimelines; //所有已导入时间线
    QMap < QString, DBImgInfoList > m_importTimeLinePathsMap;  //每个已导入时间线的路径

    QMap < QString ,DBImgInfoList > m_yearDateMap; //年数据集
    QMap < QString ,DBImgInfoList > m_monthDateMap; //月数据集
    QMap < QString ,DBImgInfoList > m_dayDateMap; //日数据集
    QMap < int ,QString > m_customAlbum; //自定义相册


};

#endif // AlbumControl_H
