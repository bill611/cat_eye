/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define TAG "face_service"

#include <adk/utils/assert.h>
#include <adk/utils/logger.h>

#include "face_camera_buffer.h"

using namespace rk;

FaceCameraBuffer::FaceCameraBuffer(ion_user_handle_t handle, int sharefd,
                                   unsigned long phy, void* vaddr, const char* camPixFmt,
                                   unsigned int width, unsigned int height, int stride,size_t size,
                                   weak_ptr<CameraBufferAllocator> allocator,
                                   weak_ptr<ICameraBufferOwener> bufOwener)
    : CameraBuffer(allocator, bufOwener), mHandle(handle), mShareFd(sharefd),
    mPhy(phy), mVaddr(vaddr), mWidth(width), mHeight(height), mBufferSize(size),
    mCamPixFmt(camPixFmt), mStride(stride) {
    faces_ = std::make_shared<FaceArray>();
    ASSERT(faces_.get() !=  nullptr);
}

FaceCameraBuffer::~FaceCameraBuffer(void) {
    if (!mAllocator.expired()) {

        shared_ptr<CameraBufferAllocator> spAlloc = mAllocator.lock();

        if (spAlloc.get()) {
            cout<<"line "<<__LINE__<<" func:"<<__func__<<"\n";
            spAlloc->free(this);
        }
    }
}

void* FaceCameraBuffer::getHandle(void) const {
    return (void*)&mHandle;
}

void* FaceCameraBuffer::getPhyAddr(void) const {
    return (void*)mPhy;
}

void* FaceCameraBuffer::getVirtAddr(void) const {
    return mVaddr;
}

const char* FaceCameraBuffer::getFormat(void) {
    return mCamPixFmt;
}

unsigned int FaceCameraBuffer::getWidth(void) {
    return mWidth;
}

unsigned int FaceCameraBuffer::getHeight(void) {
    return mHeight;
}

size_t FaceCameraBuffer::getDataSize(void) const {
    return mDataSize;
}

void FaceCameraBuffer::setDataSize(size_t size) {
    mDataSize = size;
}

size_t FaceCameraBuffer::getCapacity(void) const {
    return mBufferSize;
}

unsigned int FaceCameraBuffer::getStride(void) const {
    return mStride;
}

bool FaceCameraBuffer::lock(unsigned int usage) {
    UNUSED_PARAM(usage);
    return true;
}

bool FaceCameraBuffer::unlock(unsigned int usage) {
    UNUSED_PARAM(usage);
    return true;
}

FaceArray::SharedPtr FaceCameraBuffer::faces(void) const {
    return faces_;
}

void FaceCameraBuffer::set_image(Image::SharedPtr image) {
    image_ = image;
}

Image::SharedPtr FaceCameraBuffer::image(void) const {
    return image_;
}

FaceCameraBufferAllocator::FaceCameraBufferAllocator(void) {
    mIonClient = ion_open();
    if (mIonClient < 0) {
        pr_err("open /dev/ion failed!\n");
        mError = true;
    }
}

FaceCameraBufferAllocator::~FaceCameraBufferAllocator(void) {
    ion_close(mIonClient);
    if (mNumBuffersAllocated > 0)
        pr_err("%s: memory leak; %d camera buffers have not been freed", __func__, mNumBuffersAllocated);
}

std::shared_ptr<CameraBuffer>
    FaceCameraBufferAllocator::alloc(const char* camPixFmt, unsigned int width, unsigned int height,
                                     unsigned int usage, weak_ptr<ICameraBufferOwener> bufOwener)
{
    int ret;
    ion_user_handle_t buffHandle;
    int stride;
    int sharefd;
    void* vaddr;
    size_t buffer_size;
    shared_ptr<FaceCameraBuffer> camBuff;
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
        pr_err("%s: ion buffer allocation failed (error %d)", __func__, ret);
        goto alloc_end;
    }

    if (type == ION_HEAP_TYPE_DMA_MASK)
        ion_get_phys(mIonClient, buffHandle, &phy);

    vaddr = mmap(NULL, buffer_size, ionFlags, MAP_SHARED, sharefd, 0);

    camBuff = shared_ptr<FaceCameraBuffer> (new FaceCameraBuffer(buffHandle, sharefd, phy, vaddr, camPixFmt, width, height, stride,
                                                             buffer_size, shared_from_this(), bufOwener));
    if (!camBuff.get()) {
        pr_err("%s: Out of memory", __func__);
    } else {
        mNumBuffersAllocated++;
        if (camBuff->error()) {
        }
    }
alloc_end:
    return camBuff;
}

void FaceCameraBufferAllocator::free(CameraBuffer* buffer) {
    int ret;
    if (buffer) {
        FaceCameraBuffer* camBuff = static_cast<FaceCameraBuffer*>(buffer);
        munmap(camBuff->mVaddr, camBuff->mBufferSize);
        close(camBuff->mShareFd);
        ret = ion_free(mIonClient, camBuff->mHandle);
        if (ret != 0)
            pr_err("%s: ion free buffer failed (error %d)", __func__, ret);
        mNumBuffersAllocated--;
    }
}
