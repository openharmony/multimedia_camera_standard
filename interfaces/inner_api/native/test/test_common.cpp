/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test_common.h"
#include <cstdio>
#include <fcntl.h>
#include <securec.h>
#include <sys/time.h>
#include <unistd.h>
#include "camera_util.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
uint64_t TestUtils::GetCurrentLocalTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return tmp.count();
}

int32_t TestUtils::SaveYUV(const char *buffer, int32_t size, SurfaceType type)
{
    char path[PATH_MAX] = {0};
    int32_t retVal;

    if ((buffer == nullptr) || (size == 0)) {
        MEDIA_ERR_LOG("buffer is null or size is 0");
        return -1;
    }

    MEDIA_DEBUG_LOG("TestUtils::SaveYUV(), type: %{public}d", type);
    if (type == SurfaceType::PREVIEW) {
        (void)system("mkdir -p /data/media/preview");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]), "/data/media/preview/%s_%lld.yuv", "preview",
                           GetCurrentLocalTimeStamp());
        if (retVal < 0) {
            MEDIA_ERR_LOG("Path Assignment failed");
            return -1;
        }
    } else if (type == SurfaceType::PHOTO) {
        (void)system("mkdir -p /data/media/photo");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]), "/data/media/photo/%s_%lld.jpg", "photo",
                           GetCurrentLocalTimeStamp());
        if (retVal < 0) {
            MEDIA_ERR_LOG("Path Assignment failed");
            return -1;
        }
    } else if (type == SurfaceType::SECOND_PREVIEW) {
        (void)system("mkdir -p /data/media/preview2");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]), "/data/media/preview2/%s_%lld.yuv", "preview2",
                           GetCurrentLocalTimeStamp());
        if (retVal < 0) {
            MEDIA_ERR_LOG("Path Assignment failed");
            return -1;
        }
    } else {
        MEDIA_ERR_LOG("Unexpected flow!");
        return -1;
    }

    MEDIA_DEBUG_LOG("%s, saving file to %{private}s", __FUNCTION__, path);
    int imgFd = open(path, O_RDWR | O_CREAT, FILE_PERMISSIONS_FLAG);
    if (imgFd == -1) {
        MEDIA_ERR_LOG("%s, open file failed, errno = %{public}s.", __FUNCTION__, strerror(errno));
        return -1;
    }
    int ret = write(imgFd, buffer, size);
    if (ret == -1) {
        MEDIA_ERR_LOG("%s, write file failed, error = %{public}s", __FUNCTION__, strerror(errno));
        close(imgFd);
        return -1;
    }
    close(imgFd);
    return 0;
}

bool TestUtils::IsNumber(const char number[])
{
    for (int i = 0; number[i] != 0; i++) {
        if (!std::isdigit(number[i])) {
            return false;
        }
    }
    return true;
}

int32_t TestUtils::SaveVideoFile(const char *buffer, int32_t size, VideoSaveMode operationMode, int32_t &fd)
{
    int32_t retVal = 0;

    if (operationMode == VideoSaveMode::CREATE) {
        char path[255] = {0};

        (void)system("mkdir -p /data/media/video");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]),
                           "/data/media/video/%s_%lld.h264", "video", GetCurrentLocalTimeStamp());
        if (retVal < 0) {
            MEDIA_ERR_LOG("Failed to create video file name");
            return -1;
        }
        MEDIA_DEBUG_LOG("%{public}s, save video to file %{private}s", __FUNCTION__, path);
        fd = open(path, O_RDWR | O_CREAT, FILE_PERMISSIONS_FLAG);
        if (fd == -1) {
            std::cout << "open file failed, errno = " << strerror(errno) << std::endl;
            return -1;
        }
    } else if (operationMode == VideoSaveMode::APPEND && fd != -1) {
        int32_t ret = write(fd, buffer, size);
        if (ret == -1) {
            std::cout << "write file failed, error = " << strerror(errno) << std::endl;
            close(fd);
            fd = -1;
            return fd;
        }
    } else { // VideoSaveMode::CLOSE
        if (fd != -1) {
            close(fd);
            fd = -1;
        }
    }
    return 0;
}

TestCameraMngerCallback::TestCameraMngerCallback(const char *testName) : testName_(testName) {
}

void TestCameraMngerCallback::OnCameraStatusChanged(const CameraStatusInfo &cameraStatusInfo) const
{
    MEDIA_DEBUG_LOG("OnCameraStatusChanged()");
    return;
}

void TestCameraMngerCallback::OnFlashlightStatusChanged(const std::string &cameraID,
                                                        const FlashlightStatus flashStatus) const
{
    MEDIA_DEBUG_LOG("OnFlashlightStatusChanged(), testName_: %{public}s, cameraID: %{public}s, flashStatus: %{public}d",
                    testName_, cameraID.c_str(), flashStatus);
    return;
}

TestDeviceCallback::TestDeviceCallback(const char *testName) : testName_(testName) {
}

void TestDeviceCallback::OnError(const int32_t errorType, const int32_t errorMsg) const
{
    MEDIA_DEBUG_LOG("TestDeviceCallback::OnError(), testName_: %{public}s, errorType: %{public}d, errorMsg: %{public}d",
                    testName_, errorType, errorMsg);
    return;
}

TestPhotoOutputCallback::TestPhotoOutputCallback(const char *testName) : testName_(testName) {
}

void TestPhotoOutputCallback::OnCaptureStarted(const int32_t captureID) const
{
    MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureStarted(), testName_: %{public}s, captureID: %{public}d",
                   testName_, captureID);
}

void TestPhotoOutputCallback::OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const
{
    MEDIA_INFO_LOG("TestPhotoOutputCallback:OnCaptureEnded(), testName_: %{public}s, captureID: %{public}d,"
                   " frameCount: %{public}d", testName_, captureID, frameCount);
}

void TestPhotoOutputCallback::OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_INFO_LOG("OnFrameShutter(), testName_: %{public}s, captureID: %{public}d", testName_, captureId);
}

void TestPhotoOutputCallback::OnCaptureError(const int32_t captureId, const int32_t errorCode) const
{
    MEDIA_INFO_LOG("OnCaptureError(), testName_: %{public}s, captureID: %{public}d, errorCode: %{public}d",
                   testName_, captureId, errorCode);
}

TestPreviewOutputCallback::TestPreviewOutputCallback(const char *testName) : testName_(testName) {
}

void TestPreviewOutputCallback::OnFrameStarted() const
{
    MEDIA_INFO_LOG("TestPreviewOutputCallback:OnFrameStarted(), testName_: %{public}s", testName_);
}
void TestPreviewOutputCallback::OnFrameEnded(const int32_t frameCount) const
{
    MEDIA_INFO_LOG("TestPreviewOutputCallback:OnFrameEnded(), testName_: %{public}s, frameCount: %{public}d",
                   testName_, frameCount);
}
void TestPreviewOutputCallback::OnError(const int32_t errorCode) const
{
    MEDIA_INFO_LOG("TestPreviewOutputCallback:OnError(), testName_: %{public}s, errorCode: %{public}d",
                   testName_, errorCode);
}

TestVideoOutputCallback::TestVideoOutputCallback(const char *testName) : testName_(testName) {
}

void TestVideoOutputCallback::OnFrameStarted() const
{
    MEDIA_INFO_LOG("TestVideoOutputCallback:OnFrameStarted(), testName_: %{public}s", testName_);
}

void TestVideoOutputCallback::OnFrameEnded(const int32_t frameCount) const
{
    MEDIA_INFO_LOG("TestVideoOutputCallback:OnFrameEnded(), testName_: %{public}s, frameCount: %{public}d",
                   testName_, frameCount);
}

void TestVideoOutputCallback::OnError(const int32_t errorCode) const
{
    MEDIA_INFO_LOG("TestVideoOutputCallback:OnError(), testName_: %{public}s, errorCode: %{public}d",
                   testName_, errorCode);
}

SurfaceListener::SurfaceListener(const char *testName, SurfaceType type, int32_t &fd, sptr<Surface> surface)
    : testName_(testName), surfaceType_(type), fd_(fd), surface_(surface) {
}

void SurfaceListener::OnBufferAvailable()
{
    int32_t flushFence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    MEDIA_DEBUG_LOG("SurfaceListener::OnBufferAvailable(), testName_: %{public}s, surfaceType_: %{public}d",
                    testName_, surfaceType_);
    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    if (surface_ == nullptr) {
        MEDIA_ERR_LOG("OnBufferAvailable:surface_ is null");
        return;
    }
    surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
    if (buffer != nullptr) {
        char *addr = static_cast<char *>(buffer->GetVirAddr());
        int32_t size = buffer->GetSize();

        switch (surfaceType_) {
            case SurfaceType::PREVIEW:
                if (previewIndex_ % TestUtils::PREVIEW_SKIP_FRAMES == 0
                    && TestUtils::SaveYUV(addr, size, surfaceType_) != CAMERA_OK) {
                    MEDIA_ERR_LOG("Failed to save buffer");
                    previewIndex_ = 0;
                }
                previewIndex_++;
                break;

            case SurfaceType::SECOND_PREVIEW:
                if (secondPreviewIndex_ % TestUtils::PREVIEW_SKIP_FRAMES == 0
                    && TestUtils::SaveYUV(addr, size, surfaceType_) != CAMERA_OK) {
                    MEDIA_ERR_LOG("Failed to save buffer");
                    secondPreviewIndex_ = 0;
                }
                secondPreviewIndex_++;
                break;

            case SurfaceType::PHOTO:
                if (TestUtils::SaveYUV(addr, size, surfaceType_) != CAMERA_OK) {
                    MEDIA_ERR_LOG("Failed to save buffer");
                }
                break;

            case SurfaceType::VIDEO:
                if (fd_ == -1 && (TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CREATE, fd_) != CAMERA_OK)) {
                    MEDIA_ERR_LOG("Failed to Create video file");
                }
                if (TestUtils::SaveVideoFile(addr, size, VideoSaveMode::APPEND, fd_) != CAMERA_OK) {
                    MEDIA_ERR_LOG("Failed to save buffer");
                }
                break;

            default:
                MEDIA_ERR_LOG("Unexpected type");
                break;
        }
        surface_->ReleaseBuffer(buffer, -1);
    } else {
        MEDIA_ERR_LOG("AcquireBuffer failed!");
    }
}
} // namespace CameraStandard
} // namespace OHOS

