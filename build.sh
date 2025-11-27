#!/usr/bin/env bash

# ===============================================
# rk_videopipe 自动构建脚本
# 使用方法：
#   ./build.sh debug    # 构建 Debug 版本
#   ./build.sh release  # 构建 Release 版本
# ===============================================

set -e  # 遇到错误直接退出

# TARGET_SOC="rk3588"
GCC_COMPILER=aarch64-linux-gnu

export LD_LIBRARY_PATH=${TOOL_CHAIN}/lib64:$LD_LIBRARY_PATH
export CC=${GCC_COMPILER}-gcc
export CXX=${GCC_COMPILER}-g++

# 1. 解析构建类型
BUILD_TYPE=${1:-release}
BUILD_DIR="build"

if [[ "$BUILD_TYPE" == "debug" ]]; then
    CMAKE_BUILD_TYPE="Debug"
elif [[ "$BUILD_TYPE" == "release" ]]; then
    CMAKE_BUILD_TYPE="Debug"
else
    #echo "❌ 无效参数: $BUILD_TYPE"
    #echo "用法: ./build.sh [debug|release]"
    #exit 1
    CMAKE_BUILD_TYPE="Debug"
fi

echo "🛠 构建类型: $CMAKE_BUILD_TYPE"
echo "📁 构建目录: $BUILD_DIR"

# 2. 创建并进入构建目录
if [ ! -d "${BUILD_DIR}" ]; then
  mkdir -p ${BUILD_DIR}
fi
cd $BUILD_DIR

# 3. 运行 CMake 配置
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..

# 4. 编译
make -j$(nproc)

# 5. （可选）安装输出
# make install

echo "✅ 构建完成: $CMAKE_BUILD_TYPE"
