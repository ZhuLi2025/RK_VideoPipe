#pragma once

#include "base/vp_des_node.h"
#include "vp_utils/vp_utils.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <memory>
#include <cstdint>
#include <arpa/inet.h>

namespace vp_nodes {

class vp_udp_des_node : public vp_des_node {
public:
    vp_udp_des_node(std::string node_name,
                    int channel_index,
                    std::string host,
                    int port);
    ~vp_udp_des_node();

    // 重写父类方法
    std::shared_ptr<vp_objects::vp_meta> handle_frame_meta(std::shared_ptr<vp_objects::vp_frame_meta> meta) override;
    std::shared_ptr<vp_objects::vp_meta> handle_control_meta(std::shared_ptr<vp_objects::vp_control_meta> meta) override;

private:
    void send_frame(const cv::Mat& frame);  // UDP 发送单帧函数

private:
    int sock_fd;
    struct sockaddr_in remote_addr;
    uint32_t frame_counter;
    std::string host;
    int port;
};

} // namespace vp_nodes
