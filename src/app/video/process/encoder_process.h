
#ifndef _ENCODER_PROCESS_H
#define _ENCODER_PROCESS_H

#include <CameraHal/StrmPUBase.h>
#include <rkmedia/encoder.h>
#include <rkmedia/buffer.h>
#include <rkmedia/image.h>
#include <adk/mm/buffer.h>
#include "queue.h"

class H264Encoder : public StreamPUBase {
 public:
    H264Encoder(int frame_width, int frame_height);
    H264Encoder();
    virtual ~H264Encoder();

    bool processFrame(std::shared_ptr<BufferBase> inBuf,
            std::shared_ptr<BufferBase> outBuf) override;

    int Start(void);
    int StartYuv(void);
    void Reset(void);

    bool init(int width, int height);

    FILE* fd(void) const {
        return fd_;
    }

	ImageInfo* image_info(void) const {
        return image_info_;
    }

	rk::Buffer::SharedPtr encoder_src(void) const {
        return encoder_src_;
    }

	rk::Buffer::SharedPtr encoder_dst(void) const  {
        return encoder_dst_;
    }

    std::shared_ptr<rkmedia::VideoEncoder> encoder(void) const {
        return encoder_;
    }

	Queue *queue(void) const {
		return queue_;
	};
 private:

    FILE* fd_;
    bool is_working_;
	Queue *queue_;
    ImageInfo* image_info_;
    rk::Buffer::SharedPtr encoder_src_;
    rk::Buffer::SharedPtr encoder_dst_;

    std::shared_ptr<rkmedia::VideoEncoder> encoder_;
};


#endif // ENCODER_
