# 定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)

# 设置工程名字
project(deepin-album-test)

# 设置cmake参数
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 添加gocv覆盖率文件的输出
set(CMAKE_CXX_FLAGS "-g -fprofile-arcs -ftest-coverage")

#if (USE_GCOV)
    set(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
    set(GCC_COVERAGE_LINK_FLAGS "-lgcov")
#endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# 设置Qt模块
set(QtModule Core Gui Widgets Network DBus Sql PrintSupport Svg Concurrent LinguistTools X11Extras OpenGL)

# 查找Qt相关的库链接
find_package(Qt5 REQUIRED ${QtModule})

# 包含源码文件夹
include_directories(../album)
include_directories(../album/albumview)
include_directories(../album/allpicview)
include_directories(../album/controller)
include_directories(../album/dbmanager)
include_directories(../album/dialogs)
include_directories(../album/frame)
include_directories(../album/imageengine)
include_directories(../album/importtimelineview)
include_directories(../album/importview)
include_directories(../album/module)
include_directories(../album/module/slideshow)
include_directories(../album/module/view)
include_directories(../album/module/view/scen)
include_directories(../album/module/view/contents)
include_directories(../album/searchview)
include_directories(../album/thumbnail)
include_directories(../album/timelineview)
include_directories(../album/utils)
include_directories(../album/widgets)
include_directories(../album/widgets/dialogs)
include_directories(/usr/include)
include_directories(/usr/include/gio-qt)
include_directories(/usr/include/udisks2-qt5)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtCore/5.11.3)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtCore/5.11.3/QtCore)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtCore/5.11.3/QtCore/private)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtWidgets/5.11.3)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtWidgets/5.11.3/QtWidgets)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtWidgets/5.11.3/QtWidgets/private)
#include_directories(/usr/include/x86_64-linux-gnu/qt5/QtCore/5.11.3)
#include_directories(/usr/include/x86_64-linux-gnu/qt5/QtCore/5.11.3/QtCore)
#include_directories(/usr/include/x86_64-linux-gnu/qt5/QtCore/5.11.3/QtCore/private)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtGui/5.11.3)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtGui/5.11.3/QtGui)
include_directories(/usr/include/x86_64-linux-gnu/qt5/QtGui/5.11.3/QtGui/private)

include_directories(googletest)
include_directories(googletest/googlemock/include)
include_directories(googletest/googletest/include)

add_subdirectory(googletest)

# 定义 source 文件
FILE (GLOB allSource
    "../album/application.cpp" "../album/dtktest.cpp""../album/mainwindow.cpp" "../album/*.c"
    "../album/albumview/*.cpp"  "../album/albumview/*.c"
    "../album/allpicview/*.cpp"  "../album/allpicview/*.c"
    "../album/controller/*.cpp"  "../album/controller/*.c"
    "../album/dbmanager/*.cpp"  "../album/dbmanager/*.c"
    "../album/dialogs/*.cpp"  "../album/dialogs/*.c"
    "../album/frame/*.cpp"  "../album/frame/*.c"
    "../album/imageengine/*.cpp"  "../album/imageengine/*.c"
    "../album/importtimelineview/*.cpp"  "../album/importtimelineview/*.c"
    "../album/importview/*.cpp"  "../album/importview/*.c"
    "../album/module/*.cpp"  "../album/module/*.c"
    "../album/module/slideshow/*.cpp" "../album/module/slideshow/*.c"
    "../album/module/view/*.cpp"  "../album/module/view/*.c"
    "../album/module/view/scen/*.cpp"  "../album/module/view/scen/*.c"
    "../album/module/view/contents/*.cpp"  "../album/module/view/contents/*.c"
    "../album/searchview/*.cpp"  "../album/searchview/*.c"
    "../album/thumbnail/*.cpp"  "../album/thumbnail/*.c"
    "../album/timelineview/*.cpp" "../album/timelineview/*.c"
    "../album/utils/*.cpp"  "../album/utils/*.c"
    "../album/widgets/*.cpp"  "../album/widgets/*.c"
    "../album/widgets/dialogs/*.cpp"  "../album/widgets/dialogs/*.c"
    )
FILE (GLOB allTestSource
    "./*.cpp" "./*.c")

# 定义 header 文件
FILE (GLOB allHeaders
    "../album/*.hpp"  "../album/*.h"
    "../album/albumview/*.hpp"  "../album/albumview/*.h"
    "../album/allpicview/*.hpp"  "../album/allpicview/*.h"
    "../album/controller/*.hpp"  "../album/controller/*.h"
    "../album/dbmanager/*.hpp"  "../album/dbmanager/*.h"
    "../album/dialogs/*.hpp"  "../album/dialogs/*.h"
    "../album/frame/*.hpp"  "../album/frame/*.h"
    "../album/imageengine/*.hpp"  "../album/imageengine/*.h"
    "../album/importtimelineview/*.hpp"  "../album/importtimelineview/*.h"
    "../album/importview/*.hpp"  "../album/importview/*.h"
    "../album/module/*.hpp"  "../album/module/*.h"
    "../album/module/slideshow/*.hpp" "../album/module/slideshow/*.h"
    "../album/module/view/*.hpp"  "../album/module/view/*.h"
    "../album/module/view/scen/*.hpp"  "../album/module/view/scen/*.h"
    "../album/module/view/contents/*.hpp"  "../album/module/view/contents/*.h"
    "../album/searchview/*.hpp"  "../album/searchview/*.h"
    "../album/thumbnail/*.hpp"  "../album/thumbnail/*.h"
    "../album/timelineview/*.hpp" "../album/timelineview/*.h"
    "../album/utils/*.hpp"  "../album/utils/*.h"
    "../album/widgets/*.hpp"  "../album/widgets/*.h"
    "../album/widgets/dialogs/*.hpp"  "../album/widgets/dialogs/*.h"
    )

# 定义资源文件
set(AllQRC
    ../album/resources.qrc
    ../icons/theme-icons.qrc
    ../album/frame/frametheme.qrc
    #album/frame/frame.qrc
    ../album/module/view/viewtheme.qrc
    #album/widgets/widgets.qrc
    ../album/widgets/widgetstheme.qrc)

# 设置不删除生成的文件夹内容文件（设置此项是为了翻译后的文件不被删除）
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

# 定义翻译文件
set(TargetTsFiles
    translations/deepin-album_zh_CN.ts
    translations/deepin-album_en_US.ts
    translations/deepin-album.ts
    )

set(LINK_DIR ../lib)
link_directories(${LINK_DIR})
link_libraries(UnionImage)

# 添加需要安装的模块
add_executable(${PROJECT_NAME} ${allHeaders} ${allSource} ${allTestSource} ${AllQRC}

#    ${gmock} ${gmock_main} ${gtest} ${gtest_main}
#${gtest} ${gmock}
    )
add_compile_definitions(LITE_DIV)

#------------------------------ 添加第三方库  DTK -------------------------------
# 使用第三方库需要用到的一个包
find_package(PkgConfig REQUIRED)

# 检查第三方库(这里检查了名字为dtkwidget的库和名字为dtkgui的库)，然后取名3rd_lib
pkg_check_modules(3rd_lib REQUIRED
        dtkwidget dtkgui gio-qt udisks2-qt5
        )
# 添加第三方库的所有文件夹路径到工程中来(注意 *_INCLUDE_DIRS)
target_include_directories(${PROJECT_NAME} PUBLIC ${3rd_lib_INCLUDE_DIRS})

# 将第三方库链接进来(注意 *_LIBRARIES)
target_link_libraries(${PROJECT_NAME} ${3rd_lib_LIBRARIES} )
#------------------------------添加第三方库end-------------------------------------

target_link_libraries(${PROJECT_NAME} gmock gmock_main gtest gtest_main)

# 将工程与Qt模块链接起来
qt5_use_modules(${PROJECT_NAME} ${QtModule})

# ------------------------------- install script -------------------------------

#set(PREFIX /usr)
#set(BinDir ${PREFIX}/bin)
#set(TranslationDir ${PREFIX}/share/deepin-draw/translations/)
#set(DbusDir ${PREFIX}/share/dbus-1/services/)
#set(DesktopDir ${PREFIX}/share/applications/)
#set(FormatFileDir ${PREFIX}/share/mime/packages/)
#set(ManIconDir ${PREFIX}/share/icons/hicolor/scalable/apps)
#set(AppIconDir ${PREFIX}/share/icons/hicolor/scalable/apps)
#set(MimeFileDir ${PREFIX}/share/mime/packages)

# install Targets:
#   * <prefix>/lib/libxxx.so
#   * header location after install: <prefix>/include/xxx.h
#   * headers can be included by C++ code `#include <xxx.h>`
#install(TARGETS ${PROJECT_NAME}
#        EXPORT ${PROJECT_NAME}Targets
#        LIBRARY DESTINATION lib
#        ARCHIVE DESTINATION lib
#        RUNTIME DESTINATION /usr/bin
#        INCLUDES DESTINATION allHeaders)

# install Translation files:
#install(FILES ${LangSrcs} DESTINATION ${TranslationDir})

# install DBus file:
#install(FILES com.deepin.Draw.service DESTINATION ${DbusDir})

# install Desktop icon
#install(FILES deepin-draw.desktop DESTINATION ${DesktopDir})

# install FormatFile ---> unused
#install(FILES src/service/x-ddf.xml DESTINATION ${FormatFileDir})

# install Manual icon
#install(FILES src/logo/deepin-draw.svg DESTINATION ${ManIconDir})

# install App icon
#install(FILES src/logo/deepin-draw.svg DESTINATION ${AppIconDir})

# install MimeFile
#install(FILES mimetype/deepin-draw.xml DESTINATION ${MimeFileDir})
