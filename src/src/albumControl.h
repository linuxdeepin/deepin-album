#ifndef AlbumControl_H
#define AlbumControl_H

#include <QObject>
#include <QUrl>
#include "unionimage/unionimage.h"


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

    //获得日月年所有创建时间线  0所有 1年 2月 3日
    Q_INVOKABLE QStringList getTimelinesTitle(TimeLineEnum timeEnum);

    //获得全部已经导入
    Q_INVOKABLE QStringList getAllImportTimelinesTitle();

    //获得某一导入时间的全部路径
    Q_INVOKABLE QStringList getImportTimelinesTitlePaths(const QString &titleName);

    //获得图片和视频总数
    Q_INVOKABLE int getCount();

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

};

#endif // AlbumControl_H
