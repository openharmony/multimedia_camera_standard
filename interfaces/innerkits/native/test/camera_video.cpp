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

#include "input/camera_input.h"
#include "input/camera_manager.h"
#include "media_log.h"
#include "surface.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <securec.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::CameraStandard;

static sptr<Surface> videoSurface;
static sptr<Surface> previewSurface;
static int32_t sVideoFd = -1;

enum class mode_ {
    MODE_PREVIEW = 0,
    MODE_PHOTO
};

enum class SaveVideoMode {
    CREATE = 0,
    APPEND,
    CLOSE
};

namespace {
const std::int32_t FILE_PERMISSION_FLAG = 00766;
}

class MyCallback : public CameraManagerCallback {
public:
    void OnCameraStatusChanged(const std::string &cameraID, const CameraDeviceStatus cameraStatus) const override
    {
        MEDIA_DEBUG_LOG("OnCameraStatusChanged() is called, cameraID: %{public}s, cameraStatus: %{public}d",
                        cameraID.c_str(), cameraStatus);
        return;
    }

    void OnFlashlightStatusChanged(const std::string &cameraID, const FlashlightStatus flashStatus) const override
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

class VideoOutputCallback : public VideoCallback {
    void OnFrameStarted() const override
    {
        MEDIA_INFO_LOG("VideoOutputCallback:OnFrameStarted() is called!");
    }
    void OnFrameEnded(const int32_t frameCount) const override
    {
        MEDIA_INFO_LOG("VideoOutputCallback:OnFrameEnded() is called!, frameCount: %{public}d", frameCount);
    }
    void OnError(const int32_t errorCode) const override
    {
        MEDIA_INFO_LOG("VideoOutputCallback:OnError() is called!, errorCode: %{public}d", errorCode);
    }
};

static uint64_t GetCurrentLocalTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return tmp.count();
}

static int32_t SaveYUV(mode_ mode, const char *buffer, int32_t size)
{
    char path[PATH_MAX] = {0};
    if (mode == mode_::MODE_PREVIEW) {
        system("mkdir -p /mnt/preview");
        (void)sprintf_s(path, sizeof(path) / sizeof(path[0]),
            "/mnt/preview/%s_%lld.yuv", "preview", GetCurrentLocalTimeStamp());
    } else {
        system("mkdir -p /mnt/capture");
        (void)sprintf_s(path, sizeof(path) / sizeof(path[0]),
            "/mnt/capture/%s_%lld.jpg", "photo", GetCurrentLocalTimeStamp());
    }
    MEDIA_DEBUG_LOG("%s, saving file to %{public}s", __FUNCTION__, path);
    int imgFd = open(path, O_RDWR | O_CREAT, FILE_PERMISSION_FLAG);
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

static int32_t SaveVideoFile(const char *buffer, int32_t size, SaveVideoMode operationMode)
{
    if (operationMode == SaveVideoMode::CREATE) {
        char path[255] = {0};
        system("mkdir -p /mnt/video");
        (void)sprintf_s(path, sizeof(path) / sizeof(path[0]),
                        "/mnt/video/%s_%lld.h264", "video", GetCurrentLocalTimeStamp());
        MEDIA_DEBUG_LOG("%s, save video to file %s", __FUNCTION__, path);
        sVideoFd = open(path, O_RDWR | O_CREAT, FILE_PERMISSION_FLAG);
        if (sVideoFd == -1) {
            std::cout << "open file failed, errno = " << strerror(errno) << std::endl;
            return -1;
        }
    } else if (operationMode == SaveVideoMode::APPEND && sVideoFd != -1) {
        int32_t ret = write(sVideoFd, buffer, size);
        if (ret == -1) {
            std::cout << "write file failed, error = " << strerror(errno) << std::endl;
            close(sVideoFd);
            return -1;
        }
    } else {
        if (sVideoFd != -1) {
            close(sVideoFd);
            sVideoFd = -1;
        }
    }
    return 0;
}

class VideoSurfaceListener : public IBufferConsumerListener {
public:
    sptr<Surface> surface_;

    void OnBufferAvailable() override
    {
        if (sVideoFd == -1) {
            // Create video file
            SaveVideoFile(nullptr, 0, SaveVideoMode::CREATE);
        }
        int32_t flushFence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        MEDIA_DEBUG_LOG("VideoSurfaceListener OnBufferAvailable");
        OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
        if (buffer != nullptr) {
            char *addr = static_cast<char *>(buffer->GetVirAddr());
            int32_t size = buffer->GetSize();
            MEDIA_DEBUG_LOG("Saving to video file");
            SaveVideoFile(addr, size, SaveVideoMode::APPEND);
            surface_->ReleaseBuffer(buffer, -1);
        } else {
            MEDIA_DEBUG_LOG("AcquireBuffer failed!");
        }
    }
};

class SurfaceListener : public IBufferConsumerListener {
public:
    mode_ mode;
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
            char *addr = static_cast<char *>(buffer->GetVirAddr());
            int32_t size = buffer->GetSize();
            MEDIA_DEBUG_LOG("Calling SaveYUV");
            SaveYUV(mode, addr, size);
            surface_->ReleaseBuffer(buffer, -1);
        } else {
            MEDIA_DEBUG_LOG("AcquireBuffer failed!");
        }
    }
};

static bool IsNumber(const char number[])
{
    for (int i = 0; number[i] != 0; i++) {
        if (!std::isdigit(number[i])) {
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    const std::int32_t PREVIEW_DEFAULT_WIDTH = 640;
    const std::int32_t PREVIEW_DEFAULT_HEIGHT = 480;
    const std::int32_t VIDEO_DEFAULT_WIDTH = 1280;
    const std::int32_t VIDEO_DEFAULT_HEIGHT = 720;
    const std::int32_t PREVIEW_WIDTH_INDEX = 1;
    const std::int32_t PREVIEW_HEIGHT_INDEX = 2;
    const std::int32_t VIDEO_WIDTH_INDEX = 3;
    const std::int32_t VIDEO_HEIGHT_INDEX = 4;
    const std::int32_t VALID_ARG_COUNT = 5;
    const std::int32_t VIDEO_CAPTURE_DURATION = 10; // Sleep for 10 sec
    const std::int32_t VIDEO_PAUSE_DURATION = 5; // Sleep for 5 sec
    const std::int32_t PREVIEW_VIDEO_GAP = 2; // Sleep for 2 sec
    int32_t intResult = -1;
    // Default sizes for PreviewOutput and VideoOutput
    int32_t previewWidth = PREVIEW_DEFAULT_WIDTH;
    int32_t previewHeight = PREVIEW_DEFAULT_HEIGHT;
    int32_t videoWidth = VIDEO_DEFAULT_WIDTH;
    int32_t videoHeight = VIDEO_DEFAULT_HEIGHT;

    MEDIA_DEBUG_LOG("Camera new sample begin.");
    // Update sizes if enough number of valid arguments are passed
    if (argc == VALID_ARG_COUNT) {
        // Validate arguments and consider if valid
        for (int counter = 1; counter < argc; counter++) {
            if (!IsNumber(argv[counter])) {
                cout << "Invalid argument: " << argv[counter] << endl;
                cout << "Retry by giving proper sizes" << endl;
                return 0;
            }
        }
        previewWidth = atoi(argv[PREVIEW_WIDTH_INDEX]);
        previewHeight = atoi(argv[PREVIEW_HEIGHT_INDEX]);
        videoWidth = atoi(argv[VIDEO_WIDTH_INDEX]);
        videoHeight = atoi(argv[VIDEO_HEIGHT_INDEX]);
    }
    MEDIA_DEBUG_LOG("previewWidth: %{public}d, previewHeight: %{public}d", previewWidth, previewHeight);
    MEDIA_DEBUG_LOG("videoWidth: %{public}d, videoHeight: %{public}d", videoWidth, videoHeight);
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::shared_ptr<MyCallback> cameraMngrCallback = make_shared<MyCallback>();
    MEDIA_DEBUG_LOG("Setting callback to listen camera status and flash status");
    camManagerObj->SetCallback(cameraMngrCallback);
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    MEDIA_DEBUG_LOG("Camera ID count: %{public}zu", cameraObjList.size());
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
            ((sptr<CameraInput> &)cameraInput)->SetErrorCallback(deviceCallback);
            intResult = captureSession->AddInput(cameraInput);
            if (intResult == 0) {
                previewSurface = Surface::CreateSurfaceAsConsumer();
                previewSurface->SetDefaultWidthAndHeight(previewWidth, previewHeight);
                sptr<SurfaceListener> listener = new SurfaceListener();
                listener->mode = mode_::MODE_PREVIEW;
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
                if (intResult != 0) {
                    MEDIA_DEBUG_LOG("Failed to Add output to session, intResult: %{public}d", intResult);
                    return 0;
                }
                videoSurface = Surface::CreateSurfaceAsConsumer();
                videoSurface->SetDefaultWidthAndHeight(videoWidth, videoHeight);
                sptr<VideoSurfaceListener> videoListener = new VideoSurfaceListener();
                videoListener->surface_ = videoSurface;
                videoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)videoListener);
                sptr<CaptureOutput> videoOutput = camManagerObj->CreateVideoOutput(videoSurface);
                if (videoOutput == nullptr) {
                    MEDIA_DEBUG_LOG("Failed to create video output");
                    return 0;
                }
                MEDIA_DEBUG_LOG("Setting preview callback");
                std::shared_ptr videoCallback = std::make_shared<VideoOutputCallback>();
                ((sptr<VideoOutput> &)videoOutput)->SetCallback(videoCallback);
                intResult = captureSession->AddOutput(videoOutput);
                if (intResult != 0) {
                    MEDIA_DEBUG_LOG("Failed to Add output to session, intResult: %{public}d", intResult);
                    return 0;
                }
                intResult = captureSession->CommitConfig();
                if (intResult != 0) {
                    MEDIA_DEBUG_LOG("Failed to Commit config, intResult: %{public}d", intResult);
                    return 0;
                }
                intResult = captureSession->Start();
                if (intResult != 0) {
                    MEDIA_DEBUG_LOG("Failed to start, intResult: %{public}d", intResult);
                    return 0;
                }
                MEDIA_DEBUG_LOG("Preview started");
                sleep(PREVIEW_VIDEO_GAP);
                MEDIA_DEBUG_LOG("Start video recording");
                ((sptr<VideoOutput> &)videoOutput)->Start();
                sleep(VIDEO_CAPTURE_DURATION);
                MEDIA_DEBUG_LOG("Pause video recording for 5 sec");
                ((sptr<VideoOutput> &)videoOutput)->Pause();
                sleep(VIDEO_PAUSE_DURATION);
                MEDIA_DEBUG_LOG("Resume video recording");
                ((sptr<VideoOutput> &)videoOutput)->Resume();
                sleep(VIDEO_CAPTURE_DURATION);
                MEDIA_DEBUG_LOG("Stop video recording");
                ((sptr<VideoOutput> &)videoOutput)->Stop();
                MEDIA_DEBUG_LOG("Closing the session");
                captureSession->Stop();
                captureSession->Release();
                // Close video file
                SaveVideoFile(nullptr, 0, SaveVideoMode::CLOSE);
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
