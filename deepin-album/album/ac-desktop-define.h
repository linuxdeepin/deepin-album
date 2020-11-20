/**
 * Copyright (C) 2020 UOS Technology Co., Ltd.
 *
 * to mark the desktop UI
 **/

#ifndef DESKTOP_ACCESSIBLE_UI_DEFINE_H
#define DESKTOP_ACCESSIBLE_UI_DEFINE_H

#include <QString>
#include <QObject>

// 使用宏定义，方便国际化操作
#define OPENACCESSIBLE

#ifdef ENABLE_ACCESSIBILITY
#define AC_SET_ACCESSIBLE_NAME(classObj,accessiblename) classObj->setAccessibleName(accessiblename);
#else
#define AC_SET_ACCESSIBLE_NAME(classObj,accessiblename)
#endif

#define AC_SET_OBJECT_NAME(classObj,objectname) classObj->setObjectName(objectname);

#define Import_Image_View QObject::tr("ImportImageView") //导入图片控件
#define Import_Image_View_Button QObject::tr("ImportImageViewButton") //导入图片按钮
#define Import_Image_Menu QObject::tr("ImportImageMenu") //导入图片菜单
#define Main_All_Picture_Button QObject::tr("MainAllPictureButton") //主界面所有图片按钮
#define MainWindow_Center_Widget QObject::tr("MainWindowCenterWidget")
#define MainWindow_TitleBtn_Widget QObject::tr("MainWindowTitleBtnWidget")
#define MainWindow_Titlebar QObject::tr("MainWindowTitlebar")

#define All_Picture_View QObject::tr("AllPictureView") //所有图片界面
#define All_Picture_StackedWidget QObject::tr("AllPictureStackedWidget")

#define All_Picture_Thembnail QObject::tr("AllPictureThembnail") //所有图片缩略图list
#define Main_Time_Line_Button QObject::tr("MainTimeLineButton") //主界面时间线按钮
#define Main_Album_Button QObject::tr("MainAlbumButton") //主界面相册按钮
#define Thumbnail_Slider QObject::tr("ThumbnailSlider") //缩略图缩放滑动条
#define All_Pic_Count QObject::tr("AllPicCount") //图片数量lable

#define Time_Line_Choose_Button QObject::tr("TimeLineChooseButton") //时间线选择按钮

#define Add_Album_Button QObject::tr("AddAlbumButton") //添加相册按钮
#define Import_Time_Line_Choose_Button QObject::tr("ImportTimeLineChooseButton") //导入到时间线选择按钮
#define Album_Delete_Button QObject::tr("AlbumDeleteButton") //相册界面删除按钮
#define Album_Restore_Button QObject::tr("AlbumRestoreButton") //相册界面恢复按钮

#define Slider_Play_Pause_Button QObject::tr("SliderPlayPauseButton") //幻灯片播放暂停按钮
#define Slider_Pre_Button QObject::tr("SliderPreButton") //幻灯片上一张按钮
#define Slider_Next_Button QObject::tr("SliderNextButton") //幻灯片下一张按钮
#define Slider_Exit_Button QObject::tr("SliderExitButton") //幻灯片退出按钮

#define Ttbcontent_Back_Button QObject::tr("TtbcontentBackButton") //ttb返回按钮
#define Ttbcontent_Pre_Button QObject::tr("TtbcontentPreButton") //ttb上一张按钮
#define Ttbcontent_Next_Button QObject::tr("TtbcontentNextButton") //ttb下一张按钮
#define Ttbcontent_AdaptImg_Button QObject::tr("TtbcontentAdaptImgButton") //ttb适应图片按钮
#define Ttbcontent_AdaptScreen_Button QObject::tr("TtbcontentAdaptScreenButton") //ttb适应屏幕按钮
#define Ttbcontent_Collect_Button QObject::tr("TtbcontentCollectButton") //ttb收藏按钮
#define Ttbcontent_Rotate_Right_Button QObject::tr("TtbcontentRotateRightButton") //ttb顺时针按钮
#define Ttbcontent_Rotate_Left_Button QObject::tr("TtbcontentRotateLeftButton") //ttb逆时针按钮
#define Ttbcontent_Trash_Button QObject::tr("TtbcontentTrashButton") //ttb删除按钮

#endif // DESKTOP_ACCESSIBLE_UI_DEFINE_H
