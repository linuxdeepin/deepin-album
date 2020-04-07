#include "dbusclient.h"




dbusclient::dbusclient(QObject *parent)
    : QDBusAbstractInterface(staticInterfaceService(), staticInterfacePath(),
                             staticInterfaceName(), QDBusConnection::sessionBus(), parent)
{

   // QDBusConnection::sessionBus().connect(staticInterfaceService(),staticInterfacePath(),staticInterfaceName(),"com.deepin.Draw",  "com.deepin.Draw",this,SLOT(propertyChanged(QDBusMessage)));

    connect(dApp->signalM,&SignalManager::sigDrawingBoard,this,&dbusclient::openDrawingBoard);
}

dbusclient::~dbusclient()
{

}

void dbusclient::propertyChanged(const QDBusMessage &msg)
{

}


void dbusclient::openDrawingBoard(QStringList paths)
{
    QList<QString> list=paths;

    openFiles(list);
}

