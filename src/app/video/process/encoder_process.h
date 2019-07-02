
#ifndef _ENCODER_PROCESS_H
#define _ENCODER_PROCESS_H

#include <CameraHal/StrmPUBase.h>
#include <rkmedia/encoder.h>
#include <rkmedia/buffer.h>
#include <rkmedia/image.h>
#include <adk/mm/buffer.h>

typedef void (*EncCallbackFunc)(void *data,int size);
class H264Encoder : public StreamPUBase {
 public:
    H264Encoder();
    virtual ~H264Encoder();

    bool processFrame(std::shared_ptr<BufferBase> inBuf,
            std::shared_ptr<BufferBase> outBuf) override;

    int startEnc(int width,int height,EncCallbackFunc encCallback);
    int stopEnc(void);

    FILE* fd(void) const {
        return fd_;
    }

	bool start_enc(void) const {
		return start_enc_;
	};

	int getWidth(void) const {
		return width_;
	}
	int getHeight(void) const {
		return height_;
	}

	EncCallbackFunc encCallback(void) const {
		return encCallback_;
	};

 private:

    FILE* fd_;
    bool start_enc_;
    bool last_frame_;
	int width_;
	int height_;
	EncCallbackFunc encCallback_;

};


#endif // ENCODER_
