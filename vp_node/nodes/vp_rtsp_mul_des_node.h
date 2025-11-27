// ===== vp_rtsp_mul_des_node.h =====

#pragma once

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include "base/vp_des_node.h"

/*
* ##### Compile Tips #####
* install additional packages for this node before compiling:
* sudo apt-get install libgstrtspserver-1.0-dev gstreamer1.0-rtsp
* https://github.com/GStreamer/gst-rtsp-server
*/

namespace vp_nodes {
    // rtsp des node, push video stream via rtsp.
    // example: rtsp://localhost:7890/test
    // note, no specialized rtsp server needed since this node itself is a rtsp server, which can be pulled directly by video players such as VLC.
    class vp_rtsp_mul_des_node: public vp_des_node {
    private:
        /* data */
        // pipeline template: appsrc -> encoder -> h264parse -> rtph264pay -> tee -> udpsink(RTP) + udpsink(RTCP)
        std::string gst_template =
            "appsrc is-live=true format=time ! "
            "videoconvert ! video/x-raw,format=NV12 ! "
            "%s rc-mode=cbr gop=30 ! "
            "h264parse ! rtph264pay config-interval=1 name=pay0 pt=96 ! "
            "tee name=t "
            "t. ! queue ! udpsink host=%s port=%d sync=false async=false "
            "t. ! queue ! udpsink host=%s port=%d sync=false async=false";

        cv::VideoWriter rtsp_writer;

        // start rtsp server
        void start_rtsp_streaming ();
        
        // resolution for rtsp stream
        vp_objects::vp_size resolution_w_h;
        int bitrate;
        // for osd frame 
        bool osd;

        std::string host_ip;

        int udp_buffer_size = 0;
        // base udp port for stream data exchange internally, used in pair per channel
        const int base_udp_port = 7890; // default base port

        int rtsp_port = 9000;          // RTSP signaling port (service)
        std::string rtsp_name = "";
        int RTP;                       // RTP data port for this channel
        int RTCP ;                     // RTCP data port for this channel
        /* gst-rtsp-server variables shared between instances for different channels */
        static GstRTSPServer* rtsp_server;

        // set x264enc as the default encoder, we can use hardware encoder instead.
        std::string gst_encoder_name = "mpph264enc";
    protected:
        // re-implementation, return nullptr.
        virtual std::shared_ptr<vp_objects::vp_meta> handle_frame_meta(std::shared_ptr<vp_objects::vp_frame_meta> meta) override; 
    public:
        vp_rtsp_mul_des_node(std::string node_name, 
                        int channel_index, 
                        std::string host_ip,
                        int rtsp_port = 9000, 
                        std::string rtsp_name = "", 
                        vp_objects::vp_size resolution_w_h = {}, 
                        int bitrate = 512,
                        bool osd = true,
                        std::string gst_encoder_name = "mpph264enc");
        ~vp_rtsp_mul_des_node();

         virtual std::string to_string() override;
    };
}

