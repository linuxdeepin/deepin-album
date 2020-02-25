#include "imageengineobject.h"
#include "imageengineapi.h"

ImageEngineThreadObject::ImageEngineThreadObject()
{

}
ImageMountImportPathsObject::ImageMountImportPathsObject()
{
    ImageEngineApi::instance()->insertObject(this);
}
ImageMountImportPathsObject::~ImageMountImportPathsObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}
ImageMountGetPathsObject::ImageMountGetPathsObject()
{
    ImageEngineApi::instance()->insertObject(this);
}
ImageMountGetPathsObject::~ImageMountGetPathsObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}
ImageEngineImportObject::ImageEngineImportObject()
{
    ImageEngineApi::instance()->insertObject(this);

}
ImageEngineImportObject::~ImageEngineImportObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}
ImageEngineObject::ImageEngineObject()
{
    ImageEngineApi::instance()->insertObject(this);
    m_threads.clear();
}

ImageEngineObject::~ImageEngineObject()
{
    ImageEngineApi::instance()->removeObject(this);
    clearAndStopThread();
}
