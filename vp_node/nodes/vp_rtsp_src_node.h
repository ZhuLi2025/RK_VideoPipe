#pragma once

#include <string>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "base/vp_src_node.h"

namespace vp_nodes {

class vp_rtsp_src_node : public vp_src_node {
private:
    std::string gst_template = "rtspsrc location=%s protocols=udp latency=0 drop-on-latency=true "
                               "! rtph264depay "
                               "! h264parse "
                               "! %s "
                               "! video/x-raw,format=NV12 "
                               "! appsink name=mysink emit-signals=true max-buffers=2 drop=true sync=false";

    GstElement* pipeline = nullptr;
    GstElement* appsink = nullptr;

    // 拉取 GstSample 并转换成 BGR
    cv::Mat rga_convert_nv12_to_bgr(GstBuffer* nv12_ptr, int width, int height);

protected:
    virtual void handle_run() override;

public:
    vp_rtsp_src_node(std::string node_name,
                     int channel_index,
                     std::string rtsp_url,
                     float resize_ratio = 1.0,
                     std::string gst_decoder_name = "mppvideodec",
                     int skip_interval = 1);

    ~vp_rtsp_src_node();

    virtual std::string to_string() override;

    std::string rtsp_url;
    std::string gst_decoder_name = "mppvideodec";
    int skip_interval = 0;
};

} // namespace vp_nodes
