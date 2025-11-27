#include "rga/RgaApi.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include "vp_rtsp_src_node.h"
#include "vp_utils/vp_utils.h"

namespace vp_nodes {

// NOTE: 这个补丁把 rga_convert_nv12_to_bgr 改为接收 GstBuffer*，优先使用 dmabuf fd
// 如果不是 dmabuf 则回退到映射(map)并复制到可访问的 CPU 内存再做转换。

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

// 新版：接收 GstBuffer*，优先使用 dmabuf fd -> wrapbuffer_fd
cv::Mat vp_rtsp_src_node::rga_convert_nv12_to_bgr(GstBuffer* buffer, int width, int height) {
    cv::Mat bgr_mat(height, width, CV_8UC3);

    GstMemory* mem = nullptr;
    GstMapInfo map;
    bool mapped = false;

    // 尝试从 buffer 获取第 0 个 memory
    mem = gst_buffer_peek_memory(buffer, 0);
    if (!mem) {
        VP_WARN("No GstMemory found in buffer");
        return bgr_mat;
    }

    // 如果是 dmabuf memory，直接取 fd 给 RGA（零拷贝）
    if (gst_is_dmabuf_memory(mem)) {
        int dma_fd = gst_dmabuf_memory_get_fd(mem);
        if (dma_fd < 0) {
            VP_WARN("dmabuf memory but failed to get fd");
            // 回退到映射处理
        } else {
            // wrapbuffer_fd 会自动设置 stride/format 等
            rga_buffer_t src = wrapbuffer_fd(dma_fd, width, height, RK_FORMAT_YCbCr_420_SP);
            rga_buffer_t dst = wrapbuffer_virtualaddr(bgr_mat.data, width, height, RK_FORMAT_BGR_888);

            IM_STATUS st = imcvtcolor(src, dst, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_BGR_888);
            if (st != IM_STATUS_SUCCESS) {
                VP_WARN(vp_utils::string_format("imcvtcolor failed: %d", st));
            }
            return bgr_mat;
        }
    }

    // 非 dmabuf 回退：map 并复制到连续 CPU 内存（确保 RGA 能访问）
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        mapped = true;
        // 计算 NV12 大小：Y plane + UV plane
        size_t nv12_size = map.size;

        // 申请一块对齐的 CPU 内存（这里用 malloc 简单示例）
        uint8_t* nv12_cpu = (uint8_t*)malloc(nv12_size);
        if (!nv12_cpu) {
            VP_WARN("malloc nv12_cpu failed");
            gst_buffer_unmap(buffer, &map);
            return bgr_mat;
        }
        memcpy(nv12_cpu, map.data, nv12_size);

        // 必须设置 stride（按照 16 字节对齐），否则 RGA 读取可能出错
        int y_stride = (width + 15) & (~15);
        int uv_stride = y_stride; // NV12 UV stride 与 Y stride 一致（常见情况）

        rga_buffer_t src;
        memset(&src, 0, sizeof(src));
        src.fd = -1;
        src.vir_addr = nv12_cpu;
        src.width = width;
        src.height = height;
        src.wstride = y_stride;
        src.hstride = height;
        src.format = RK_FORMAT_YCbCr_420_SP;

        rga_buffer_t dst = wrapbuffer_virtualaddr(bgr_mat.data, width, height, RK_FORMAT_BGR_888);

        // 使用 imcvtcolor（色彩转换），也可以用 imcopy 但 imcvtcolor 更合适
        IM_STATUS st = imcvtcolor(src, dst, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_BGR_888);
        if (st != IM_STATUS_SUCCESS) {
            VP_WARN(vp_utils::string_format("imcvtcolor (fallback) failed: %d", st));
        }

        free(nv12_cpu);
        gst_buffer_unmap(buffer, &map);
        return bgr_mat;
    }

    VP_WARN("Failed to map GstBuffer (non-dmabuf fallback)");
    return bgr_mat;
}

void vp_rtsp_src_node::handle_run() {
    int video_width = 0, video_height = 0, fps = 0;

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

    int skip = 0;

    while (alive) {
        gate.knock();

        // 拉取 sample
        GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), GST_SECOND);
        if (!sample) {
            VP_WARN("Failed to pull GstSample");
            continue;
        }

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        if (!buffer) {
            VP_WARN("sample has no buffer");
            gst_sample_unref(sample);
            continue;
        }

        if (video_width == 0 || video_height == 0) {
            GstCaps* caps = gst_sample_get_caps(sample);
            if (caps) {
                GstStructure* s = gst_caps_get_structure(caps, 0);
                gst_structure_get_int(s, "width", &video_width);
                gst_structure_get_int(s, "height", &video_height);
                // 尝试读取 fps
                int num = 0, denom = 1;
                if (gst_structure_get_fraction(s, "framerate", &num, &denom)) {
                    if (denom != 0) fps = num / denom;
                }
                if (fps == 0) fps = 20; // fallback
            }
        }
        // stream_info_hooker activated if need
        vp_stream_info stream_info {channel_index, fps, video_width, video_height, to_string()};
        invoke_stream_info_hooker(node_name, stream_info);
        // 跳帧逻辑
        if (skip < skip_interval) {
            skip++;
            gst_sample_unref(sample);
            continue;
        }
        skip = 0;

        // 使用基于 GstBuffer 的 RGA 转换（优先 dmabuf）
        cv::Mat rga_bgr = rga_convert_nv12_to_bgr(buffer, video_width, video_height);

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
