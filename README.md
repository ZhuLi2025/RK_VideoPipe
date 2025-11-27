#  1. RK_VideoPipe
## 1.1. 项目简介
本项目主要参考[RK_VideoPipe](https://github.com/alexw914/RK_VideoPipe.git)开源项目, 进行部分修改，适配本人项目使用，如有问题，请参考原作者代码仓库。
本项目目标主要是优化原作者的实现方案：
- 输入节点优化：使用gstreamer拉rtsp流并使用udp传输协议（udp传输不稳定，仅满足本人项目需求），通过gst-sample采集appsink中的nv12格式图像帧，通过rga实现NV12->RGB转换，降低CPU开销。
- 输出节点个性化设计: copy [VideoPipe](https://github.com/sherlockchou86/VideoPipe.git)中的rtsp输出节点，同样使用gstreamer作为推流管道，实现多路UDP共端口无互相干扰输出。

未完成优化：
- 使用rga替代opencv实现预处理、后处理。
- 将dma_fd(dma文件描述符)作为图像帧上下文，替代cv::Mat,目的是方便rga函数的实现以及输出节点可免去videoconvert步骤，进一步减少CPU占用。


## 1.2. 项目构建

### 1.2.1. 平台
- Ubuntu 22.04 jammy aarch64 / Debain (已测试香橙派5B平台ubuntu系统和Rock5B平台Armbain系统)

### 1.2.2. 环境
- C++ 17
- OpenCV >= 4.6 (需支持FreeType, 否则需要改写部分OSD节点)
- GStreamer (官网推荐完整安装，需额外支持rkmpp插件)
- FFmpeg >= 4.3 (需要编译mpp插件或使用推荐源)

需要校对cmake目录下的common.cmake文件中定义了FFmpeg、Gstreamer与OpenCV位置, 如果不符合则需要修改。构建项目后执行build/bin下可执行文件即可运行案例
```
cd RK_VideoPipe
./build.sh
build/demo
```
### 1.2.3. 推理库安装/更新
使用最新版的rknnruntime库，可通过下列方式检查librknnrt.so版本：
```
strings /path/to/librknnrt.so | grep version
```
如果版本低，去[rknn-toolkit2](https://github.com/airockchip/rknn-toolkit2.git)仓库找到librknnrt.so文件，下载并替换本地老旧版本即可。

### 1.2.4. 补充库安装
- vp_rtsp_mul_des_node节点需要gst的rtsp server:
```
sudo apt-get install libgstrtspserver-1.0-dev gstreamer1.0-rtsp
```

- 输出输出节点需要使用mpp硬件编解码：
如果你的镜像中有mpp的编解码器，请忽视（gst-inspect-1.0 |grep mpp 如果输出中包含mpph264enc/mpph265enc、mppvideodec则表示gstreamer已经具备mpp编解码插件）
>前置依赖安装
```
sudo apt-get update
sudo apt-get install -y \
    git build-essential meson ninja-build pkg-config \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libdrm-dev \
    libglib2.0-dev
```
   >安装 MPP（从源码）
```
git clone https://github.com/rockchip-linux/mpp.git
cd mpp
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```
> 编译 gstreamer-rockchip 插件
```
git clone https://github.com/Lockzhiner/gstreamer-rockchip.git
cd gstreamer-rockchip
meson build && ninja -C build
sudo ninja -C build install
```
## 1.3. 参考项目
[RK_VideoPipe](https://github.com/alexw914/RK_VideoPipe.git):主要参考项目\
[VideoPipe](https://github.com/sherlockchou86/VideoPipe.git): 主要参考项目，大部分节点定义和实现均由该仓库提供 \
[trt_yolo_video_pipeline](https://github.com/1461521844lijin/trt_yolo_video_pipeline.git) 参考了FFmpeg的编解码的实现 \
[rknn_model_zoo](https://github.com/airockchip/rknn_model_zoo) 参考了YOLO系列和分类模型实现，完成了C++类封装 \
[RTMPose-Deploy](https://github.com/HW140701/RTMPose-Deploy) 参考了RTMPose后处理方案，放弃了仿射变换实现，转用LetterBox实现。
