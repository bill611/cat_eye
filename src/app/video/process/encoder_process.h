
#ifndef _ENCODER_PROCESS_H
#define _ENCODER_PROCESS_H

#include <CameraHal/StrmPUBase.h>
#include <rkmedia/encoder.h>
#include <rkmedia/buffer.h>
#include <rkmedia/image.h>
#include <adk/mm/buffer.h>

typedef void (*EncCallbackFunc)(void *data,int size,int frame_type);
typedef void (*RecordStopCallbackFunc)(void);

class H264Encoder : public StreamPUBase {
 public:
    H264Encoder();
    virtual ~H264Encoder();

    bool processFrame(std::shared_ptr<BufferBase> inBuf,
            std::shared_ptr<BufferBase> outBuf) override;

    int startEnc(int width,int height,EncCallbackFunc encCallback);
    int stopEnc(void);
    void recordStart(EncCallbackFunc recordCallback);
    void recordSetStopFunc(RecordStopCallbackFunc recordCallback);
    void recordStop(void);
	void capture(char *file_name);

    FILE* fd(void) const {
        return fd_;
    }

	bool start_enc(void) const {
		return start_enc_;
	};

	bool start_record(void) const {
		return start_record_;
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

	EncCallbackFunc recordCallback(void) const {
		return recordCallback_;
	};

	RecordStopCallbackFunc recordStopCallback(void) const {
		return recordStopCallback_;
	};

 private:

    FILE* fd_;
    bool start_enc_;
    bool start_record_;
    bool last_frame_;
	int width_;
	int height_;
	EncCallbackFunc encCallback_;
	EncCallbackFunc recordCallback_;
	RecordStopCallbackFunc recordStopCallback_;

};


#endif // ENCODER_
