#ifndef __DISPLAY_PROCESS_H_
#define __DISPLAY_PROCESS_H_

#include <CameraHal/StrmPUBase.h>

#include <rk_fb/rk_fb.h>
#include <rk_rga/rk_rga.h>

typedef void (*DecCallbackFunc)(void **data,int *size);

class DisplayProcess : public StreamPUBase {
 public:
    DisplayProcess();
    virtual ~DisplayProcess();

    bool processFrame(std::shared_ptr<BufferBase> input,
                            std::shared_ptr<BufferBase> output) override;
	void setVideoBlack(void);
	void showLocalVideo(void);
	void showPeerVideo(int w,int h,DecCallbackFunc decCallBack);
    
	bool start_dec(void) const {
		return start_dec_;
	};

	int getWidth(void) const {
		return width_;
	}
	int getHeight(void) const {
		return height_;
	}

	DecCallbackFunc decCallback(void) const {
		return decCallback_;
	};

 private:
    int rga_fd;
    bool start_dec_;
    bool last_frame_;
	int width_;
	int height_;
	DecCallbackFunc decCallback_;
};


#endif // __DISPLAY_PROCESS_H_

