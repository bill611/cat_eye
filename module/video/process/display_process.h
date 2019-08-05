#ifndef __DISPLAY_PROCESS_H_
#define __DISPLAY_PROCESS_H_

#include <CameraHal/StrmPUBase.h>

#include <rk_fb/rk_fb.h>
#include <rk_rga/rk_rga.h>

typedef void (*DecCallbackFunc)(void *data,int *size);
typedef void (*DecCallbackEndFunc)(void);
typedef void (*CapCallbackFunc)(void);

class DisplayProcess : public StreamPUBase {
 public:
    DisplayProcess();
    virtual ~DisplayProcess();

    bool processFrame(std::shared_ptr<BufferBase> input,
                            std::shared_ptr<BufferBase> output) override;
	void setVideoBlack(void);
	void showLocalVideo(void);
	void showPeerVideo(int w,int h,DecCallbackFunc decCallBack,DecCallbackEndFunc decEndCallBack);
	void capture(CapCallbackFunc capCallbackFunc,char *file_name);
    
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

	DecCallbackEndFunc decEndCallBack(void) const {
		return decEndCallBack_;
	};

	CapCallbackFunc capCallbackFunc(void) const {
		return capCallbackFunc_;
	};


 private:
    int rga_fd;
    bool start_dec_;
	int width_;
	int height_;
	DecCallbackFunc decCallback_;
	DecCallbackEndFunc decEndCallBack_;
	CapCallbackFunc capCallbackFunc_;
};


#endif // __DISPLAY_PROCESS_H_

