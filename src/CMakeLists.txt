#定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)

set(CMD_NAME deepin-album)

project(${CMD_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(DOTEST)
    set(QtModule Core Gui Widgets DBus Sql PrintSupport Svg Concurrent LinguistTools Test)
    set(CMAKE_CXX_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer -g -fprofile-arcs -ftest-coverage -lgcov")
else()
    set(QtModule Core Gui Widgets DBus Sql PrintSupport Svg Concurrent LinguistTools)
endif()

find_package(Qt5 REQUIRED ${QtModule})

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

FILE (GLOB allHeaders
    "./album/module/*.h"
    )

set(AllQRC
    album/resources.qrc
    icons/icons.qrc
    )
file(GLOB TargetTsFiles LIST_DIRECTORIES false ../translations/${CMD_NAME}*.ts)

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

qt5_create_translation(LangSrcs ${allSource} ${allHeaders} ${TargetTsFiles}
    OPTIONS )
qt5_add_translation(${TargetTsFiles})
set_source_files_properties(${qmfiles} PROPERTIES OUTPUT_LOCATION "translations")

set(LINK_DIR lib)
link_directories(${LINK_DIR})

add_executable(${PROJECT_NAME} ${allSource} ${AllQRC} ${allHeaders} ${LangSrcs} )

add_compile_definitions(LITE_DIV)

#------------------------------ 配置test ---------------------------------------

if(DOTEST)
    SET(PROJECT_NAME_TEST
        ${PROJECT_NAME}_test)

    include_directories(./../tests/)

    aux_source_directory(./../tests/ allTestSource)
    aux_source_directory(./../tests/albumview allTestSource)
    aux_source_directory(./../tests/allpicview allTestSource)
    aux_source_directory(./../tests/controller allTestSource)
    aux_source_directory(./../tests/dbmanager allTestSource)
    aux_source_directory(./../tests/dialogs allTestSource)
    aux_source_directory(./../tests/frame allTestSource)
    aux_source_directory(./../tests/imageengine allTestSource)
    aux_source_directory(./../tests/importtimelineview allTestSource)
    aux_source_directory(./../tests/importview allTestSource)
    aux_source_directory(./../tests/module allTestSource)
    aux_source_directory(./../tests/module/slideshow allTestSource)
    aux_source_directory(./../tests/module/view allTestSource)
    aux_source_directory(./../tests/module/view allTestSource)
    aux_source_directory(./../tests/module/view allTestSource)
    aux_source_directory(./../tests/searchview allTestSource)
    aux_source_directory(./../tests/thumbnail allTestSource)
    aux_source_directory(./../tests/timelineview allTestSource)
    aux_source_directory(./../tests/utils allTestSource)
    aux_source_directory(./../tests/widgets allTestSource)
    aux_source_directory(./../tests/widgets/dialogs allTestSource)
    aux_source_directory(./album/albumview allTestSource)
    aux_source_directory(./album/allpicview allTestSource)
    aux_source_directory(./album/controller allTestSource)
    aux_source_directory(./album/dbmanager allTestSource)
    aux_source_directory(./album/dialogs allTestSource)
    aux_source_directory(./album/frame allTestSource)
    aux_source_directory(./album/imageengine allTestSource)
    aux_source_directory(./album/importtimelineview allTestSource)
    aux_source_directory(./album/importview allTestSource)
    aux_source_directory(./album/module allTestSource)
    aux_source_directory(./album/module/slideshow allTestSource)
    aux_source_directory(./album/module/view allTestSource)
    aux_source_directory(./album/module/view/scen allTestSource)
    aux_source_directory(./album/module/view/contents allTestSource)
    aux_source_directory(./album/searchview allTestSource)
    aux_source_directory(./album/settings/slideshow allTestSource)
    aux_source_directory(./album/thumbnail allTestSource)
    aux_source_directory(./album/timelineview allTestSource)
    aux_source_directory(./album/utils allTestSource)
    aux_source_directory(./album/widgets allTestSource)
    aux_source_directory(./album/widgets/dialogs allTestSource)

    FILE(GLOB allTestSource1
        "./album/application.cpp"
        "./album/mainwindow.cpp"
        "./album/fileinotify.cpp"
        )

    add_executable(${PROJECT_NAME_TEST} ${allHeaders} ${AllTestQRC} ${allTestSource} ${allTestSource1})
    target_link_libraries(${PROJECT_NAME_TEST} gmock gtest pthread)

    add_test(${PROJECT_NAME_TEST} COMMAND ${PROJECT_NAME_TEST})

#------------------------------ 创建'make test'指令---------------------------------------
    add_custom_target(test
        COMMAND mkdir -p coverageResult
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    add_custom_command(TARGET test
        COMMAND echo " =================== CREAT LCOV REPROT BEGIN ==================== "

        COMMAND lcov --directory ./CMakeFiles/${PROJECT_NAME_TEST}.dir --zerocounters
        COMMAND lcov --directory ./CMakeFiles/${PROJECT_NAME}.dir --zerocounters
        COMMAND ./${PROJECT_NAME}/${PROJECT_NAME_TEST}

        COMMAND lcov --directory . --capture --output-file ./coverageResult/${PROJECT_NAME}_Coverage.info

        COMMAND echo " =================== do filter begin ==================== "
        COMMAND lcov --remove ./coverageResult/${PROJECT_NAME}_Coverage.info
        '*/${PROJECT_NAME_TEST}_autogen/*' '*/${PROJECT_NAME}_autogen/*' '*/usr/include/*' '*/tests/*' '*/googletest/*'
        -o ./coverageResult/${PROJECT_NAME}_Coverage.info
        COMMAND echo " =================== do filter end ==================== "

        COMMAND genhtml -o ./coverageResult/report ./coverageResult/${PROJECT_NAME}_Coverage.info
        COMMAND echo " =================== CREAT LCOV REPROT END ==================== "

#        COMMAND nohup x-www-browser ${CMAKE_BINARY_DIR}/coverageResult/report/index.html &

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(test ${PROJECT_NAME_TEST})
endif()

#------------------------------ 创建'make test'指令---------------------------------------

find_package(PkgConfig REQUIRED)
pkg_check_modules(3rd_lib REQUIRED
        dtkwidget dtkgui gio-qt udisks2-qt5
        )
target_include_directories(${PROJECT_NAME} PUBLIC ${3rd_lib_INCLUDE_DIRS})
if(DOTEST)
    target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${3rd_lib_INCLUDE_DIRS})
endif()

target_link_libraries(${PROJECT_NAME} ${3rd_lib_LIBRARIES} UnionImage)
if(DOTEST)
    target_link_libraries(${PROJECT_NAME_TEST} ${3rd_lib_LIBRARIES} UnionImage)
endif()
#------------------添加第三方库end--------------------------#


qt5_use_modules(${PROJECT_NAME} ${QtModule})
if(DOTEST)
qt5_use_modules(${PROJECT_NAME_TEST} ${QtModule})
endif()

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

# install
install(FILES ${LangSrcs} DESTINATION ${TranslationDir})
install(FILES deepin-album.desktop DESTINATION ${DesktopDir})
install(FILES album/resources/images/other/deepin-album.svg DESTINATION ${ManIconDir})
install(FILES album/resources/images/other/deepin-album.svg DESTINATION ${AppIconDir})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/../assets/deepin-album DESTINATION /usr/share/deepin-manual/manual-assets/application/)

#set(CMAKE_CXX_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
