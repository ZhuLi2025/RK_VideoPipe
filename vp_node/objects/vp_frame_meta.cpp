#include <iterator>

#include "vp_frame_meta.h"

namespace vp_objects {
        
    vp_frame_meta::vp_frame_meta(cv::Mat frame, int frame_index, int channel_index, int original_width, int original_height, int fps): 
        vp_meta(vp_meta_type::FRAME, channel_index), 
        frame_index(frame_index), 
        original_width(original_width),
        original_height(original_height),
        fps(fps),
        frame(frame) {
            assert(!frame.empty());
    }
    
    // DMA-FD 构造
    vp_frame_meta::vp_frame_meta(int _dma_fd,  int width, int height,int channel_index, int _frame_index , int _fps)
        : vp_meta(vp_meta_type::FRAME, channel_index), frame_index(_frame_index), fps(_fps), original_width(width), original_height(height), dma_fd(_dma_fd)
    {
        // 包装 DMA-FD buffer 到 rga_buffer_t
        rga_buf = wrapbuffer_fd(_dma_fd, width, height, RK_FORMAT_BGR_888); // 假设是 BGR 格式，可根据实际调整
    }

    // copy constructor of vp_frame_meta would NOT be called at most time.
    // only when it flow through vp_split_node with vp_split_node::split_with_deep_copy==true.
    // in fact, all kinds of meta would NOT be copyed in its lifecycle, we just pass them by poniter most time.
    vp_frame_meta::vp_frame_meta(const vp_frame_meta& meta): 
        vp_meta(meta),
        frame_index(meta.frame_index),
        original_width(meta.original_width),
        original_height(meta.original_height),
        fps(meta.fps), 
        dma_fd(meta.dma_fd),
        rga_buf(meta.rga_buf)
        {
            // deep copy frame data
            this->frame = meta.frame.clone();
            this->osd_frame = meta.osd_frame.clone();
            this->mask = meta.mask.clone();

            // deep copy targets
            for(auto& i: meta.targets) {
                this->targets.push_back(i->clone());
            }
            // deep copy pose targets
            for(auto& i: meta.pose_targets) {
                this->pose_targets.push_back(i->clone());
            }
            // deep copy face targets
            for(auto& i: meta.face_targets) {
                this->face_targets.push_back(i->clone());
            }
            // deep copy text targets
            for(auto& i: meta.text_targets) {
                this->text_targets.push_back(i->clone());
            }
            // deep copy ba results
            for(auto& i: meta.ba_results) {
                this->ba_results.push_back(i->clone());
            }
    }
    
    vp_frame_meta::~vp_frame_meta() {

    }

    std::shared_ptr<vp_meta> vp_frame_meta::clone() {
        // just call copy constructor and return new pointer
        return std::make_shared<vp_frame_meta>(*this);
    }

    std::vector<std::shared_ptr<vp_frame_target>> vp_frame_meta::get_targets_by_ids(const std::vector<int>& ids) {
        std::vector<std::shared_ptr<vp_objects::vp_frame_target>> results;
        for(auto& t: targets) {
            if (std::find(ids.begin(), ids.end(), t->track_id) != ids.end()) {
                results.push_back(t);
            }
        }
        return results;
    }

    // helper: 将 DMA-FD buffer 映射为 CPU Mat（如果需要在 CPU 上操作）
    cv::Mat vp_frame_meta::get_cpu_mat() {
        if (!frame.empty())
            return frame; // 已经有 CPU copy
        if (dma_fd >= 0) {
            void* cpu_ptr = nullptr;
            rga_buffer_t tmp = wrapbuffer_fd(dma_fd, original_width, original_height, RK_FORMAT_BGR_888);
            cpu_ptr = malloc(original_width * original_height * 3);
            rga_buffer_t dst = wrapbuffer_virtualaddr(cpu_ptr, original_width, original_height, RK_FORMAT_BGR_888);
            imcvtcolor(tmp, dst, RK_FORMAT_BGR_888, RK_FORMAT_BGR_888); // 或根据实际格式转换
            cv::Mat mat(original_height, original_width, CV_8UC3, cpu_ptr);
            return mat;
        }
        return cv::Mat();
    }
}