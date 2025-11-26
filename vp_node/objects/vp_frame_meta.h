#pragma once

#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "vp_meta.h"
#include "vp_frame_target.h"
#include "vp_frame_pose_target.h"
#include "vp_frame_face_target.h"
#include "vp_frame_text_target.h"
#include "ba/vp_ba_result.h"

// RK3399/RK3588 RGA/MPP buffer
#include "im2d.h"
#include "RgaUtils.h"

/*
* ##########################################
* how does frame meta work?
* ##########################################
* frame meta, holding all data(targets/elements/...) of current frame in the video scene. frame meta are independent and don't know about each other, neither its previous frames nor next frames.
* the data in frame meta is just telling us what **current frame** is so we can not get something like 'state-switch' from a single frame meta. 
* if you need know when the 'state-switch' happen, for example, you want to notify to cloud via restful api if state changed(ignore if it's keeping), 
* you need cache previous frame meta(maybe partial data) in your custom node first and then compare with each other to figure out if it has changed.
* 
* frame meta works like our eyes, by taking a glance at the frame in video we can see what the picture is and how many targets are there.
* but if you want to  know something like state-switch, for example, a person was walking and then stop or it stop for a while and then start to walk, you have to see(cache) more frames.
* 
* see more implementation of 'vp_track_node' and 'vp_message_broker_node' which saved history frame meta data and then work based on them.
* 1. vp_track_node          : save previous locations of targets and then do tracking based on them, we need see more frames to track targets in video.
* 2. vp_message_broker_node : save previous ba_flags and then do notifying based on them, we need see more frames to check if state-switch has happened.
* ##########################################
*/ 
namespace vp_objects {

// frame meta 支持 zero-copy
class vp_frame_meta : public vp_meta {
public:
    int frame_index;
    int fps;
    int original_width;
    int original_height;

    // CPU Mat 可选，用于 CPU 处理或显示
    cv::Mat frame;       // 深拷贝模式使用
    cv::Mat osd_frame;
    cv::Mat mask;

    // GPU DMA-FD buffer
    int dma_fd{-1};      // 如果使用 zero-copy，这里保存 fd
    rga_buffer_t rga_buf; // 可选 RGA buffer 描述

    // targets
    std::vector<std::shared_ptr<vp_frame_target>> targets;
    std::vector<std::shared_ptr<vp_frame_pose_target>> pose_targets;
    std::vector<std::shared_ptr<vp_frame_face_target>> face_targets;
    std::vector<std::shared_ptr<vp_frame_text_target>> text_targets;
    std::vector<std::shared_ptr<vp_ba_result>> ba_results;

public:
    // CPU deep-copy 构造
    vp_frame_meta(cv::Mat _frame, int _frame_index = -1, int _channel_index = -1,
                  int _original_width = 0, int _original_height = 0, int _fps = 0);

    // DMA-FD 构造
    vp_frame_meta(int _dma_fd, int width, int height, int _channel_index = -1,int _frame_index = -1, int _fps = 0);

    ~vp_frame_meta() ;

    // copy constructor, 支持 CPU Mat 和 DMA-FD 都能复制
    vp_frame_meta(const vp_frame_meta& meta);

    // get targets by ids
    std::vector<std::shared_ptr<vp_frame_target>> get_targets_by_ids(const std::vector<int>& ids);

    // clone
    std::shared_ptr<vp_meta> clone() override;

    // helper: 将 DMA-FD buffer 映射为 CPU Mat（如果需要在 CPU 上操作）
    cv::Mat get_cpu_mat();
};

} // namespace vp_objects