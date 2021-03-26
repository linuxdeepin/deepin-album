#定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)

project(libUnionImage)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QtModule Core Gui Widgets Svg Concurrent)
find_package(Qt5 REQUIRED ${QtModule})

include_directories(UnionImage)

aux_source_directory(UnionImage allSources)

#------------------添加第三方库begins--------------------------#

set(INC_DIR /usr/include/)
set(LINK_DIR /usr/lib/)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})
link_libraries(freeimage)

add_library(unionimage SHARED ${allHeaders} ${allSources})
set_target_properties(unionimage PROPERTIES VERSION 1.0.0 SOVERSION 1)

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

qt5_use_modules(unionimage ${QtModule})
set(PREFIX /usr)
set(UnionImageDir ${PREFIX}/lib)

install(TARGETS unionimage LIBRARY DESTINATION ${UnionImageDir})
