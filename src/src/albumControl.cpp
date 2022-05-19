#include "albumControl.h"
#include "dbmanager/dbmanager.h"

AlbumControl::AlbumControl(QObject *parent) : QObject(parent)
{

}

AlbumControl::~AlbumControl()
{

}

void AlbumControl::getAllInfos()
{
    m_infoList = DBManager::instance()->getAllInfos();
}

QStringList AlbumControl::getAllPaths()
{
    QStringList pathList;
    if(m_infoList.count() <= 0){
        getAllInfos();
    }
    for(DBImgInfo info : m_infoList){
        pathList << "file://"+ info.filePath;
    }
    return pathList;
}
