cmake_minimum_required(VERSION 3.4.1)

project(demo)

# ======= 通用设置 =======
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIB_ARCH aarch64)

# ======= 可选：默认构建类型为 Release =======
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ======= 编译优化选项 =======
#set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -DDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=armv8-a -mtune=cortex-a76.cortex-a55 -fopenmp")

# ======= 链接器配置 =======
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

# ======= 安装路径 =======
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build/bin)
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/build/lib)

# ======= 基础配置 =======
include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

# ======= 依赖与路径 =======
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty 3rdparty)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/media
    ${CMAKE_SOURCE_DIR}/bytetrack
    ${CMAKE_SOURCE_DIR}/videocodec
    ${CMAKE_SOURCE_DIR}/vp_node
    ${BASE_INCLUDES}
)

add_subdirectory(${CMAKE_SOURCE_DIR}/models rknn_models)
add_subdirectory(${CMAKE_SOURCE_DIR}/bytetrack bytetrack)
add_subdirectory(${CMAKE_SOURCE_DIR}/videocodec videocodec)
add_subdirectory(${CMAKE_SOURCE_DIR}/vp_node vp_node)

add_executable(demo
    main.cc
)

target_link_libraries(demo
    ${COMMON_LIBS}
    ${BASE_LIBS}
    rknn_models
    videocodec
    vp_node
)

# ======= 安装 =======
install(TARGETS  ${PROJECT_NAME} DESTINATION .)
install(PROGRAMS ${BASE_LIBS} ${RKNN_RT_LIB} ${MPP_LIB} ${RGA_LIB} ${YAML_LIB} ${ZLMEDIAKIT_LIB} DESTINATION lib)
