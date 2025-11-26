#include "vp_udp_des_node.h"
#include "vp_utils/vp_utils.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace vp_nodes {

vp_udp_des_node::vp_udp_des_node(std::string node_name,
                                 int channel_index,
                                 std::string host,
                                 int port)
    : vp_des_node(node_name, channel_index),
      host(host), port(port), frame_counter(0) {

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &remote_addr.sin_addr) <= 0) {
        std::cerr << "Invalid host IP: " << host << std::endl;
        exit(EXIT_FAILURE);
    }

    // 扩大发送缓冲区
    int buf_size = 4 * 1024 * 1024;
    setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));

    VP_INFO(vp_utils::string_format("[%s] UDP sender initialized: %s:%d", node_name.c_str(), host.c_str(), port));

    this->initialized();
}

vp_udp_des_node::~vp_udp_des_node() {
    close(sock_fd);
    deinitialized();
}

// 重写 handle_frame_meta，用 UDP 发送帧
std::shared_ptr<vp_objects::vp_meta> vp_udp_des_node::handle_frame_meta(std::shared_ptr<vp_objects::vp_frame_meta> meta) {
    VP_DEBUG(vp_utils::string_format("[%s] received frame meta, channel_index=>%d, frame_index=>%d", 
                                     node_name.c_str(), meta->channel_index, meta->frame_index));

    cv::Mat frame_to_send = (meta->osd_frame.empty()) ? meta->frame : meta->osd_frame;

    send_frame(frame_to_send);

    // 调用父类处理通用逻辑
    return vp_des_node::handle_frame_meta(meta);
}

// 重写 handle_control_meta
std::shared_ptr<vp_objects::vp_meta> vp_udp_des_node::handle_control_meta(std::shared_ptr<vp_objects::vp_control_meta> meta) {
    return vp_des_node::handle_control_meta(meta);
}

// UDP 发送帧函数
void vp_udp_des_node::send_frame(const cv::Mat& frame) {
    if (frame.empty()) return;

    uint32_t frame_id = frame_counter++;
    size_t total_size = frame.total() * frame.elemSize();
    const uint8_t* data_ptr = frame.ptr<uint8_t>(0);

    const size_t PACKET_SIZE = 60000;
    uint8_t packet[PACKET_SIZE + 12]; // 头部12字节 + payload

    for (size_t offset = 0; offset < total_size; offset += PACKET_SIZE) {
        size_t chunk = std::min(PACKET_SIZE, total_size - offset);

        // header: [frame_id|total_size|offset]
        memcpy(packet, &frame_id, 4);
        memcpy(packet + 4, &total_size, 4);
        uint32_t off32 = offset;
        memcpy(packet + 8, &off32, 4);

        memcpy(packet + 12, data_ptr + offset, chunk);

        ssize_t sent = sendto(sock_fd, packet, chunk + 12, 0,
                              (struct sockaddr*)&remote_addr, sizeof(remote_addr));
        if (sent < 0) {
            perror("sendto failed");
            break;
        }
    }
}

} // namespace vp_nodes
