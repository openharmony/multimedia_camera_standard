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

#include <cinttypes>

#include "input/camera_input.h"
#include "input/camera_manager.h"
#include "media_log.h"
#include "surface.h"
#include "test_common.h"
#include "camera_framework_moduletest.h"

#include "ipc_skeleton.h"
#include "access_token.h"
#include "hap_token_info.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"

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
    const int32_t WAIT_TIME_AFTER_START = 2;
    const int32_t WAIT_TIME_BEFORE_STOP = 1;

    bool g_camInputOnError = false;
    int32_t g_videoFd = -1;
    std::bitset<static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_MAX_EVENT)> g_photoEvents;
    std::bitset<static_cast<unsigned int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_MAX_EVENT)> g_previewEvents;
    std::bitset<static_cast<unsigned int>(CAM_VIDEO_EVENTS::CAM_VIDEO_MAX_EVENT)> g_videoEvents;
    std::unordered_map<std::string, bool> g_camStatusMap;
    std::unordered_map<std::string, bool> g_camFlashMap;

    class AppCallback : public CameraManagerCallback, public ErrorCallback, public PhotoCallback,
                        public PreviewCallback {
    public:
        void OnCameraStatusChanged(const CameraStatusInfo &cameraStatusInfo) const override
        {
            const std::string cameraID = cameraStatusInfo.cameraInfo->GetID();
            const CameraDeviceStatus cameraStatus = cameraStatusInfo.cameraStatus;

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

        void OnCaptureStarted(const int32_t captureId) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnCaptureStarted captureId: %{public}d", captureId);
            g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] = 1;
            return;
        }

        void OnCaptureEnded(const int32_t captureId, const int32_t frameCount) const override
        {
            MEDIA_DEBUG_LOG("AppCallback::OnCaptureEnded captureId: %{public}d, frameCount: %{public}d",
                            captureId, frameCount);
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
} // namespace

static std::string permissionName = "ohos.permission.CAMERA";
static OHOS::Security::AccessToken::HapInfoParams g_infoManagerTestInfoParms = {
    .userID = 1,
    .bundleName = permissionName,
    .instIndex = 0,
    .appIDDesc = "testtesttesttest"
};

static OHOS::Security::AccessToken::PermissionDef g_infoManagerTestPermDef1 = {
    .permissionName = "ohos.permission.CAMERA",
    .bundleName = "ohos.permission.CAMERA",
    .grantMode = 1,
    .availableLevel = OHOS::Security::AccessToken::ATokenAplEnum::APL_NORMAL,
    .label = "label",
    .labelId = 1,
    .description = "camera test",
    .descriptionId = 1
};

static OHOS::Security::AccessToken::PermissionStateFull g_infoManagerTestState1 = {
    .permissionName = "ohos.permission.CAMERA",
    .isGeneral = true,
    .resDeviceID = {"local"},
    .grantStatus = {OHOS::Security::AccessToken::PermissionState::PERMISSION_GRANTED},
    .grantFlags = {1}
};

static OHOS::Security::AccessToken::HapPolicyParams g_infoManagerTestPolicyPrams = {
    .apl = OHOS::Security::AccessToken::ATokenAplEnum::APL_NORMAL,
    .domain = "test.domain",
    .permList = {g_infoManagerTestPermDef1},
    .permStateList = {g_infoManagerTestState1}
};

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePhotoOutput(int32_t width, int32_t height)
{
    int32_t fd = -1;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(photoFormat_));
    sptr<SurfaceListener> listener = new SurfaceListener("Test_Capture", SurfaceType::PHOTO, fd, surface);
    surface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
    sptr<CaptureOutput> photoOutput = manager_->CreatePhotoOutput(surface);
    return photoOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePhotoOutput()
{
    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(photoWidth_, photoHeight_);
    return photoOutput;
}


sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePreviewOutput(bool customPreview, int32_t width, int32_t height)
{
    int32_t fd = -1;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(previewFormat_));
    sptr<SurfaceListener> listener = new SurfaceListener("Test_Preview", SurfaceType::PREVIEW, fd, surface);
    surface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
    sptr<CaptureOutput> previewOutput = nullptr;
    if (customPreview) {
        previewOutput = manager_->CreateCustomPreviewOutput(surface, width, height);
    } else {
        previewOutput = manager_->CreatePreviewOutput(surface);
    }
    return previewOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePreviewOutput()
{
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(false, previewWidth_, previewHeight_);
    return previewOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreateVideoOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(videoFormat_));
    sptr<SurfaceListener> listener = new SurfaceListener("Test_Video", SurfaceType::VIDEO, g_videoFd, surface);
    surface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
    sptr<CaptureOutput> videoOutput = manager_->CreateVideoOutput(surface);
    return videoOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreateVideoOutput()
{
    sptr<CaptureOutput> videoOutput = CreateVideoOutput(videoWidth_, videoHeight_);
    return videoOutput;
}

void CameraFrameworkModuleTest::SetCameraParameters(sptr<CameraInput> &camInput, bool video)
{
    camInput->LockForControl();

    std::vector<float> zoomRatioRange = camInput->GetSupportedZoomRatioRange();
    if (!zoomRatioRange.empty()) {
        camInput->SetZoomRatio(zoomRatioRange[0]);
    }

    camera_flash_mode_enum_t flash = OHOS_CAMERA_FLASH_MODE_OPEN;
    if (video) {
        flash = OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN;
    }
    camInput->SetFlashMode(flash);

    camera_af_mode_t focus = OHOS_CAMERA_AF_MODE_AUTO;
    camInput->SetFocusMode(focus);

    camera_ae_mode_t exposure = OHOS_CAMERA_AE_MODE_ON;
    camInput->SetExposureMode(exposure);

    camInput->UnlockForControl();

    if (!zoomRatioRange.empty()) {
        EXPECT_TRUE(camInput->GetZoomRatio() == zoomRatioRange[0]);
    }
    EXPECT_TRUE(camInput->GetFlashMode() == flash);
    EXPECT_TRUE(camInput->GetFocusMode() == focus);
    EXPECT_TRUE(camInput->GetExposureMode() == exposure);
}

void CameraFrameworkModuleTest::TestCallbacksSession(sptr<CaptureOutput> photoOutput,
    sptr<CaptureOutput> videoOutput)
{
    int32_t intResult;

    intResult = session_->Start();
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
    session_->Stop();
}

void CameraFrameworkModuleTest::TestCallbacks(sptr<CameraInfo> &cameraInfo, bool video)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    // Register error callback
    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input_;
    camInput->SetErrorCallback(callback);

    SetCameraParameters(camInput, video);

    EXPECT_TRUE(g_camInputOnError == false);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = nullptr;
    sptr<CaptureOutput> videoOutput = nullptr;
    if (!video) {
        photoOutput = CreatePhotoOutput();
        ASSERT_NE(photoOutput, nullptr);

        // Register photo callback
        ((sptr<PhotoOutput> &)photoOutput)->SetCallback(callback);
        intResult = session_->AddOutput(photoOutput);
    } else {
        videoOutput = CreateVideoOutput();
        ASSERT_NE(videoOutput, nullptr);

        // Register video callback
        ((sptr<VideoOutput> &)videoOutput)->SetCallback(std::make_shared<AppVideoCallback>());
        intResult = session_->AddOutput(videoOutput);
    }

    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    // Register preview callback
    ((sptr<PreviewOutput> &)previewOutput)->SetCallback(callback);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    /* In case of wagner device, once commit config is done with flash on
    it is not giving the flash status callback, removing it */
#ifndef BALTIMORE_CAMERA
    EXPECT_TRUE(g_camFlashMap.count(cameraInfo->GetID()) != 0);
#endif
    EXPECT_TRUE(g_photoEvents.none());
    EXPECT_TRUE(g_previewEvents.none());
    EXPECT_TRUE(g_videoEvents.none());

    TestCallbacksSession(photoOutput, videoOutput);

    EXPECT_TRUE(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_START)] == 1);

    if (photoOutput != nullptr) {
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] == 1);
        /* In case of wagner device, frame shutter callback not working,
        hence removed. Once supported by hdi, the same needs to be
        enabled */
#ifndef BALTIMORE_CAMERA
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_FRAME_SHUTTER)] == 1);
#endif
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_END)] == 1);

        ((sptr<PhotoOutput> &)photoOutput)->Release();
    }

    if (videoOutput != nullptr) {
        TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

        EXPECT_TRUE(g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_START)] == 1);
        EXPECT_TRUE(g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_END)] == 1);

        ((sptr<VideoOutput> &)videoOutput)->Release();
    }

    ((sptr<PreviewOutput> &)previewOutput)->Release();

    EXPECT_TRUE(g_camStatusMap.count(cameraInfo->GetID()) == 0);
}

void CameraFrameworkModuleTest::TestSupportedResolution(int32_t previewWidth, int32_t previewHeight,
                                                        int32_t photoWidth, int32_t photoHeight,
                                                        int32_t videoWidth, int32_t videoHeight)
{
    sptr<CaptureInput> input = manager_->CreateCameraInput(cameras_[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureSession> session = manager_->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->AddInput(input);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(true, previewWidth, previewHeight);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = nullptr;
    sptr<CaptureOutput> photoOutput = nullptr;
    if ((videoWidth != videoWidth_) || (videoHeight != videoHeight_)) {
        videoOutput = CreateVideoOutput(videoWidth, videoHeight);
        ASSERT_NE(videoOutput, nullptr);
        intResult = session->AddOutput(videoOutput);
        EXPECT_TRUE(intResult == 0);
    } else if ((photoWidth != photoWidth_) || (photoHeight != photoHeight_)) {
        photoOutput = CreatePhotoOutput(photoWidth, photoHeight);
        ASSERT_NE(photoOutput, nullptr);
        intResult = session->AddOutput(photoOutput);
        EXPECT_TRUE(intResult == 0);
    }

    intResult = session->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session->Start();
    EXPECT_TRUE(intResult == 0);

    if (photoOutput != nullptr) {
        intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
        EXPECT_TRUE(intResult == 0);
        sleep(WAIT_TIME_AFTER_CAPTURE);
    }

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
        EXPECT_TRUE(intResult == 0);
        sleep(WAIT_TIME_BEFORE_STOP);
    }

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
        EXPECT_TRUE(intResult == 0);
        TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);
    }

    session->Stop();
    session->Release();
    input->Release();
}

void CameraFrameworkModuleTest::TestUnSupportedResolution(int32_t previewWidth, int32_t previewHeight,
                                                          int32_t photoWidth, int32_t photoHeight,
                                                          int32_t videoWidth, int32_t videoHeight)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = nullptr;
    if ((previewWidth != previewWidth_) || (previewHeight != previewHeight_)) {
        previewOutput = CreatePreviewOutput(true, previewWidth, previewHeight);
    } else {
        previewOutput = CreatePreviewOutput();
    }
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> output = nullptr;
    if ((videoWidth != videoWidth_) || (videoHeight != videoHeight_)) {
        output = CreateVideoOutput(videoWidth, videoHeight);
        ASSERT_NE(output, nullptr);
    } else if ((photoWidth != photoWidth_) || (photoHeight != photoHeight_)) {
        output = CreatePhotoOutput(photoWidth, photoHeight);
        ASSERT_NE(output, nullptr);
    }

    if (output != nullptr) {
        intResult = session_->AddOutput(output);
        EXPECT_TRUE(intResult == 0);
    }

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    session_->Stop();
}

void CameraFrameworkModuleTest::SetUpTestCase(void) {}
void CameraFrameworkModuleTest::TearDownTestCase(void) {}

void CameraFrameworkModuleTest::SetUpInit()
{
    MEDIA_DEBUG_LOG("Beginning of camera test case!");
    g_photoEvents.reset();
    g_previewEvents.reset();
    g_videoEvents.reset();
    g_camStatusMap.clear();
    g_camFlashMap.clear();
    g_camInputOnError = false;
    g_videoFd = -1;

#ifndef BALTIMORE_CAMERA
    previewFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    videoFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    photoFormat_ = OHOS_CAMERA_FORMAT_JPEG;
    previewWidth_ = PREVIEW_DEFAULT_WIDTH;
    previewHeight_ = PREVIEW_DEFAULT_HEIGHT;
    photoWidth_ = PHOTO_DEFAULT_WIDTH;
    photoHeight_ = PHOTO_DEFAULT_HEIGHT;
    videoWidth_ = VIDEO_DEFAULT_WIDTH;
    videoHeight_ = VIDEO_DEFAULT_HEIGHT;
#endif
}

OHOS::Security::AccessToken::AccessTokenIDEx tokenIdEx = {0};

void CameraFrameworkModuleTest::SetUp()
{
    int32_t ret = -1;
    
    SetUpInit();

    /* Grant the permission so that create camera test can be success */
    tokenIdEx = OHOS::Security::AccessToken::AccessTokenKit::AllocHapToken(
        g_infoManagerTestInfoParms,
        g_infoManagerTestPolicyPrams);
    if (tokenIdEx.tokenIdExStruct.tokenID == 0) {
        MEDIA_DEBUG_LOG("Alloc TokenID failure \n");
        return;
    }

    (void)SetSelfTokenID(tokenIdEx.tokenIdExStruct.tokenID);

    ret = Security::AccessToken::AccessTokenKit::GrantPermission(
        tokenIdEx.tokenIdExStruct.tokenID,
        permissionName, OHOS::Security::AccessToken::PERMISSION_USER_FIXED);
    if (ret != 0) {
        MEDIA_ERR_LOG("GrantPermission( ) failed");
        return;
    } else {
        MEDIA_DEBUG_LOG("GrantPermission( ) success");
    }

    manager_ = CameraManager::GetInstance();
    manager_->SetCallback(std::make_shared<AppCallback>());

    cameras_ = manager_->GetCameras();
    ASSERT_TRUE(cameras_.size() != 0);

    input_ = manager_->CreateCameraInput(cameras_[0]);
    ASSERT_NE(input_, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)input_;
#ifdef BALTIMORE_CAMERA
    previewFormats_ = camInput->GetSupportedPreviewFormats();
    ASSERT_TRUE(previewFormats_.size() != 0);
    if (std::find(previewFormats_.begin(), previewFormats_.end(), OHOS_CAMERA_FORMAT_YCRCB_420_SP)
        != previewFormats_.end()) {
        previewFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    } else {
        previewFormat_ = previewFormats_[0];
    }
    photoFormats_ = camInput->GetSupportedPhotoFormats();
    ASSERT_TRUE(photoFormats_.size() != 0);
    photoFormat_ = photoFormats_[0];
    videoFormats_ = camInput->GetSupportedVideoFormats();
    ASSERT_TRUE(videoFormats_.size() != 0);
    if (std::find(videoFormats_.begin(), videoFormats_.end(), OHOS_CAMERA_FORMAT_YCRCB_420_SP)
        != videoFormats_.end()) {
        videoFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    } else {
        videoFormat_ = videoFormats_[0];
    }
    std::vector<CameraPicSize> previewSizes = camInput->getSupportedSizes(previewFormat_);
    ASSERT_TRUE(previewSizes.size() != 0);
    std::vector<CameraPicSize> photoSizes = camInput->getSupportedSizes(photoFormat_);
    ASSERT_TRUE(photoSizes.size() != 0);
    std::vector<CameraPicSize> videoSizes = camInput->getSupportedSizes(videoFormat_);
    ASSERT_TRUE(videoSizes.size() != 0);
    CameraPicSize size = previewSizes.back();
    previewWidth_ = size.width;
    previewHeight_ = size.height;
    size = photoSizes.back();
    photoWidth_ = size.width;
    photoHeight_ = size.height;
    size = videoSizes.back();
    videoWidth_ = size.width;
    videoHeight_ = size.height;
#endif
    session_ = manager_->CreateCaptureSession();
    ASSERT_NE(session_, nullptr);
}

void CameraFrameworkModuleTest::TearDown()
{
    session_->Release();
    input_->Release();
    (void)OHOS::Security::AccessToken::AccessTokenKit::DeleteToken(
        tokenIdEx.tokenIdExStruct.tokenID);
    MEDIA_DEBUG_LOG("Deleted the allocated Token");
    MEDIA_DEBUG_LOG("End of camera test case");
}

#ifndef BALTIMORE_CAMERA
/*
 * Feature: Framework
 * Function: Test Capture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_001, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);
}
#endif
/*
 * Feature: Framework
 * Function: Test Capture + Preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture + Preview
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_002, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Preview + Video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview + Video
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_003, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_TRUE(intResult == 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test camera status, flash, camera input, photo output and preview output callbacks
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test callbacks
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_004, TestSize.Level0)
{
    TestCallbacks(cameras_[0], false);
}

/*
 * Feature: Framework
 * Function: Test camera status, flash, camera input, preview output and video output callbacks
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test callbacks
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_005, TestSize.Level0)
{
    TestCallbacks(cameras_[0], true);
}

/*
 * Feature: Framework
 * Function: Test Preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_006, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_007, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    // Video mode without preview is not supported
    EXPECT_TRUE(intResult != 0);
#endif
}

#ifdef BALTIMORE_CAMERA
/*
 * Feature: Framework
 * Function: Test Custom Preview with valid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with valid resolutions
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_008, TestSize.Level0)
{
    for (auto &format : previewFormats_) {
        if (format != OHOS_CAMERA_FORMAT_YCRCB_420_SP) {
            continue;
        }
        previewFormat_ = format;
        std::vector<CameraPicSize> previewSizes = ((sptr<CameraInput> &)input_)->getSupportedSizes(previewFormat_);
        for (auto &size : previewSizes) {
            TestSupportedResolution(size.width, size.height, photoWidth_, photoHeight_, videoWidth_, videoHeight_);
        }
    }
}

/*
 * Feature: Framework
 * Function: Test photo with valid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo with valid resolutions
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_009, TestSize.Level0)
{
    for (auto &format : photoFormats_) {
        photoFormat_ = format;
        std::vector<CameraPicSize> photoSizes = ((sptr<CameraInput> &)input_)->getSupportedSizes(photoFormat_);
        for (auto &size : photoSizes) {
            TestSupportedResolution(previewWidth_, previewHeight_, size.width, size.height, videoWidth_, videoHeight_);
        }
    }
}

/*
 * Feature: Framework
 * Function: Test Video with valid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video with valid resolutions
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_010, TestSize.Level0)
{
    for (auto &format : videoFormats_) {
        if (format != OHOS_CAMERA_FORMAT_YCRCB_420_SP) {
            continue;
        }
        videoFormat_ = format;
        std::vector<CameraPicSize> videoSizes = ((sptr<CameraInput> &)input_)->getSupportedSizes(videoFormat_);
        for (auto &size : videoSizes) {
            TestSupportedResolution(previewWidth_, previewHeight_, photoWidth_, photoHeight_, size.width, size.height);
        }
    }
}
#endif

/*
 * Feature: Framework
 * Function: Test Custom Preview with invalid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with invalid resolution(0 * 0)
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_011, TestSize.Level0)
{
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(true, 0, 0);
    EXPECT_TRUE(previewOutput == nullptr);
}

#ifdef BALTIMORE_CAMERA
/*
 * Feature: Framework
 * Function: Test Video with invalid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video with invalid resolution(0 * 0)
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_012, TestSize.Level0)
{
    TestUnSupportedResolution(previewWidth_, previewHeight_, photoWidth_, photoHeight_, 0, 0);
}

/*
 * Feature: Framework
 * Function: Test Capture with invalid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with invalid resolution(0 * 0)
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_013, TestSize.Level0)
{
    TestUnSupportedResolution(previewWidth_, previewHeight_, 0, 0, videoWidth_, videoHeight_);
}

/*
 * Feature: Framework
 * Function: Test Custom Preview with unsupported resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with unsupported resolution
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_014, TestSize.Level0)
{
    int32_t previewWidth = 10;
    int32_t previewHeight = 10;
    TestUnSupportedResolution(previewWidth, previewHeight, photoWidth_, photoHeight_, videoWidth_, videoHeight_);
}

/*
 * Feature: Framework
 * Function: Test Video with unsupported resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video with unsupported resolution
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_015, TestSize.Level0)
{
    int32_t videoWidth = 10;
    int32_t videoHeight = 10;
    TestUnSupportedResolution(previewWidth_, previewHeight_, photoWidth_, photoHeight_, videoWidth, videoHeight);
}

/*
 * Feature: Framework
 * Function: Test Capture with unsupported resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with unsupported resolution
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_016, TestSize.Level0)
{
    int32_t photoWidth = 10;
    int32_t photoHeight = 10;
    TestUnSupportedResolution(previewWidth_, previewHeight_, photoWidth, photoHeight, videoWidth_, videoHeight_);
}
#endif

/*
 * Feature: Framework
 * Function: Test capture session with commit config multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with commit config multiple times
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_017, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->CommitConfig();
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
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_018, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureInput> input1 = nullptr;
    intResult = session_->AddInput(input1);
    EXPECT_TRUE(intResult != 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session add output with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add output with invalid value
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_019, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = nullptr;
    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult != 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding input
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_020, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding output
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_021, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop without adding preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop without adding preview output
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_022, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult != 0);

    intResult = session_->Stop();
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
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_023, TestSize.Level0)
{
    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    int32_t intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult != 0);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_TRUE(intResult != 0);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult != 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult != 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult != 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult != 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple photo outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple photo outputs
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_024, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput1 = CreatePhotoOutput();
    ASSERT_NE(photoOutput1, nullptr);

    intResult = session_->AddOutput(photoOutput1);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput2 = CreatePhotoOutput();
    ASSERT_NE(photoOutput2, nullptr);

    intResult = session_->AddOutput(photoOutput2);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    intResult = session_->Start();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput1)->Capture();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<PhotoOutput> &)photoOutput2)->Capture();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();

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
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_025, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput1 = CreatePreviewOutput();
    ASSERT_NE(previewOutput1, nullptr);

    intResult = session_->AddOutput(previewOutput1);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput2, nullptr);

    intResult = session_->AddOutput(previewOutput2);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->Stop();

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
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_026, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput1 = CreateVideoOutput();
    ASSERT_NE(videoOutput1, nullptr);

    intResult = session_->AddOutput(videoOutput1);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput2 = CreateVideoOutput();
    ASSERT_NE(videoOutput2, nullptr);

    intResult = session_->AddOutput(videoOutput2);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    intResult = session_->Start();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    intResult = ((sptr<VideoOutput> &)videoOutput1)->Start();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    intResult = ((sptr<VideoOutput> &)videoOutput2)->Start();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput1)->Stop();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    intResult = ((sptr<VideoOutput> &)videoOutput2)->Stop();
#ifdef BALTIMORE_CAMERA
    EXPECT_TRUE(intResult == 0);
#else
    EXPECT_TRUE(intResult != 0);
#endif

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    session_->Stop();

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
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_027, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop video multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop video multiple times
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_028, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
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

    session_->Stop();
}

#ifdef BALTIMORE_CAMERA
/*
 * Feature: Framework
 * Function: Test remove video output and commit when preview + video outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove video output and commit when preview + video outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_029, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->RemoveOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->Stop();
}
#endif

/*
 * Feature: Framework
 * Function: Test remove video output, add photo output and commit when preview + video outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove video output, add photo output and commit when preview + video outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_030, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->RemoveOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_TRUE(intResult == 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

#ifdef BALTIMORE_CAMERA
/*
 * Feature: Framework
 * Function: Test remove photo output and commit when preview + photo outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove photo output and commit when preview + photo outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_031, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->RemoveOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->Stop();
}
#endif

/*
 * Feature: Framework
 * Function: Test remove photo output, add video output and commit when preview + photo outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove photo output, add video output and commit when preview + photo outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_032, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddInput(input_);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->RemoveOutput(photoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_TRUE(intResult == 0);

    intResult = session_->CommitConfig();
    EXPECT_TRUE(intResult == 0);

    intResult = session_->Start();
    EXPECT_TRUE(intResult == 0);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_TRUE(intResult == 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_TRUE(intResult == 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session remove output with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove output with null
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_033, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureOutput> output = nullptr;
    intResult = session_->RemoveOutput(output);
    EXPECT_TRUE(intResult != 0);
}

/*
 * Feature: Framework
 * Function: Test capture session remove input with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove input with null
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_034, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_TRUE(intResult == 0);

    sptr<CaptureInput> input = nullptr;
    intResult = session_->RemoveInput(input);
    EXPECT_TRUE(intResult != 0);
}
} // CameraStandard
} // OHOS
