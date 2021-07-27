/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "input/camera_manager.h"
#include "input/camera_input.h"
#include "media_log.h"
#include "surface.h"
#include <securec.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::CameraStandard;

static sptr<Surface> previewSurface;
static sptr<Surface> photoSurface;

enum mode_ {
    MODE_PREVIEW = 0,
    MODE_PHOTO
};

class MyCallback : public CameraManagerCallback {
public:
    void OnCameraStatusChanged(const std::string cameraID, const CameraDeviceStatus cameraStatus) const override
    {
        MEDIA_DEBUG_LOG("OnCameraStatusChanged() is called, cameraID: %{public}s, cameraStatus: %{public}d",
                        cameraID.c_str(), cameraStatus);
        return;
    }

    void OnFlashlightStatusChanged(const std::string cameraID, const FlashlightStatus flashStatus) const override
    {
        MEDIA_DEBUG_LOG("OnFlashlightStatusChanged() is called cameraID: %{public}s, flashStatus: %{public}d",
                        cameraID.c_str(), flashStatus);
        return;
    }
};

class MyDeviceCallback : public ErrorCallback {
public:
    void OnError(const int32_t errorType, const int32_t errorMsg) const override
    {
        MEDIA_DEBUG_LOG("OnError() is called, errorType: %{public}d, errorMsg: %{public}d", errorType, errorMsg);
        return;
    }
};

class PhotoOutputCallback : public PhotoCallback {
    void OnCaptureStarted(const int32_t captureID) const override
    {
        MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureStarted() is called!, captureID: %{public}d", captureID);
    }

    void OnCaptureEnded(const int32_t captureID) const override
    {
        MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureEnded() is called!, captureID: %{public}d", captureID);
    }

    void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const override
    {
        MEDIA_INFO_LOG("PhotoOutputCallback:OnFrameShutter() called!, captureID: %{public}d, timestamp: %{public}llu",
                       captureId, timestamp);
    }

    void OnCaptureError(const int32_t captureId, const int32_t errorCode) const override
    {
        MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureError() is called!, captureID: %{public}d, errorCode: %{public}d",
                       captureId, errorCode);
    }
};

class PreviewOutputCallback : public PreviewCallback {

    void OnFrameStarted() const override
    {
        MEDIA_INFO_LOG("PreviewOutputCallback:OnFrameStarted() is called!");
    }
    void OnFrameEnded(const int32_t frameCount) const override
    {
        MEDIA_INFO_LOG("PreviewOutputCallback:OnFrameEnded() is called!, frameCount: %{public}d", frameCount);
    }
    void OnError(const int32_t errorCode) const override
    {
        MEDIA_INFO_LOG("PreviewOutputCallback:OnError() is called!, errorCode: %{public}d", errorCode);
    }
};

uint64_t GetCurrentLocalTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return tmp.count();
}

int32_t SaveYUV(int32_t mode, void* buffer, int32_t size)
{
    char path[PATH_MAX] = {0};
    if (mode == MODE_PREVIEW) {
        system("mkdir -p /mnt/preview");
        sprintf_s(path, sizeof(path) / sizeof(path[0]), "/mnt/preview/%s_%lld.yuv", "preview", GetCurrentLocalTimeStamp());
    } else {
        system("mkdir -p /mnt/capture");
        sprintf_s(path, sizeof(path) / sizeof(path[0]), "/mnt/capture/%s_%lld.jpg", "photo", GetCurrentLocalTimeStamp());
    }
    MEDIA_DEBUG_LOG("%s, saving file to %{public}s", __FUNCTION__, path);
    int imgFd = open(path, O_RDWR | O_CREAT, 00766);
    if (imgFd == -1) {
        MEDIA_DEBUG_LOG("%s, open file failed, errno = %{public}s.", __FUNCTION__, strerror(errno));
        return -1;
    }
    int ret = write(imgFd, buffer, size);
    if (ret == -1) {
        MEDIA_DEBUG_LOG("%s, write file failed, error = %{public}s", __FUNCTION__, strerror(errno));
        close(imgFd);
        return -1;
    }
    close(imgFd);
    return 0;
}

class SurfaceListener : public IBufferConsumerListener {
public:
    int32_t mode_;
    sptr<Surface> surface_;

    void OnBufferAvailable() override
    {
        int32_t flushFence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        MEDIA_DEBUG_LOG("SurfaceListener OnBufferAvailable");
        OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
        if (buffer != nullptr) {
            void *addr = buffer->GetVirAddr();
            int32_t size = buffer->GetSize();
            MEDIA_DEBUG_LOG("Calling SaveYUV");
            SaveYUV(mode_, addr, size);
            surface_->ReleaseBuffer(buffer, -1);
        } else {
            MEDIA_DEBUG_LOG("AcquireBuffer failed!");
        }
    }
};

class CaptureSurfaceListener : public IBufferConsumerListener {
public:
    int32_t mode_;
    sptr<Surface> surface_;

    void OnBufferAvailable() override
    {
        int32_t flushFence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        MEDIA_DEBUG_LOG("CaptureSurfaceListener OnBufferAvailable");
        OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
        if (buffer != nullptr) {
            void *addr = buffer->GetVirAddr();
            int32_t size = buffer->GetSize();
            MEDIA_DEBUG_LOG("Saving Image");
            SaveYUV(mode_, addr, size);
            surface_->ReleaseBuffer(buffer, -1);
        } else {
            MEDIA_DEBUG_LOG("AcquireBuffer failed!");
        }
    }
};

int main()
{
    int32_t intResult = -1;
    MEDIA_DEBUG_LOG("Camera new sample begin.");

    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::shared_ptr<MyCallback> cameraMngrCallback = make_shared<MyCallback>();
    MEDIA_DEBUG_LOG("Setting callback to listen camera status and flash status");
    camManagerObj->SetCallback(cameraMngrCallback);
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    MEDIA_DEBUG_LOG("Camera ID count: %{public}d", cameraObjList.size());
    for (auto& it : cameraObjList) {
        MEDIA_DEBUG_LOG("Camera ID: %{public}s", it->GetID().c_str());
    }
    if (cameraObjList.size() > 0) {
        sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
        if (captureSession == nullptr) {
            MEDIA_DEBUG_LOG("Failed to create capture session");
        }
        captureSession->BeginConfig();
        sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
        if (cameraInput != nullptr) {
            std::shared_ptr<MyDeviceCallback> deviceCallback = make_shared<MyDeviceCallback>();
            ((sptr<CameraInput> &) cameraInput)->SetErrorCallback(deviceCallback);
            intResult = captureSession->AddInput(cameraInput);
            if (0 == intResult) {
                photoSurface = Surface::CreateSurfaceAsConsumer();
                sptr<CaptureSurfaceListener> capturelistener = new CaptureSurfaceListener();
                capturelistener->mode_ = MODE_PHOTO;
                capturelistener->surface_ = photoSurface;
                photoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)capturelistener);
                sptr<CaptureOutput> photoOutput = camManagerObj->CreatePhotoOutput(photoSurface);
                if (photoOutput == nullptr) {
                    MEDIA_DEBUG_LOG("Failed to create PhotoOutput");
                    return 0;
                }
                MEDIA_DEBUG_LOG("Setting photo callback");
                std::shared_ptr photoCallback = std::make_shared<PhotoOutputCallback>();
                ((sptr<PhotoOutput> &)photoOutput)->SetCallback(photoCallback);
                intResult = captureSession->AddOutput(photoOutput);
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to Add output to session, intResult: %{public}d", intResult);
                    return 0;
                }
                previewSurface = Surface::CreateSurfaceAsConsumer();
                sptr<SurfaceListener> listener = new SurfaceListener();
                listener->mode_ = MODE_PREVIEW;
                listener->surface_ = previewSurface;
                previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
                sptr<CaptureOutput> previewOutput = camManagerObj->CreatePreviewOutput(previewSurface);
                if (previewOutput == nullptr) {
                    MEDIA_DEBUG_LOG("Failed to create previewOutput");
                    return 0;
                }
                MEDIA_DEBUG_LOG("Setting preview callback");
                std::shared_ptr previewCallback = std::make_shared<PreviewOutputCallback>();
                ((sptr<PreviewOutput> &)previewOutput)->SetCallback(previewCallback);
                intResult = captureSession->AddOutput(previewOutput);
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to Add output to session, intResult: %{public}d", intResult);
                    return 0;
                }
                intResult = captureSession->CommitConfig();
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to Commit config, intResult: %{public}d", intResult);
                    return 0;
                }
                intResult = captureSession->Start();
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to start, intResult: %{public}d", intResult);
                    return 0;
                }
                MEDIA_DEBUG_LOG("Preview started");
                MEDIA_DEBUG_LOG("Waiting for 5 seconds begin");
                sleep(5);
                MEDIA_DEBUG_LOG("Waiting for 5 seconds end");
                MEDIA_DEBUG_LOG("Photo capture started");
                intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to capture, intResult: %{public}d", intResult);
                    return 0;
                }
                MEDIA_DEBUG_LOG("Waiting for 2 seconds begin");
                sleep(2);
                MEDIA_DEBUG_LOG("Waiting for 2 seconds end");
                MEDIA_DEBUG_LOG("Closing the session");
                captureSession->Stop();
                captureSession->Release();
                camManagerObj->SetCallback(nullptr);
            } else {
                MEDIA_DEBUG_LOG("Add input to session is failed, intResult: %{public}d", intResult);
            }
        } else {
            MEDIA_DEBUG_LOG("Failed to create Camera Input");
        }
    }
    MEDIA_DEBUG_LOG("Camera new sample end.");
    return 0;
}
