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

#ifndef FACE_CAMERA_BUFFER_H_
#define FACE_CAMERA_BUFFER_H_

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ion/ion.h>

#include <CameraHal/CameraBuffer.h>

#include <adk/base/definition_magic.h>
#include <adk/face/face.h>
#include <adk/face/face_array.h>

namespace rk {

class FaceCameraBuffer : public CameraBuffer {
    friend class FaceCameraBufferAllocator;
 public:
    ADK_DECLARE_SHARED_PTR(FaceCameraBuffer);

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

    virtual ~FaceCameraBuffer();

    virtual int getFd() { return mShareFd;}

    virtual FaceArray::SharedPtr faces(void) const;

    virtual void set_faces(FaceArray::SharedPtr array) {
        faces_ = array;
    }

    virtual void set_image(Image::SharedPtr image);

    virtual Image::SharedPtr image(void) const;

 protected:
    FaceCameraBuffer(ion_user_handle_t handle, int sharefd, unsigned long phy, void* vaddr,
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

 private:
    Image::SharedPtr image_;
    FaceArray::SharedPtr faces_;
};

class FaceCameraBufferAllocator : public CameraBufferAllocator {
    friend class FaceCameraBuffer;
 public:
    FaceCameraBufferAllocator(void);

    virtual ~FaceCameraBufferAllocator(void);

    virtual std::shared_ptr<CameraBuffer> alloc(const char* camPixFmt, unsigned int width, unsigned int height,
                                                unsigned int usage, weak_ptr<ICameraBufferOwener> bufOwener) override;

    virtual void free(CameraBuffer* buffer) override;

 private:
    int mIonClient;
};

} // namespace rk

#endif // FACE_CAMERA_BUFFER_H
