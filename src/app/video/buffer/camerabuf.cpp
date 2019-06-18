#include "camerabuf.h"


RKCameraBuffer::RKCameraBuffer(ion_user_handle_t handle, int sharefd,
                                   unsigned long phy, void* vaddr, const char* camPixFmt,
                                   unsigned int width, unsigned int height, int stride,size_t size,
                                   weak_ptr<CameraBufferAllocator> allocator,
                                   weak_ptr<ICameraBufferOwener> bufOwener)
    : CameraBuffer(allocator, bufOwener), mHandle(handle), mShareFd(sharefd),
    mPhy(phy), mVaddr(vaddr), mWidth(width), mHeight(height), mBufferSize(size),
    mCamPixFmt(camPixFmt), mStride(stride) {
}

RKCameraBuffer::~RKCameraBuffer(void) {
    if (!mAllocator.expired()) {

        shared_ptr<CameraBufferAllocator> spAlloc = mAllocator.lock();

        if (spAlloc.get()) {
            cout<<"line "<<__LINE__<<" func:"<<__func__<<"\n";
            spAlloc->free(this);
        }
    }
}

void* RKCameraBuffer::getHandle(void) const {
    return (void*)&mHandle;
}

void* RKCameraBuffer::getPhyAddr(void) const {
    return (void*)mPhy;
}

void* RKCameraBuffer::getVirtAddr(void) const {
    return mVaddr;
}

const char* RKCameraBuffer::getFormat(void) {
    return mCamPixFmt;
}

unsigned int RKCameraBuffer::getWidth(void) {
    return mWidth;
}

unsigned int RKCameraBuffer::getHeight(void) {
    return mHeight;
}

size_t RKCameraBuffer::getDataSize(void) const {
    return mDataSize;
}

void RKCameraBuffer::setDataSize(size_t size) {
    mDataSize = size;
}

size_t RKCameraBuffer::getCapacity(void) const {
    return mBufferSize;
}

unsigned int RKCameraBuffer::getStride(void) const {
    return mStride;
}

bool RKCameraBuffer::lock(unsigned int usage) {
    UNUSED_PARAM(usage);
    return true;
}

bool RKCameraBuffer::unlock(unsigned int usage) {
    UNUSED_PARAM(usage);
    return true;
}



RKCameraBufferAllocator::RKCameraBufferAllocator(void) {
    mIonClient = ion_open();
    if (mIonClient < 0) {
        printf("open /dev/ion failed!\n");
        mError = true;
    }
}

RKCameraBufferAllocator::~RKCameraBufferAllocator(void) {
    ion_close(mIonClient);
    if (mNumBuffersAllocated > 0) {
        printf("%s: memory leak; %d camera buffers have not been freed", __func__, mNumBuffersAllocated);
    }
}

std::shared_ptr<CameraBuffer>
    RKCameraBufferAllocator::alloc(const char* camPixFmt, unsigned int width, unsigned int height,
                                     unsigned int usage, weak_ptr<ICameraBufferOwener> bufOwener)
{
    int ret;
    ion_user_handle_t buffHandle;
    int stride;
    int sharefd;
    void* vaddr;
    size_t buffer_size;
    shared_ptr<RKCameraBuffer> camBuff;
    unsigned int ionFlags = 0;
    unsigned int halPixFmt;
    unsigned long phy = 0;
    int type = ION_HEAP_TYPE_DMA_MASK;

    if (usage & CameraBuffer::WRITE)
        ionFlags |= PROT_WRITE;
    if (usage & CameraBuffer::READ)
        ionFlags |= PROT_READ;

    buffer_size = calcBufferSize(camPixFmt, (width + 0xf) & ~0xf, (height + 0xf) & ~0xf);
    stride = width;
    ret = ion_alloc(mIonClient, buffer_size, 0, type, 0, &buffHandle);
    ret = ion_share(mIonClient, buffHandle, &sharefd);
    if (ret != 0) {
        printf("%s: ion buffer allocation failed (error %d)", __func__, ret);
        goto alloc_end;
    }

    if (type == ION_HEAP_TYPE_DMA_MASK)
        ion_get_phys(mIonClient, buffHandle, &phy);

    vaddr = mmap(NULL, buffer_size, ionFlags, MAP_SHARED, sharefd, 0);

    camBuff = shared_ptr<RKCameraBuffer> (new RKCameraBuffer(buffHandle, sharefd, phy, vaddr, camPixFmt, width, height, stride,
                                                             buffer_size, shared_from_this(), bufOwener));
    if (!camBuff.get()) {
        printf("%s: Out of memory", __func__);
    } else {
        mNumBuffersAllocated++;
        if (camBuff->error()) {
        }
    }
alloc_end:
    return camBuff;
}

void RKCameraBufferAllocator::free(CameraBuffer* buffer) {
    int ret;
    if (buffer) {
        RKCameraBuffer* camBuff = static_cast<RKCameraBuffer*>(buffer);
        munmap(camBuff->mVaddr, camBuff->mBufferSize);
        close(camBuff->mShareFd);
        ret = ion_free(mIonClient, camBuff->mHandle);
        if (ret != 0) {
            printf("%s: ion free buffer failed (error %d)", __func__, ret);
        }
        mNumBuffersAllocated--;
    }
}

