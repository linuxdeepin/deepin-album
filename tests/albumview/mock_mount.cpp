#include "mock_mount.h"
#include "stub-tool/cpp-stub/stub.h"
#include "stub-tool/cpp-stub/addr_any.h"

#include <dgiomount.h>
#include <dgiofile.h>
#include <dgiovolumemanager.h>

QString Mock_Mount::mountPoint;

Mock_Mount::Mock_Mount(const QString &mountPoint)
{
    this->mountPoint = mountPoint;

    //以下代码顺序不能调换
    stub = new Stub;
    initMountMock();
}

Mock_Mount::~Mock_Mount()
{
    delete stub;
}

void Mock_Mount::initMountMock()
{
    //基础函数替换
    stub->set(&DGioMount::name, &Mock_Mount::stub_DGioMount_name);
    stub->set(&DGioMount::getRootFile, &Mock_Mount::stub_DGioMount_getRootFile);
    stub->set(&DGioMount::getDefaultLocationFile, &Mock_Mount::stub_DGioMount_getDefaultLocationFile);
    stub->set(&DGioMount::canEject, &Mock_Mount::stub_DGioMount_canEject);
    stub->set(&DGioFile::uri, &Mock_Mount::stub_DGioFile_uri);
    stub->set(&DGioFile::path, &Mock_Mount::stub_DGioFile_path);
    stub->set(&DGioVolumeManager::getMounts, &Mock_Mount::stub_DGioVolumeManager_getMounts);
}

void Mock_Mount::setToUmountDevice()
{
    stub->reset(&DGioVolumeManager::getMounts);
}

QString Mock_Mount::stub_DGioMount_name()
{
    return "Journey";
}

QExplicitlySharedDataPointer<DGioFile> Mock_Mount::stub_DGioMount_getRootFile()
{
    return QExplicitlySharedDataPointer<DGioFile>(nullptr);
}

bool Mock_Mount::stub_DGioMount_canEject() const
{
    return true;
}

QExplicitlySharedDataPointer<DGioFile> Mock_Mount::stub_DGioMount_getDefaultLocationFile()
{
    return QExplicitlySharedDataPointer<DGioFile>(nullptr);
}

QString Mock_Mount::stub_DGioFile_uri()
{
    return "file:///" + mountPoint;
}

QString Mock_Mount::stub_DGioFile_path()
{
    return mountPoint;
}

const QList<QExplicitlySharedDataPointer<DGioMount>> Mock_Mount::stub_DGioVolumeManager_getMounts() //这个是静态函数
{
    return QList<QExplicitlySharedDataPointer<DGioMount>> {QExplicitlySharedDataPointer<DGioMount>(nullptr)};
}
