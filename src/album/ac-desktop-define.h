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

#ifndef DESKTOP_ACCESSIBLE_UI_DEFINE_H
#define DESKTOP_ACCESSIBLE_UI_DEFINE_H

#include <QString>
#include <QObject>

// 使用宏定义，方便国际化操作
#define OPENACCESSIBLE

//平板宏定义
//#define tablet_PC

//#define SELECT_CASE "load"

#ifdef SELECT_CASE
#define TEST_CASE_NAME(testName) if(testName != SELECT_CASE) return;
#else
#define TEST_CASE_NAME(testName)
#endif

#ifdef ENABLE_ACCESSIBILITY
#define AC_SET_ACCESSIBLE_NAME(classObj,accessiblename) classObj->setAccessibleName(accessiblename);
#else
#define AC_SET_ACCESSIBLE_NAME(classObj,accessiblename)
#endif

#define AC_SET_OBJECT_NAME(classObj,objectname) classObj->setObjectName(objectname);

#define Import_Image_View          "ImportImageView"         //导入图片控件
#define Import_Image_View_Button   "ImportImageViewButton"   //导入图片按钮
#define Import_Image_Menu          "ImportImageMenu"         //导入图片菜单
#define Main_All_Picture_Button    "MainAllPictureButton"    //主界面所有图片按钮
#define MainWindow_Center_Widget   "MainWindowCenterWidget"
#define MainWindow_TitleBtn_Widget "MainWindowTitleBtnWidget"
#define MainWindow_Titlebar        "MainWindowTitlebar"

#define All_Picture_View           "AllPictureView"          //所有图片界面
#define All_Picture_StackedWidget  "AllPictureStackedWidget"

#define All_Picture_Thembnail      "AllPictureThembnail"     //所有图片缩略图list
#define Main_Time_Line_Button      "MainTimeLineButton"      //主界面时间线按钮
#define Main_Album_Button          "MainAlbumButton"         //主界面相册按钮
#define Thumbnail_Slider           "ThumbnailSlider"         //缩略图缩放滑动条
#define All_Pic_Count              "AllPicCount"             //图片数量lable

#define Time_Line_Choose_Button    "TimeLineChooseButton"    //时间线选择按钮

#define Add_Album_Button           "AddAlbumButton"          //添加相册按钮
#define Import_Time_Line_Choose_Button  "ImportTimeLineChooseButton"  //导入到时间线选择按钮
#define Album_Delete_Button        "AlbumDeleteButton"       //相册界面删除按钮
#define Album_Restore_Button       "AlbumRestoreButton"      //相册界面恢复按钮
#define Album_Import_Label         "AlbumImportLabel"
#define Album_Trash_Label          "AlbumTrashLabel"
#define Album_Fav_Label            "AlbumFavLabel"

#define TOP_TOOL_BAR               "top tool bar"            //顶部工具栏
#define TITLE_TEXT                 "title text"              //顶部标题栏
#define TITLE_BAR                  "title bar"               //顶部菜单

#define VIEW_PANEL_WIDGET          "viewpanel"               //图片显示区域
#define VIEW_PANEL_STACK           "viewpanel stack"         //图片显示区域堆栈窗口

#define Slider_Play_Pause_Button   "SliderPlayPauseButton"   //幻灯片播放暂停按钮
#define Slider_Pre_Button          "SliderPreButton"         //幻灯片上一张按钮
#define Slider_Next_Button         "SliderNextButton"        //幻灯片下一张按钮
#define Slider_Exit_Button         "SliderExitButton"        //幻灯片退出按钮

#define Ttbcontent_Back_Button     "TtbcontentBackButton"    //ttb返回按钮
#define Ttbcontent_Pre_Button      "TtbcontentPreButton"     //ttb上一张按钮
#define Ttbcontent_Next_Button     "TtbcontentNextButton"    //ttb下一张按钮
#define Ttbcontent_AdaptImg_Button  "TtbcontentAdaptImgButton"  //ttb适应图片按钮
#define Ttbcontent_AdaptScreen_Button  "TtbcontentAdaptScreenButton"  //ttb适应屏幕按钮
#define Ttbcontent_Collect_Button  "TtbcontentCollectButton"  //ttb收藏按钮
#define Ttbcontent_Rotate_Right_Button  "TtbcontentRotateRightButton"  //ttb顺时针按钮
#define Ttbcontent_Rotate_Left_Button   "TtbcontentRotateLeftButton"   //ttb逆时针按钮
#define Ttbcontent_Trash_Button    "TtbcontentTrashButton"    //ttb删除按钮

// helin
#define ImportTime_pChose          "importTime_pChose"
#define ImportTime_pThumbnailListView  "importTime_pThumbnailListView"
#define AlbumView_pImpTimeLineWidget   "albumView_pImpTimeLineWidget"

#endif // DESKTOP_ACCESSIBLE_UI_DEFINE_H
