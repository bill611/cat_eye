#ifndef _CAMERA_BUF_H_
#define _CAMERA_BUF_H_

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ion/ion.h>
#include <list>

#include <CameraHal/CameraBuffer.h>

class RKCameraBuffer : public CameraBuffer {
    friend class RKCameraBufferAllocator;
 public:
	std::list<shared_ptr<RKCameraBuffer>> camhals;

    virtual void* getHandle(void) const;

    virtual void* getPhyAddr(void) const;

    virtual void* getVirtAddr(void) const;

    virtual const char* getFormat(void);

    virtual unsigned int getWidth(void);

    virtual unsigned int getHeight(void);

    virtual size_t getDataSize(void) const;

    virtual void setDataSize(size_t size);

    virtual size_t getCapacity(void) const;

    virtual unsigned int getStride(void) const;

    virtual bool lock(unsigned int usage = CameraBuffer::READ);

    virtual bool unlock(unsigned int usage = CameraBuffer::READ);

    virtual ~RKCameraBuffer();

    virtual int getFd() { return mShareFd;}

 protected:
    RKCameraBuffer(ion_user_handle_t handle, int sharefd, unsigned long phy, void* vaddr,
                     const char* camPixFmt, unsigned int width, unsigned int height, int stride, size_t size,
                     std::weak_ptr<CameraBufferAllocator> allocator, std::weak_ptr<ICameraBufferOwener> bufOwener);

    ion_user_handle_t mHandle;
    int mShareFd;
    void* mVaddr;
    unsigned int mWidth;
    unsigned int mHeight;
    size_t mBufferSize;
    const char* mCamPixFmt;
    unsigned int mStride;
    size_t mDataSize;
    unsigned long mPhy;

};

class RKCameraBufferAllocator : public CameraBufferAllocator {
    friend class RKCameraBuffer;
 public:
    RKCameraBufferAllocator(void);

    virtual ~RKCameraBufferAllocator(void);

    virtual std::shared_ptr<CameraBuffer> alloc(const char* camPixFmt, unsigned int width, unsigned int height,
                                                unsigned int usage, weak_ptr<ICameraBufferOwener> bufOwener) override;

    virtual void free(CameraBuffer* buffer) override;

 private:
    int mIonClient;
};

#endif // FACE_CAMERA_BUFFER_H

