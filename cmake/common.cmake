# threads
find_package(Threads REQUIRED)

# opencv header：/usr/local/include
set(OpenCV_DIR /usr/local/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)

# ffmpeg path
include_directories(/usr/include/aarch64-linux-gnu)
include_directories(/usr/include)
file(GLOB_RECURSE FFmpeg_LIBS
        /usr/lib/aarch64-linux-gnu/libav*.so
        /usr/lib/aarch64-linux-gnu/libsw*.so
        /usr/lib/aarch64-linux-gnu/libpostproc.so)

# -----------------------------
# ✅ GStreamer (头文件 + 库)
# -----------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-video-1.0
        gstreamer-rtsp-server-1.0
        )

# 包含头文件路径
include_directories(
    ${GST_INCLUDE_DIRS}
)

# 链接库路径
link_directories(
    ${GST_LIBRARY_DIRS}
)

# -----------------------------
# 公共库集合
# -----------------------------
set(COMMON_LIBS
    Threads::Threads
    ${FFmpeg_LIBS}
    ${OpenCV_LIBS}
    ${GST_LIBRARIES}
)
