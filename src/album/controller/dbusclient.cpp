#include "dbusclient.h"




dbusclient::dbusclient(QObject *parent)
    : QDBusAbstractInterface(staticInterfaceService(), staticInterfacePath(),
                             staticInterfaceName(), QDBusConnection::sessionBus(), parent)
{
//    connect(dApp->signalM,&SignalManager::sigDrawingBoard,this,&dbusclient::openDrawingBoard);
}

dbusclient::~dbusclient()
{

}

//void dbusclient::openDrawingBoard(QStringList paths)
//{
//    QList<QString> list=paths;

//    openFiles(list);
//}

