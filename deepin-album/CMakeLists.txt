#定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)

set(CMD_NAME deepin-album)

project(${CMD_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#设置是否执行测试
option(DOTEST "option for test" OFF)

if(DOTEST)
    # 添加gocv覆盖率文件的输出
    set(CMAKE_CXX_FLAGS "-g -fprofile-arcs -ftest-coverage")
    set(QtModule Core Gui Widgets DBus Sql PrintSupport Svg Concurrent LinguistTools Test)
else()
    set(QtModule Core Gui Widgets DBus Sql PrintSupport Svg Concurrent LinguistTools)
endif()
#先查找到这些qt相关的模块以供链接使用
find_package(Qt5 REQUIRED ${QtModule})

#文件夹包含draw_zh_CN
include_directories(album)
include_directories(album/albumview)
include_directories(album/allpicview)
include_directories(album/controller)
include_directories(album/dbmanager)
include_directories(album/dialogs)
include_directories(album/frame)
include_directories(album/imageengine)
include_directories(album/importtimelineview)
include_directories(album/importview)
include_directories(album/module)
include_directories(album/module/slideshow)
include_directories(album/module/view)
include_directories(album/module/view/scen)
include_directories(album/module/view/contents)
include_directories(album/searchview)
include_directories(album/thumbnail)
include_directories(album/timelineview)
include_directories(album/utils)
include_directories(album/widgets)
include_directories(album/widgets/dialogs)
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

aux_source_directory(./album allSource)
aux_source_directory(./album/albumview allSource)
aux_source_directory(./album/allpicview allSource)
aux_source_directory(./album/controller allSource)
aux_source_directory(./album/dbmanager allSource)
aux_source_directory(./album/dialogs allSource)
aux_source_directory(./album/frame allSource)
aux_source_directory(./album/imageengine allSource)
aux_source_directory(./album/importtimelineview allSource)
aux_source_directory(./album/importview allSource)
aux_source_directory(./album/module allSource)
aux_source_directory(./album/module/slideshow allSource)
aux_source_directory(./album/module/view allSource)
aux_source_directory(./album/module/view/scen allSource)
aux_source_directory(./album/module/view/contents allSource)
aux_source_directory(./album/searchview allSource)
aux_source_directory(./album/settings/slideshow allSource)
aux_source_directory(./album/thumbnail allSource)
aux_source_directory(./album/timelineview allSource)
aux_source_directory(./album/utils allSource)
aux_source_directory(./album/widgets allSource)
aux_source_directory(./album/widgets/dialogs allSource)

#如果执行测试
if(DOTEST)
include_directories(tests/googletest)
include_directories(tests/googletest/googlemock/include)
include_directories(tests/googletest/googletest/include)
endif()

FILE (GLOB allHeaders
    "./album/*.hpp"  "./album/*.h"
    "./album/albumview/*.hpp"  "./album/albumview/*.h"
    "./album/allpicview/*.hpp"  "./album/allpicview/*.h"
    "./album/controller/*.hpp"  "./album/controller/*.h"
    "./album/dbmanager/*.hpp"  "./album/dbmanager/*.h"
    "./album/dialogs/*.hpp"  "./album/dialogs/*.h"
    "./album/frame/*.hpp"  "./album/frame/*.h"
    "./album/imageengine/*.hpp"  "./album/imageengine/*.h"
    "./album/importtimelineview/*.hpp"  "./album/importtimelineview/*.h"
    "./album/importview/*.hpp"  "./album/importview/*.h"
    "./album/module/*.hpp"  "./album/module/*.h"
    "./album/module/slideshow/*.hpp" "./album/module/slideshow/*.h"
    "./album/module/view/*.hpp"  "./album/module/view/*.h"
    "./album/module/view/scen/*.hpp"  "./album/module/view/scen/*.h"
    "./album/module/view/contents/*.hpp"  "./album/module/view/contents/*.h"
    "./album/searchview/*.hpp"  "./album/searchview/*.h"
    "./album/thumbnail/*.hpp"  "./album/thumbnail/*.h"
    "./album/timelineview/*.hpp" "./album/timelineview/*.h"
    "./album/utils/*.hpp"  "./album/utils/*.h"
    "./album/widgets/*.hpp"  "./album/widgets/*.h"
    "./album/widgets/dialogs/*.hpp"  "./album/widgets/dialogs/*.h"
    )

FILE (GLOB allSource
    "./album/application.cpp"  "./album/mainwindow.cpp"
    "./album/albumview/*.cpp"  "./album/albumview/*.c"
    "./album/allpicview/*.cpp"  "./album/allpicview/*.c"
    "./album/controller/*.cpp"  "./album/controller/*.c"
    "./album/dbmanager/*.cpp"  "./album/dbmanager/*.c"
    "./album/dialogs/*.cpp"  "./album/dialogs/*.c"
    "./album/frame/*.cpp"  "./album/frame/*.c"
    "./album/imageengine/*.cpp"  "./album/imageengine/*.c"
    "./album/importtimelineview/*.cpp"  "./album/importtimelineview/*.c"
    "./album/importview/*.cpp"  "./album/importview/*.c"
    "./album/module/*.cpp"  "./album/module/*.c"
    "./album/module/slideshow/*.cpp" "./album/module/slideshow/*.c"
    "./album/module/view/*.cpp"  "./album/module/view/*.c"
    "./album/module/view/scen/*.cpp"  "./album/module/view/scen/*.c"
    "./album/module/view/contents/*.cpp"  "./album/module/view/contents/*.c"
    "./album/searchview/*.cpp"  "./album/searchview/*.c"
    "./album/thumbnail/*.cpp"  "./album/thumbnail/*.c"
    "./album/timelineview/*.cpp" "./album/timelineview/*.c"
    "./album/utils/*.cpp"  "./album/utils/*.c"
    "./album/widgets/*.cpp"  "./album/widgets/*.c"
    "./album/widgets/dialogs/*.cpp"  "./album/widgets/dialogs/*.c"
    )

set(AllQRC
    album/resources.qrc
    icons/theme-icons.qrc
    album/frame/frametheme.qrc
    album/module/view/viewtheme.qrc
    album/widgets/widgetstheme.qrc)
file(GLOB TargetTsFiles LIST_DIRECTORIES false ../translations/${CMD_NAME}*.ts)
#set(TargetTsFiles
#    translations/deepin-album_zh_HK.ts
#    translations/deepin-album_zh_TW.ts
#    translations/deepin-album_zh_CN.ts
#    translations/deepin-album.ts
#    )



# 设置不删除生成的文件夹内容文件
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

qt5_create_translation(LangSrcs ${allSource} ${allHeaders} ${TargetTsFiles}
    OPTIONS )
#set_source_files_properties(${TargetTsFiles} PROPERTIES OUTPUT_LOCATION "translations")
qt5_add_translation(${TargetTsFiles})

set_source_files_properties(${qmfiles} PROPERTIES OUTPUT_LOCATION "translations")

set(LINK_DIR lib)
link_directories(${LINK_DIR})
#link_libraries(UnionImage)
#target_include_directories(${PROJECT_NAME} PUBLIC ${LINK_DIR})


add_executable(${PROJECT_NAME} ${allSource} ${AllQRC} ${allHeaders} ${LangSrcs} "./album/main.cpp")
add_compile_definitions(LITE_DIV)

#------------------------------ 配置test ---------------------------------------

if(DOTEST)
    SET(PROJECT_NAME_TEST
        ${PROJECT_NAME}_test)
    FILE(GLOB allTestSource
        "./tests/*.cpp"
        "./tests/application.cpp"
        "./tests/albumview/*.cpp"
        "./tests/allpicview/*.cpp"
        "./tests/controller/*.cpp"
        "./tests/dbmanager/*.cpp"
        "./tests/dialogs/*.cpp"
        "./tests/frame/*.cpp"
        "./tests/imageengine/*.cpp"
        "./tests/importtimelineview/*.cpp"
        "./tests/importview/*.cpp"
        "./tests/module/*.cpp"
        "./tests/module/slideshow/*.cpp"
        "./tests/module/view/*.cpp"
        "./tests/module/view/scen/*.cpp"
        "./tests/module/view/contents/*.cpp"
        "./tests/searchview/*.cpp"
        "./tests/thumbnail/*.cpp"
        "./tests/timelineview/*.cpp"
        "./tests/utils/*.cpp"
        "./tests/widgets/*.cpp"
        "./tests/widgets/dialogs/*.cpp"
        )
    add_subdirectory(tests/googletest)
    add_executable(${PROJECT_NAME_TEST} ${allHeaders} ${allSource} ${allTestSource})
    target_link_libraries(${PROJECT_NAME_TEST} gmock gmock_main gtest gtest_main)

    # 添加 QTest 测试
    add_test(${PROJECT_NAME_TEST} COMMAND ${PROJECT_NAME_TEST})

#------------------------------ 创建'make test'指令---------------------------------------
    add_custom_target(test
        COMMAND mkdir -p coverageResult
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    add_custom_command(TARGET test
        COMMAND echo " =================== CREAT LCOV REPROT BEGIN ==================== "
        #1.清理原先的gcov信息
        COMMAND lcov --directory ./CMakeFiles/${PROJECT_NAME_TEST}.dir --zerocounters
        COMMAND lcov --directory ./CMakeFiles/${PROJECT_NAME}.dir --zerocounters
        COMMAND ./${PROJECT_NAME}/${PROJECT_NAME_TEST}

        #2.收集gcov信息到.info文件中
        COMMAND lcov --directory . --capture --output-file ./coverageResult/${PROJECT_NAME}_Coverage.info

        #3.过滤一些我们不感兴趣的文件的覆盖率信息
        COMMAND echo " =================== do filter begin ==================== "
        COMMAND lcov --remove ./coverageResult/${PROJECT_NAME}_Coverage.info
        '*/${PROJECT_NAME_TEST}_autogen/*' '*/${PROJECT_NAME}_autogen/*' '*/usr/include/*' '*/tests/*' '*/googletest/*'
        -o ./coverageResult/${PROJECT_NAME}_Coverage.info
        COMMAND echo " =================== do filter end ==================== "
        #3.将.info信息生成报告到reprot文件夹中
        COMMAND genhtml -o ./coverageResult/report ./coverageResult/${PROJECT_NAME}_Coverage.info
        COMMAND echo " =================== CREAT LCOV REPROT END ==================== "
        COMMAND echo " Coverage files have been output to ${CMAKE_BINARY_DIR}/coverageResult/report "
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    #'make test'命令依赖与我们的测试程序
    add_dependencies(test ${PROJECT_NAME_TEST})
endif()

#------------------------------ 创建'make test'指令---------------------------------------

#------------------添加第三方库begins--------------------------#
#使用第三方库需要用到的一个包
find_package(PkgConfig REQUIRED)

#检查第三方库(这里检查了名字为dtkwidget的库和名字为dtkgui的库)，然后取名3rd_lib
pkg_check_modules(3rd_lib REQUIRED
        dtkwidget dtkgui gio-qt udisks2-qt5
        )

#添加第三方库的所有文件夹路径到工程中来(注意 *_INCLUDE_DIRS)
target_include_directories(${PROJECT_NAME} PUBLIC ${3rd_lib_INCLUDE_DIRS})
if(DOTEST)
    target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${3rd_lib_INCLUDE_DIRS})
endif()

#将第三方库链接进来(注意 *_LIBRARIES)
target_link_libraries(${PROJECT_NAME} ${3rd_lib_LIBRARIES} UnionImage)
if(DOTEST)
    target_link_libraries(${PROJECT_NAME_TEST} ${3rd_lib_LIBRARIES} UnionImage)
endif()

# 将第三方库链接进来(注意 *_LIBRARIES)
#target_link_libraries(${PROJECT_NAME} ${3rd_lib_LIBRARIES} pthread)

if(DOTEST)
    target_link_libraries(${PROJECT_NAME_TEST} ${3rd_lib_LIBRARIES})
endif()


#------------------添加第三方库end--------------------------#

#将工程与这些模块链接起来
qt5_use_modules(${PROJECT_NAME} ${QtModule})

if(DOTEST)
qt5_use_modules(${PROJECT_NAME_TEST} ${QtModule})
endif()


# ------------------------------- install report script -------------------------------

set(PREFIX /usr)
set(UnionImageDir ${PREFIX}/lib/)
set(TranslationDir ${PREFIX}/share/${CMD_NAME}/translations/)
set(DesktopDir ${PREFIX}/share/applications/)
set(ManIconDir ${PREFIX}/share/icons/hicolor/scalable/apps)
set(AppIconDir ${PREFIX}/share/icons/hicolor/scalable/apps)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION /usr/bin
        INCLUDES DESTINATION allHeaders)

# install Translation files:
install(FILES ${LangSrcs} DESTINATION ${TranslationDir})

# install Desktop icon
install(FILES deepin-album.desktop DESTINATION ${DesktopDir})

# install Manual icon
install(FILES album/resources/images/other/deepin-album.svg DESTINATION ${ManIconDir})

# install App icon
install(FILES album/resources/images/other/deepin-album.svg DESTINATION ${AppIconDir})

#set(CMAKE_CXX_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
