#include "rga/RgaApi.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include "vp_rtsp_src_node.h"
#include "vp_utils/vp_utils.h"

namespace vp_nodes {

vp_rtsp_src_node::vp_rtsp_src_node(std::string node_name,
                                   int channel_index,
                                   std::string rtsp_url,
                                   float resize_ratio,
                                   std::string gst_decoder_name,
                                   int skip_interval)
    : vp_src_node(node_name, channel_index, resize_ratio),
      rtsp_url(rtsp_url), gst_decoder_name(gst_decoder_name), skip_interval(skip_interval) {

    assert(skip_interval >= 0 && skip_interval <= 9);
    this->gst_template = vp_utils::string_format(this->gst_template, rtsp_url.c_str(), gst_decoder_name.c_str());
    VP_INFO(vp_utils::string_format("[%s] pipeline: %s", node_name.c_str(), gst_template.c_str()));
    this->initialized();
}

vp_rtsp_src_node::~vp_rtsp_src_node() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    deinitialized();
}

cv::Mat vp_rtsp_src_node::rga_convert_nv12_to_bgr(void* nv12_ptr, int width, int height) {
    cv::Mat bgr_mat(height, width, CV_8UC3);

    rga_buffer_t src;
    src.width = width;
    src.height = height;
    src.format = RK_FORMAT_YCbCr_420_SP;
    src.vir_addr = nv12_ptr;
    src.fd = -1;

    rga_buffer_t dst;
    dst.width = width;
    dst.height = height;
    dst.format = RK_FORMAT_BGR_888;
    dst.vir_addr = bgr_mat.data;
    dst.fd = -1;

    imcopy(src, dst, 0);  // 调用 RGA 硬件转换
    return bgr_mat;
}

void vp_rtsp_src_node::handle_run() {
    int video_width = 0, video_height = 0, fps = 0, skip = 0;

    // 初始化 GStreamer
    gst_init(nullptr, nullptr);
    GError* error = nullptr;
    pipeline = gst_parse_launch(this->gst_template.c_str(), &error);
    if (!pipeline) {
        VP_WARN("Failed to create GStreamer pipeline");
        return;
    }

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    gst_app_sink_set_max_buffers(GST_APP_SINK(appsink), 2);
    gst_app_sink_set_drop(GST_APP_SINK(appsink), TRUE);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    while (alive) {
        gate.knock();

        // 拉取 sample
        GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), GST_SECOND);
        if (!sample) {
            VP_WARN("Failed to pull GstSample");
            continue;
        }

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            VP_WARN("Failed to map GstBuffer");
            gst_sample_unref(sample);
            continue;
        }

        if (video_width == 0 || video_height == 0) {
            GstCaps* caps = gst_sample_get_caps(sample);
            GstStructure* s = gst_caps_get_structure(caps, 0);
            gst_structure_get_int(s, "width", &video_width);
            gst_structure_get_int(s, "height", &video_height);
            fps = 20; // 可以从 caps 读取或固定
        }

        // 跳帧逻辑
        if (skip < skip_interval) {
            skip++;
            gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);
            continue;
        }
        skip = 0;

        // NV12 -> BGR
        cv::Mat rga_bgr = rga_convert_nv12_to_bgr(map.data, video_width, video_height);

        cv::Mat resize_frame;
        if (this->resize_ratio != 1.0f) {
            cv::resize(rga_bgr, resize_frame, cv::Size(), resize_ratio, resize_ratio);
        } else {
            resize_frame = rga_bgr.clone();
        }

        video_width = resize_frame.cols;
        video_height = resize_frame.rows;

        this->frame_index++;
        auto out_meta = std::make_shared<vp_objects::vp_frame_meta>(
            resize_frame, this->frame_index, this->channel_index, video_width, video_height, fps
        );

        if (out_meta) {
            this->out_queue.push(out_meta);
            if (this->meta_handled_hooker) {
                meta_handled_hooker(node_name, out_queue.size(), out_meta);
            }
            this->out_queue_semaphore.signal();
        }

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
    }

    this->out_queue.push(nullptr);
    this->out_queue_semaphore.signal();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    pipeline = nullptr;
}

std::string vp_rtsp_src_node::to_string() {
    return rtsp_url;
}

} // namespace vp_nodes
