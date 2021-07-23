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

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "camera_framework_test.h"
#include "input/camera_input.h"
#include "input/camera_manager.h"
#include <securec.h>
#include <stdio.h>
#include "surface.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace OHOS;
using namespace OHOS::CameraStandard;
using namespace testing::ext;

void CameraFrameworkTest::SetUpTestCase(void) {}
void CameraFrameworkTest::TearDownTestCase(void) {}

void CameraFrameworkTest::SetUp() {}
void CameraFrameworkTest::TearDown() {}

static sptr<Surface> previewSurface;
static sptr<Surface> photoSurface;

enum mode_ {
    MODE_PREVIEW = 0,
    MODE_PHOTO
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
    // MEDIA_DEBUG_LOG("%s, saving file to %{public}s", __FUNCTION__, path);
    int imgFd = open(path, O_RDWR | O_CREAT, 00766);
    if (imgFd == -1) {
        // MEDIA_DEBUG_LOG("%s, open file failed, errno = %{public}s.", __FUNCTION__, strerror(errno));
        return -1;
    }
    int ret = write(imgFd, buffer, size);
    if (ret == -1) {
        // MEDIA_DEBUG_LOG("%s, write file failed, error = %{public}s", __FUNCTION__, strerror(errno));
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
        // MEDIA_DEBUG_LOG("SurfaceListener OnBufferAvailable");
        OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
        if (buffer != nullptr) {
            void *addr = buffer->GetVirAddr();
            int32_t size = buffer->GetSize();
            // MEDIA_DEBUG_LOG("Calling SaveYUV");
            SaveYUV(mode_, addr, size);
            surface_->ReleaseBuffer(buffer, -1);
        } else {
            // MEDIA_DEBUG_LOG("AcquireBuffer failed!");
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
        // MEDIA_DEBUG_LOG("CaptureSurfaceListener OnBufferAvailable");
        OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
        if (buffer != nullptr) {
            void *addr = buffer->GetVirAddr();
            int32_t size = buffer->GetSize();
            // MEDIA_DEBUG_LOG("Saving Image");
            SaveYUV(mode_, addr, size);
            surface_->ReleaseBuffer(buffer, -1);
        } else {
            // MEDIA_DEBUG_LOG("AcquireBuffer failed!");
        }
    }
};

/*
 * Feature: Framework
 * Function: Test Capture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_001, TestSize.Level1)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    EXPECT_TRUE(cameraObjList.size() != 0);

    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    ASSERT_NE(captureSession, nullptr);

    captureSession->BeginConfig();

    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    ASSERT_NE(cameraInput, nullptr);

    int32_t intResult = captureSession->AddInput(cameraInput);
    EXPECT_TRUE(intResult == 0);

    photoSurface = Surface::CreateSurfaceAsConsumer();
    sptr<CaptureSurfaceListener> capturelistener = new CaptureSurfaceListener();
    capturelistener->mode_ = MODE_PHOTO;
    capturelistener->surface_ = photoSurface;
    photoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)capturelistener);
    sptr<CaptureOutput> photoOutput = camManagerObj->CreatePhotoOutput(photoSurface);
    ASSERT_NE(photoOutput, nullptr);

    intResult = captureSession->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = captureSession->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);

    captureSession->Release();
}

/*
 * Feature: Framework
 * Function: Test Capture + Preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture + Preview
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_002, TestSize.Level1)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    EXPECT_TRUE(cameraObjList.size() != 0);

    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    ASSERT_NE(captureSession, nullptr);

    captureSession->BeginConfig();

    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    ASSERT_NE(cameraInput, nullptr);

    int32_t intResult = captureSession->AddInput(cameraInput);
    EXPECT_TRUE(intResult == 0);

    photoSurface = Surface::CreateSurfaceAsConsumer();
    sptr<CaptureSurfaceListener> capturelistener = new CaptureSurfaceListener();
    capturelistener->mode_ = MODE_PHOTO;
    capturelistener->surface_ = photoSurface;
    photoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)capturelistener);
    sptr<CaptureOutput> photoOutput = camManagerObj->CreatePhotoOutput(photoSurface);
    ASSERT_NE(photoOutput, nullptr);

    intResult = captureSession->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    previewSurface = Surface::CreateSurfaceAsConsumer();
    sptr<SurfaceListener> listener = new SurfaceListener();
    listener->mode_ = MODE_PREVIEW;
    listener->surface_ = previewSurface;
    previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
    sptr<CaptureOutput> previewOutput = camManagerObj->CreatePreviewOutput(previewSurface);
    ASSERT_NE(previewOutput, nullptr);

    intResult = captureSession->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = captureSession->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = captureSession->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(5);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);

    sleep(2);
    ((sptr<PhotoOutput> &)photoOutput)->CancelCapture();

    captureSession->Stop();
    captureSession->Release();
}

/*
 * Feature: Framework
 * Function: Test Capture with metadata
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with metadata configured
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_003, TestSize.Level1)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    EXPECT_TRUE(cameraObjList.size() != 0);

    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    ASSERT_NE(captureSession, nullptr);

    captureSession->BeginConfig();

    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    ASSERT_NE(cameraInput, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)cameraInput;

    camera_exposure_mode_enum_t exposure = OHOS_CAMERA_EXPOSURE_MODE_CONTINUOUS_AUTO;
    camera_focus_mode_enum_t focus = OHOS_CAMERA_FOCUS_MODE_AUTO;
    // camera_flash_mode_enum_t flash = OHOS_CAMERA_FLASH_MODE_AUTO;

    camInput->LockForControl();

    // Set metadata parameters
    camInput->SetExposureMode(exposure);
    camInput->SetFocusMode(focus);
    camInput->SetZoomRatio(2.0);
    // camInput->SetFlashMode(flash);

    camInput->UnlockForControl();

    std::vector<CameraInput::PhotoFormat> photoFormats = camInput->GetSupportedPhotoFormats();
    EXPECT_TRUE(photoFormats.size() != 0);

    std::vector<CameraInput::VideoFormat> videoFormats = camInput->GetSupportedVideoFormats();
    EXPECT_TRUE(videoFormats.size() != 0);

    EXPECT_TRUE(camInput->IsPhotoFormatSupported(CameraInput::JPEG_FORMAT));
    EXPECT_TRUE(camInput->IsVideoFormatSupported(CameraInput::YUV_FORMAT));

    std::vector<CameraPicSize *> photoSizes = camInput->GetSupportedSizesForPhoto(CameraInput::JPEG_FORMAT);
    EXPECT_TRUE(photoSizes.size() != 0);

    std::vector<CameraPicSize *> videoSizes = camInput->GetSupportedSizesForVideo(CameraInput::YUV_FORMAT);
    EXPECT_TRUE(videoSizes.size() != 0);

    EXPECT_TRUE(camInput->GetExposureMode() == exposure);
    EXPECT_TRUE(camInput->GetFocusMode() == focus);
    EXPECT_TRUE(camInput->GetZoomRatio() == 2.0);
    // EXPECT_TRUE(camInput->GetFlashMode() == flash);

    std::vector<camera_exposure_mode_enum_t> exposureModes = camInput->GetSupportedExposureModes();
    // EXPECT_TRUE(exposureModes.size() != 0);

    std::vector<camera_focus_mode_enum_t> focusModes = camInput->GetSupportedFocusModes();
    // EXPECT_TRUE(focusModes.size() != 0);

    std::vector<float> zoomRatios = camInput->GetSupportedZoomRatioRange();
    // EXPECT_TRUE(zoomRatios.size() != 0);

    std::vector<camera_flash_mode_enum_t> flashModes = camInput->GetSupportedFlashModes();
    // EXPECT_TRUE(flashModes.size() != 0);

    int32_t intResult = captureSession->AddInput(cameraInput);
    EXPECT_TRUE(intResult == 0);

    photoSurface = Surface::CreateSurfaceAsConsumer();
    sptr<CaptureSurfaceListener> capturelistener = new CaptureSurfaceListener();
    capturelistener->mode_ = MODE_PHOTO;
    capturelistener->surface_ = photoSurface;
    photoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)capturelistener);
    sptr<CaptureOutput> photoOutput = camManagerObj->CreatePhotoOutput(photoSurface);
    ASSERT_NE(photoOutput, nullptr);

    intResult = captureSession->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = captureSession->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);

    captureSession->Release();

    camInput->Release();
}

