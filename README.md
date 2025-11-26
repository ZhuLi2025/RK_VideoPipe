# RK_VideoPipe
本项目主要参考[RK_VideoPipe](https://github.com/alexw914/RK_VideoPipe.git)开源项目, 进行部分修改，适配本人项目使用，如有问题，请参考原作者代码仓库。
本项目目标主要是优化原作者的实现方案：
- 输入节点优化：使用gstreamer拉rtsp流并使用udp传输协议（udp传输不稳定，仅满足本人项目需求），通过gst-sample采集appsink中的nv12格式图像帧，通过rga实现NV12->RGB转换，降低CPU开销。
- 输出节点个性化设计: copy [VideoPipe](https://github.com/sherlockchou86/VideoPipe.git)中的rtsp输出节点，同样使用gstreamer作为推流管道，实现多路UDP共端口无互相干扰输出。

未完成优化：
- 使用rga替代opencv实现预处理、后处理。
- 将dma_fd(dma文件描述符)作为图像帧上下文，替代cv::Mat,目的是方便rga函数的实现以及输出节点可免去videoconvert步骤，进一步减少CPU占用。

### 项目构建

平台
- Ubuntu 22.04 jammy aarch64 / Debain (已测试香橙派5B平台ubuntu系统和Rock5B平台Armbain系统)

环境
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

### 参考项目
[RK_VideoPipe](https://github.com/alexw914/RK_VideoPipe.git):主要参考项目\
[VideoPipe](https://github.com/sherlockchou86/VideoPipe.git): 主要参考项目，大部分节点定义和实现均由该仓库提供 \
[trt_yolo_video_pipeline](https://github.com/1461521844lijin/trt_yolo_video_pipeline.git) 参考了FFmpeg的编解码的实现 \
[rknn_model_zoo](https://github.com/airockchip/rknn_model_zoo) 参考了YOLO系列和分类模型实现，完成了C++类封装 \
[RTMPose-Deploy](https://github.com/HW140701/RTMPose-Deploy) 参考了RTMPose后处理方案，放弃了仿射变换实现，转用LetterBox实现。
