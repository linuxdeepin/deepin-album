/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TTBCONTENT_H
#define TTBCONTENT_H

#include <QWidget>
#include <QLabel>
#include "controller/viewerthememanager.h"
//#include <dlistwidget.h>
//#include <DListWidget>
#include <DListWidget>
#include <DSpinner>
//#include <DtkWidgets>
//#include "dlistwidget.h"
#include <QListWidget>
#include <DListView>
#include <QAbstractItemModel>
#include <QStandardItem>
#include "dbmanager/dbmanager.h"
#include <DAnchors>
#include <dimagebutton.h>
#include <DThumbnailProvider>
#include <QPropertyAnimation>
#include <QHBoxLayout>
#include <DIconButton>
#include <DBlurEffectWidget>
#include <DGuiApplicationHelper>
#include <DLabel>
#include "imageengine/imageengineobject.h"

DWIDGET_USE_NAMESPACE

class ElidedLabel;
class QAbstractItemModel;
//class DImageButton;
class ImageButton;
class MyImageListWidget;
class ImageItem;

class MyImageListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MyImageListWidget(QWidget *parent = nullptr);
//    bool ifMouseLeftPressed();
    QObject *getObj();
    void setObj(QObject *obj);
    void setSelectItem(ImageItem *selectItem);
    void animationStart(bool isReset, int endPos, int duration);
    void stopAnimation();
//    bool isAnimationStart();//判断动画是否执行中

    QTimer *m_timer = nullptr;
    void findSelectItem();
protected:
    bool eventFilter(QObject *obj, QEvent *e) Q_DECL_OVERRIDE;
signals:
    void mouseLeftReleased();
    void needContinueRequest();
    void silmoved();

    void testloadRight();
    void testloadLeft();
public slots:
//    void animationTimerTimeOut();
    void animationFinished();
    void thumbnailIsMoving();
    void animationValueChanged(const QVariant value);
private:
    bool bmouseleftpressed = false;
    QObject *m_obj = nullptr;
    QVector<QPoint> m_movePoints;
    bool m_iRet = true;
    QPoint m_prepoint;//鼠标move时鼠标位置
    QPoint m_presspoint;//按下时鼠标位置
    bool m_moveToRight = true;//缩略图中鼠标向右移动为true
    ImageItem *m_selectItem = nullptr;
    int m_preSelectItemIndex = -1;
    QPropertyAnimation *m_resetAnimation = nullptr;//复位动画
    bool m_isMoving = false;//是否正在移动中
    QTimer *m_animationTimer = nullptr;//动画，移动使用
    int m_animationTimerTOCount = 0;
    int m_preListGeometryLeft = 0;
    bool m_resetFinish = false;
};

class ImageItem : public QLabel
{
    Q_OBJECT
public:
//    ImageItem(int index = 0, QString path = "", QString imageType = "", QWidget *parent = 0);
    ImageItem(int index = 0, ImageDataSt data = ImageDataSt(), QWidget *parent = nullptr);
    void setIndexNow(int i);
    void setPic(QPixmap pixmap);

    QString _path;
    int index() const;
    void setIndex(int index);
    bool index_1(int index);
    void emitClickSig();

    int indexNow() const;

signals:
    void imageItemclicked(int index, int indexNow);

protected slots:
    void updateDmgIconByTheme();
protected:
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void paintEvent(QPaintEvent *event) override;
public:
    bool bmouserelease = false;
private:
    int _index;
    int _indexNow = -1;
    DLabel *_image = nullptr;
    QPixmap _pixmap;
    DSpinner *m_spinner;
    QString m_pixmapstring;
    //bool bmouserelease = false;
    bool m_bPicNotSuppOrDamaged = false;
    QTimer *m_timer;
};
class TTBContent : public QLabel, public ImageEngineObject
{
    Q_OBJECT
public:
    struct TTBContentData {
        int index;
        ImageDataSt data;
    };
//    explicit TTBContent(bool inDB, DBImgInfoList m_infos, QWidget *parent = 0);
    explicit TTBContent(bool inDB, QStringList filelist, QWidget *parent = nullptr);
    ~TTBContent() override
    {
        clearAndStopThread();
    }

    //------------------
//    void importFilesFromLocal(QStringList files);
//    void importFilesFromLocal(DBImgInfoList files);
//    void importFilesFromDB(QString name = "");
    bool imageLocalLoaded(QStringList &filelist) override
    {
        Q_UNUSED(filelist)
        return false;
    }
    bool imageFromDBLoaded(QStringList &filelist) override
    {
        Q_UNUSED(filelist)
        return false;
    }
    bool imageLoaded(QString filepath) override;
    void insertImageItem(const ImageDataSt &file, bool bloadRight = true);
    void stopLoadAndClear();
//    void reLoad();
    QStringList getAllFileList();
    bool setCurrentItem();
    void updateScreen();
//    void updateScreenNoAnimation();
    int itemLoadedSize();
    QString getIndexPath(int index);
    void requestSomeImages();
    //------------------
    /**
     * @brief setRightlist  设置右侧数据
     * @param rightlist
     */
    void setRightlist(const QStringList &rightlist);

    void setLeftlist(const QStringList &leftlist);

signals:
    void ttbcontentClicked();
    void imageClicked(int index, int addIndex);
    void resetTransform(bool fitWindow);
    void rotateClockwise();
    void rotateCounterClockwise();

    void removed();
    void imageEmpty(bool v);
    void contentWidthChanged(int width);
    void showPrevious();
    void showNext();
    void feedBackCurrentIndex(int index, QString path);
    void sigRequestSomeImages();
    /**
     * @brief sigloadRight
     * @param rightlist
     */
    void sigloadRight(QStringList rightlist);

    void sigloadLeft(QStringList leftlist);
    //平板需求，退出时重置ttb显隐
    void resetShoworHide();

public slots:
    void setCurrentDir(const QString &text);
    void setImage(const QString &path);
    void updateCollectButton();

    void onResize();
    void disCheckAdaptImageBtn();
    void disCheckAdaptScreenBtn();
    void checkAdaptImageBtn();
    void checkAdaptScreenBtn();
    void deleteImage();
    void onNextButton();
    void onPreButton();
    void updateFilenameLayout();
    void onBackButtonClicked();
    void onAdaptImageBtnClicked();
    void onAdaptScreenBtnClicked();
    void onclBTClicked();
    void onRotateLBtnClicked();
    void onRotateRBtnClicked();
    void onImgListViewTestloadRight();
    void onImgListViewTestloadLeft();
    void onHideImageView();
    void onSilmoved();
    void onNeedContinueRequest();
    void onTrashBtnClicked();

private slots:
    void onThemeChanged(ViewerThemeManager::AppTheme theme);

protected:
    void resizeEvent(QResizeEvent *event) override;
public:
    QString m_imageType;

private:
#ifndef LITE_DIV
    PushButton *m_folderBtn;
    ReturnButton *m_returnBtn;
#endif
    bool m_inDB;
    bool m_bClBTChecked;

    DIconButton *m_adaptImageBtn;
    DIconButton *m_adaptScreenBtn;
    DIconButton *m_clBT;
    DIconButton *m_rotateLBtn;
    DIconButton *m_rotateRBtn;
    DIconButton *m_trashBtn;
    DIconButton *m_preButton;
    DIconButton *m_nextButton;
    DIconButton *m_backButton;
    ElidedLabel *m_fileNameLabel;
    DWidget *m_imgList;
    QHBoxLayout *m_imglayout;
    MyImageListWidget *m_imgListView;
    DWidget *m_preButton_spc;
    DWidget *m_nextButton_spc;
    int m_windowWidth;
    int m_contentWidth;
    int m_nowIndex = -1;
    int m_filelist_size = 0;
    bool bfilefind = false;
    bool bresized = true;
    bool badaptImageBtnChecked = false;
    bool badaptScreenBtnChecked = false;
    //------------------
    QStringList m_allfileslist;
    QStringList m_filesbeleft;
    bool bneedloadimage = true;
    bool brequestallfiles = false;
    QMap<QString, TTBContentData> m_ItemLoaded;
    int m_requestCount = 0;
    int m_allNeedRequestFilesCount = 0;
    QString m_currentpath = "";
    int m_lastIndex = -1;
    bool binsertneedupdate = true;

    QStringList m_rightlist;        //保存动态加载数据（右侧）
    QStringList m_leftlist;
};

#endif // TTLCONTENT_H
