#定义需要的cmake版本
cmake_minimum_required(VERSION 3.10)

project(deepin-album)

execute_process(COMMAND uname -m OUTPUT_VARIABLE MACH
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DMR_DEBUG on)
endif()


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -std=c++14 -O3 -DNDEBUG -fPIC --as-needed")
endif()

################### mips ##################
#if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -std=c++14 -O3 -DNDEBUG -fPIC --as-needed -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mmsa -mloongson-ext2 -mloongson-mmi")
#endif()
##
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

find_package(PkgConfig REQUIRED)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

configure_file(${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h @ONLY)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

message(STATUS ${CMAKE_CXX_FLAGS})

add_subdirectory(libUnionImage)
add_subdirectory(deepin-album)
