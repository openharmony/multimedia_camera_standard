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

#include "camera_framework_test.h"
#include "input/camera_input.h"
#include "input/camera_manager.h"
#include "media_log.h"
#include "surface.h"
#include "test_common.h"

#include <cinttypes>

using namespace testing::ext;

namespace OHOS {
namespace CameraStandard {
namespace {
    enum class CAM_PHOTO_EVENTS {
        CAM_PHOTO_CAPTURE_START = 0,
        CAM_PHOTO_CAPTURE_END,
        CAM_PHOTO_CAPTURE_ERR,
        CAM_PHOTO_FRAME_SHUTTER,
        CAM_PHOTO_MAX_EVENT
    };

    enum class CAM_PREVIEW_EVENTS {
        CAM_PREVIEW_FRAME_START = 0,
        CAM_PREVIEW_FRAME_END,
        CAM_PREVIEW_FRAME_ERR,
        CAM_PREVIEW_MAX_EVENT
    };

    enum class CAM_VIDEO_EVENTS {
        CAM_VIDEO_FRAME_START = 0,
        CAM_VIDEO_FRAME_END,
        CAM_VIDEO_FRAME_ERR,
        CAM_VIDEO_MAX_EVENT
    };

    const int32_t WAIT_TIME_AFTER_CAPTURE = 1;
    const int32_t WAIT_TIME_AFTER_START = 5;
    const int32_t WAIT_TIME_BEFORE_STOP = 2;
    const int32_t PHOTO_DEFAULT_WIDTH = 1280;
    const int32_t PHOTO_DEFAULT_HEIGHT = 960;
    const int32_t PREVIEW_DEFAULT_WIDTH = 640;
    const int32_t PREVIEW_DEFAULT_HEIGHT = 480;
    const int32_t VIDEO_DEFAULT_WIDTH = 640;
    const int32_t VIDEO_DEFAULT_HEIGHT = 360;
    bool g_camInputOnError = false;
    int32_t g_videoFd = -1;
    std::bitset<static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_MAX_EVENT)> g_photoEvents;
    std::bitset<static_cast<unsigned int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_MAX_EVENT)> g_previewEvents;
    std::bitset<static_cast<unsigned int>(CAM_VIDEO_EVENTS::CAM_VIDEO_MAX_EVENT)> g_videoEvents;
    std::unordered_map<std::string, bool> g_camStatusMap;
    std::unordered_map<std::string, bool> g_camFlashMap;
    sptr<CameraManager> manager;
    std::vector<sptr<CameraInfo>> cameras;
    sptr<CaptureInput> input;
    sptr<CaptureSession> session;

    class AppCallback : public CameraManagerCallback, public ErrorCallback, public PhotoCallback,
                        public PreviewCallback {
    public:
        void OnCameraStatusChanged(const std::string &cameraID, const CameraDeviceStatus cameraStatus) const override
        {
            switch (cameraStatus) {
                case CAMERA_DEVICE_STATUS_UNAVAILABLE: {
                    MEDIA_DEBUG_LOG("AppCallback::OnCameraStatusChanged %{public}s: CAMERA_DEVICE_STATUS_UNAVAILABLE",
                                    cameraID.c_str());
                    g_camStatusMap.erase(cameraID);
                    break;
                }
                case CAMERA_DEVICE_STATUS_AVAILABLE: {
                    MEDIA_DEBUG_LOG("AppCallback::OnCameraStatusChanged %{public}s: CAMERA_DEVICE_STATUS_AVAILABLE",
                                    cameraID.c_str());
                    g_camStatusMap[cameraID] = true;
                    break;
                }
                default: {
                    MEDIA_DEBUG_LOG("AppCallback::OnCameraStatusChanged %{public}s: unknown", cameraID.c_str());
                    EXPECT_TRUE(false);
                }
            }
            return;
        }

        void OnFlashlightStatusChanged(const std::string &cameraID, const FlashlightStatus flashStatus) const override
        {
            switch (flashStatus) {
                case FLASHLIGHT_STATUS_OFF: {
                    MEDIA_DEBUG_LOG("AppCallback::OnFlashlightStatusChanged %{public}s: FLASHLIGHT_STATUS_OFF",
                                    cameraID.c_str());
                    g_camFlashMap[cameraID] = false;
                    break;
                }
                case FLASHLIGHT_STATUS_ON: {
                    MEDIA_DEBUG_LOG("AppCallback::OnFlashlightStatusChanged %{public}s: FLASHLIGHT_STATUS_ON",
                                    cameraID.c_str());
                    g_camFlashMap[cameraID] = true;
                    break;
                }
                case FLASHLIGHT_STATUS_UNAVAILABLE: {
                    MEDIA_DEBUG_LOG("AppCallback::OnFlashlightStatusChanged %{public}s: FLASHLIGHT_STATUS_UNAVAILABLE",
                                    cameraID.c_str());
                    g_camFlashMap.erase(cameraID);
                    break;
                }
                default: {
                    MEDIA_DEBUG_LOG("AppCallback::OnFlashlightStatusChanged %{public}s: unknown", cameraID.c_str());
                    EXPECT_TRUE(false);
                }
            }
            return;
        }

        void OnError(const int32_t errorType, const int32_t errorMsg) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnError errorType: %{public}d, errorMsg: %{public}d", errorType, errorMsg);
            g_camInputOnError = true;
            return;
        }

        void OnCaptureStarted(const int32_t captureID) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnCaptureStarted captureID: %{public}d", captureID);
            g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] = 1;
            return;
        }

        void OnCaptureEnded(const int32_t captureID) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnCaptureEnded captureID: %{public}d", captureID);
            g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_END)] = 1;
            return;
        }

        void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnFrameShutter captureId: %{public}d, timestamp: %{public}"
                            PRIu64, captureId, timestamp);
            g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_FRAME_SHUTTER)] = 1;
            return;
        }

        void OnCaptureError(const int32_t captureId, const int32_t errorCode) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnCaptureError captureId: %{public}d, errorCode: %{public}d",
                            captureId, errorCode);
            g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_ERR)] = 1;
            return;
        }

        void OnFrameStarted() const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnFrameStarted");
            g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_START)] = 1;
            return;
        }
        void OnFrameEnded(const int32_t frameCount) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnFrameEnded frameCount: %{public}d", frameCount);
            g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_END)] = 1;
            return;
        }
        void OnError(const int32_t errorCode) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnError errorCode: %{public}d", errorCode);
            g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_ERR)] = 1;
            return;
        }
    };

    class AppVideoCallback : public VideoCallback {
        void OnFrameStarted() const override
        {
            MEDIA_DEBUG_LOG("AppVideoCallback::OnFrameStarted");
            g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_START)] = 1;
            return;
        }
        void OnFrameEnded(const int32_t frameCount) const override
        {
            MEDIA_DEBUG_LOG("AppVideoCallback::OnFrameEnded frameCount: %{public}d", frameCount);
            g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_END)] = 1;
            return;
        }
        void OnError(const int32_t errorCode) const override
        {
            MEDIA_DEBUG_LOG("AppVideoCallback::OnError errorCode: %{public}d", errorCode);
            g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_ERR)] = 1;
            return;
        }
    };

    sptr<CaptureOutput> CreatePhotoOutput(sptr<CameraManager> &cameraManager,
                                          int32_t width = PHOTO_DEFAULT_WIDTH,
                                          int32_t height = PHOTO_DEFAULT_HEIGHT)
    {
        int32_t fd = -1;
        sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
        surface->SetDefaultWidthAndHeight(width, height);
        sptr<SurfaceListener> listener = new SurfaceListener("Test_Capture", SurfaceType::PHOTO, fd, surface);
        surface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
        sptr<CaptureOutput> photoOutput = cameraManager->CreatePhotoOutput(surface);
        return photoOutput;
    }

    sptr<CaptureOutput> CreatePreviewOutput(sptr<CameraManager> &cameraManager,
                                            bool customPreview = false,
                                            int32_t width = PREVIEW_DEFAULT_WIDTH,
                                            int32_t height = PREVIEW_DEFAULT_HEIGHT)
    {
        int32_t fd = -1;
        sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
        surface->SetDefaultWidthAndHeight(width, height);
        sptr<SurfaceListener> listener = new SurfaceListener("Test_Preview", SurfaceType::PREVIEW, fd, surface);
        surface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
        sptr<CaptureOutput> previewOutput = nullptr;
        if (customPreview) {
            previewOutput = cameraManager->CreateCustomPreviewOutput(surface, width, height);
        } else {
            previewOutput = cameraManager->CreatePreviewOutput(surface);
        }
        return previewOutput;
    }

    sptr<CaptureOutput> CreateVideoOutput(sptr<CameraManager> &cameraManager,
                                          int32_t width = VIDEO_DEFAULT_WIDTH,
                                          int32_t height = VIDEO_DEFAULT_HEIGHT)
    {
        sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
        surface->SetDefaultWidthAndHeight(width, height);
        sptr<SurfaceListener> listener = new SurfaceListener("Test_Video", SurfaceType::VIDEO, g_videoFd, surface);
        surface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
        sptr<CaptureOutput> videoOutput = cameraManager->CreateVideoOutput(surface);
        return videoOutput;
    }
} // namespace

void CameraFrameworkTest::SetUpTestCase(void) {}
void CameraFrameworkTest::TearDownTestCase(void) {}

void CameraFrameworkTest::SetUp()
{
    g_photoEvents.reset();
    g_previewEvents.reset();
    g_videoEvents.reset();
    g_camStatusMap.clear();
    g_camFlashMap.clear();
    g_camInputOnError = false;
    g_videoFd = -1;

    manager = CameraManager::GetInstance();
    manager->SetCallback(std::make_shared<AppCallback>());

    cameras = manager->GetCameras();
    ASSERT_TRUE(cameras.size() != 0);

    input = manager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    session = manager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
}

void CameraFrameworkTest::TearDown()
{
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test Capture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_001, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(manager);
    ASSERT_NE(photoOutput, nullptr);

    intResult = session->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);
}

/*
 * Feature: Framework
 * Function: Test Capture + Preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture + Preview
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_002, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(manager);
    ASSERT_NE(photoOutput, nullptr);

    intResult = session->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test Preview + Video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview + Video
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_003, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(manager);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_TRUE(intResult == 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);
    session->Stop();
}

void SetCameraParameters(sptr<CameraInput> &camInput)
{
    camInput->LockForControl();

    float zoom = 4.0;
    camInput->SetZoomRatio(zoom);

    camera_flash_mode_enum_t flash = OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN;
    camInput->SetFlashMode(flash);

    camera_focus_mode_enum_t focus = OHOS_CAMERA_FOCUS_MODE_AUTO;
    camInput->SetFocusMode(focus);

    camera_exposure_mode_enum_t exposure = OHOS_CAMERA_EXPOSURE_MODE_CONTINUOUS_AUTO;
    camInput->SetExposureMode(exposure);

    camInput->UnlockForControl();

    EXPECT_TRUE(camInput->GetZoomRatio() == zoom);
    EXPECT_TRUE(camInput->GetFlashMode() == flash);
    EXPECT_TRUE(camInput->GetFocusMode() == focus);
    EXPECT_TRUE(camInput->GetExposureMode() == exposure);
}

void TestCallbacks(bool video)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    // Register error callback
    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    camInput->SetErrorCallback(callback);

    SetCameraParameters(camInput);

    EXPECT_TRUE(g_camInputOnError == false);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = nullptr;
    sptr<CaptureOutput> videoOutput = nullptr;
    if (!video) {
        photoOutput = CreatePhotoOutput(manager);
        ASSERT_NE(photoOutput, nullptr);

        // Register photo callback
        ((sptr<PhotoOutput> &)photoOutput)->SetCallback(callback);
        intResult = session->AddOutput(photoOutput);
    } else {
        videoOutput = CreateVideoOutput(manager);
        ASSERT_NE(videoOutput, nullptr);

        // Register video callback
        ((sptr<VideoOutput> &)videoOutput)->SetCallback(std::make_shared<AppVideoCallback>());
        intResult = session->AddOutput(videoOutput);
    }

    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    // Register preview callback
    ((sptr<PreviewOutput> &)previewOutput)->SetCallback(callback);
    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    EXPECT_TRUE(g_camFlashMap.count(cameras[0]->GetID()) != 0);

    EXPECT_TRUE(g_photoEvents.none());
    EXPECT_TRUE(g_previewEvents.none());
    EXPECT_TRUE(g_videoEvents.none());

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
        EXPECT_TRUE(intResult == 0);
        sleep(WAIT_TIME_AFTER_START);
    }

    if (photoOutput != nullptr) {
        intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
        EXPECT_TRUE(intResult == 0);
    }

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
        EXPECT_TRUE(intResult == 0);
    }

    sleep(WAIT_TIME_BEFORE_STOP);
    session->Stop();

    EXPECT_TRUE(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_START)] == 1);

    camInput->Release();

    if (photoOutput != nullptr) {
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] == 1);
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_FRAME_SHUTTER)] == 1);
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_END)] == 1);

        ((sptr<PhotoOutput> &)photoOutput)->Release();
    }

    if (videoOutput != nullptr) {
        TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

        EXPECT_TRUE(g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_START)] == 1);

        ((sptr<VideoOutput> &)videoOutput)->Release();
    }

    ((sptr<PreviewOutput> &)previewOutput)->Release();

    EXPECT_TRUE(g_camStatusMap.count(cameras[0]->GetID()) == 0);
}

/*
 * Feature: Framework
 * Function: Test camerastatus, flash, camera input, photo output and preview output callbacks
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test callbacks
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_004, TestSize.Level0)
{
    TestCallbacks(false);
}

/*
 * Feature: Framework
 * Function: Test camera status, flash, camera input, preview output and video output callbacks
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test callbacks
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_005, TestSize.Level0)
{
    TestCallbacks(true);
}

/*
 * Feature: Framework
 * Function: Test Preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_006, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test Video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_007, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(manager);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    // Video mode without preview is not supported
    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult != 0);
}

void TestSupportedResolution(int32_t previewWidth, int32_t previewHeight, int32_t videoWidth = VIDEO_DEFAULT_WIDTH,
                             int32_t videoHeight = VIDEO_DEFAULT_HEIGHT)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager, true, previewWidth, previewHeight);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = nullptr;
    if ((videoWidth != VIDEO_DEFAULT_WIDTH) || (videoHeight != VIDEO_DEFAULT_HEIGHT)) {
        videoOutput = CreateVideoOutput(manager, videoWidth, videoHeight);
        ASSERT_NE(videoOutput, nullptr);

        intResult = session->AddOutput(videoOutput);
        EXPECT_TRUE(intResult == 0);
    }

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
        EXPECT_TRUE(intResult == 0);
    }

    sleep(WAIT_TIME_AFTER_START);

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
        EXPECT_TRUE(intResult == 0);
        TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);
    }

    session->Stop();
}

void TestUnSupportedResolution(int32_t previewWidth, int32_t previewHeight, int32_t videoWidth = VIDEO_DEFAULT_WIDTH,
                               int32_t videoHeight = VIDEO_DEFAULT_HEIGHT, int32_t photoWidth = PHOTO_DEFAULT_WIDTH,
                               int32_t photoHeight = PHOTO_DEFAULT_HEIGHT)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = nullptr;
    if ((previewWidth != PREVIEW_DEFAULT_WIDTH) || (previewHeight != PREVIEW_DEFAULT_HEIGHT)) {
        previewOutput = CreatePreviewOutput(manager, true, previewWidth, previewHeight);
    } else {
        previewOutput = CreatePreviewOutput(manager);
    }
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> output = nullptr;
    if ((videoWidth != VIDEO_DEFAULT_WIDTH) || (videoHeight != VIDEO_DEFAULT_HEIGHT)) {
        output = CreateVideoOutput(manager, videoWidth, videoHeight);
        ASSERT_NE(output, nullptr);
    } else if ((photoWidth != PHOTO_DEFAULT_WIDTH) || (photoHeight != PHOTO_DEFAULT_HEIGHT)) {
        output = CreatePhotoOutput(manager, photoWidth, photoHeight);
        ASSERT_NE(output, nullptr);
    }

    if (output != nullptr) {
        intResult = session->AddOutput(output);
        EXPECT_TRUE(intResult == 0);
    }

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test Custom Preview with valid resolution
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with valid resolution(640 * 480)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_008, TestSize.Level0)
{
    TestSupportedResolution(PREVIEW_DEFAULT_WIDTH, PREVIEW_DEFAULT_HEIGHT);
}

/*
 * Feature: Framework
 * Function: Test Custom Preview with valid resolution
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with valid resolution(832 * 480)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_009, TestSize.Level0)
{
    TestSupportedResolution(832, 480);
}

/*
 * Feature: Framework
 * Function: Test Video with valid resolution
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video with valid resolution(1280 * 720)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_010, TestSize.Level0)
{
    TestSupportedResolution(PREVIEW_DEFAULT_WIDTH, PREVIEW_DEFAULT_HEIGHT, 1280, 720);
}

/*
 * Feature: Framework
 * Function: Test Custom Preview with invalid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with invalid resolutions(0 * 0)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_011, TestSize.Level0)
{
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager, true, 0, 0);
    EXPECT_TRUE(previewOutput == nullptr);
}


/*
 * Feature: Framework
 * Function: Test Video with invalid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video with invalid resolutions(0 * 0)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_012, TestSize.Level0)
{
    TestUnSupportedResolution(PREVIEW_DEFAULT_WIDTH, PREVIEW_DEFAULT_HEIGHT, 0, 0);
}

/*
 * Feature: Framework
 * Function: Test Capture with invalid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with invalid resolutions(0 * 0)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_013, TestSize.Level0)
{
    TestUnSupportedResolution(PREVIEW_DEFAULT_WIDTH, PREVIEW_DEFAULT_HEIGHT, VIDEO_DEFAULT_WIDTH,
                              VIDEO_DEFAULT_HEIGHT, 0, 0);
}

/*
 * Feature: Framework
 * Function: Test Custom Preview with unsupported resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with unsupported resolutions(1280 * 720)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_014, TestSize.Level0)
{
    int32_t previewWidth = 1280;
    int32_t previewHeight = 720;
    TestUnSupportedResolution(previewWidth, previewHeight);
}

/*
 * Feature: Framework
 * Function: Test Video with unsupported resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video with unsupported resolutions(640 * 480)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_015, TestSize.Level0)
{
    int32_t videoWidth = 640;
    int32_t videoHeight = 480;
    TestUnSupportedResolution(PREVIEW_DEFAULT_WIDTH, PREVIEW_DEFAULT_HEIGHT, videoWidth, videoHeight);
}

/*
 * Feature: Framework
 * Function: Test Capture with unsupported resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with unsupported resolutions(640 * 480)
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_016, TestSize.Level0)
{
    int32_t photoWidth = 640;
    int32_t photoHeight = 480;
    TestUnSupportedResolution(PREVIEW_DEFAULT_WIDTH, PREVIEW_DEFAULT_HEIGHT, VIDEO_DEFAULT_WIDTH,
                              VIDEO_DEFAULT_HEIGHT, photoWidth, photoHeight);
}

/*
 * Feature: Framework
 * Function: Test capture session with commit config multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with commit config multiple times
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_017, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult != 0);
}

/*
 * Feature: Framework
 * Function: Test capture session add input with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add input with invalid value
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_018, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureInput> input1 = nullptr;
    intResult = session->AddInput(input1);
    EXPECT_TRUE(intResult != 0);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session add output with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add output with invalid value
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_019, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = nullptr;
    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult != 0);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding input
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_020, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding output
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_021, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop without adding preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop without adding preview output
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_022, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(manager);
    ASSERT_NE(photoOutput, nullptr);

    intResult = session->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult != 0);

    intResult = session->Stop();
    EXPECT_TRUE(intResult != 0);
}

/*
 * Feature: Framework
 * Function: Test capture session without begin config
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session without begin config
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_023, TestSize.Level0)
{
    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(manager);
    ASSERT_NE(photoOutput, nullptr);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    int32_t intResult = session->AddInput(input);
    EXPECT_TRUE(intResult != 0);

    intResult = session->AddOutput(photoOutput);
    EXPECT_TRUE(intResult != 0);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult != 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult != 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult != 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple photo outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple photo outputs
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_024, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput1 = CreatePhotoOutput(manager);
    ASSERT_NE(photoOutput1, nullptr);

    intResult = session->AddOutput(photoOutput1);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput2 = CreatePhotoOutput(manager);
    ASSERT_NE(photoOutput2, nullptr);

    intResult = session->AddOutput(photoOutput2);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput1)->Capture();
    EXPECT_TRUE(intResult != 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<PhotoOutput> &)photoOutput2)->Capture();
    EXPECT_TRUE(intResult == 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session->Stop();

    ((sptr<PhotoOutput> &)photoOutput1)->Release();
    ((sptr<PhotoOutput> &)photoOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple preview outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple preview ouputs
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_025, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput1 = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput1, nullptr);

    intResult = session->AddOutput(previewOutput1);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput2 = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput2, nullptr);

    intResult = session->AddOutput(previewOutput2);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session->Stop();

    ((sptr<PhotoOutput> &)previewOutput1)->Release();
    ((sptr<PhotoOutput> &)previewOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple video outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple video ouputs
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_026, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput1 = CreateVideoOutput(manager);
    ASSERT_NE(videoOutput1, nullptr);

    intResult = session->AddOutput(videoOutput1);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput2 = CreateVideoOutput(manager);
    ASSERT_NE(videoOutput2, nullptr);

    intResult = session->AddOutput(videoOutput2);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<VideoOutput> &)videoOutput1)->Start();
    EXPECT_TRUE(intResult != 0);

    intResult = ((sptr<VideoOutput> &)videoOutput2)->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput1)->Stop();
    EXPECT_TRUE(intResult != 0);

    intResult = ((sptr<VideoOutput> &)videoOutput2)->Stop();
    EXPECT_TRUE(intResult == 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    session->Stop();

    ((sptr<PhotoOutput> &)videoOutput1)->Release();
    ((sptr<PhotoOutput> &)videoOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop preview multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop preview multiple times
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_027, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session->Stop();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session->Stop();
}


/*
 * Feature: Framework
 * Function: Test capture session start and stop video multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop video multiple times
 */
HWTEST_F(CameraFrameworkTest, media_camera_framework_test_028, TestSize.Level0)
{
    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(manager);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(manager);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_TRUE(intResult == 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    session->Stop();
}
} // CameraStandard
} // OHOS