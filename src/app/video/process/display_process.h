#ifndef __DISPLAY_PROCESS_H_
#define __DISPLAY_PROCESS_H_

#include <CameraHal/StrmPUBase.h>

#include <rk_fb/rk_fb.h>
#include <rk_rga/rk_rga.h>


class DisplayProcess : public StreamPUBase {
 public:
    DisplayProcess();
    virtual ~DisplayProcess();

    bool processFrame(std::shared_ptr<BufferBase> input,
                            std::shared_ptr<BufferBase> output) override;
	void setVideoBlack(void);
	void showLocalVideo(void);
	void showPeerVideo(int w,int h);
 private:
    int rga_fd;
};


#endif // __DISPLAY_PROCESS_H_

