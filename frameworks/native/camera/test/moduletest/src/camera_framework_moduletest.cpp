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

#include "camera_framework_moduletest.h"
#include <cinttypes>
#include "input/camera_input.h"
#include "input/camera_manager.h"
#include "camera_log.h"
#include "surface.h"
#include "test_common.h"

#include "ipc_skeleton.h"
#include "access_token.h"
#include "hap_token_info.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"
#include "camera_util.h"

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
    bool g_sessionclosed = false;
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
            if (errorType == CAMERA_DEVICE_PREEMPTED) {
                g_sessionclosed = true;
            }
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

    class AppMetadataCallback : public MetadataObjectCallback, public MetadataStateCallback {
    public:
        void OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const
        {
            MEDIA_DEBUG_LOG("AppMetadataCallback::OnMetadataObjectsAvailable received");
        }
        void OnError(int32_t errorCode) const
        {
            MEDIA_DEBUG_LOG("AppMetadataCallback::OnError %{public}d", errorCode);
        }
    };
} // namespace

static std::string permissionName = "ohos.permission.CAMERA";
static OHOS::Security::AccessToken::HapInfoParams g_infoManagerTestInfoParms = {
    .userID = 1,
    .bundleName = permissionName,
    .instIndex = 0,
    .dlpType = 0,
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
    sptr<SurfaceListener> listener = new(std::nothrow) SurfaceListener("Test_Capture", SurfaceType::PHOTO, fd, surface);
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
    sptr<SurfaceListener> listener = new(std::nothrow) SurfaceListener("Test_Preview", SurfaceType::PREVIEW,
                                                                       fd, surface);
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
    sptr<SurfaceListener> listener = new(std::nothrow) SurfaceListener("Test_Video", SurfaceType::VIDEO,
                                                                       g_videoFd, surface);
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

    // GetExposureBiasRange
    std::vector<int32_t> exposureBiasRange = camInput->GetExposureBiasRange();
    if (!exposureBiasRange.empty()) {
        camInput->SetExposureBias(exposureBiasRange[0]);
    }

    // Get/Set Exposurepoint
    Point exposurePoint = {1, 2};
    camInput->SetExposurePoint(exposurePoint);

    // GetFocalLength
    float focalLength = camInput->GetFocalLength();
    ASSERT_NE(focalLength, 0);

    // Get/Set focuspoint
    Point focusPoint = {1, 2};
    camInput->SetFocusPoint(focusPoint);

    camera_flash_mode_enum_t flash = OHOS_CAMERA_FLASH_MODE_OPEN;
    if (video) {
        flash = OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN;
    }
    camInput->SetFlashMode(flash);

    camera_focus_mode_enum_t focus = OHOS_CAMERA_FOCUS_MODE_AUTO;
    camInput->SetFocusMode(focus);

    camera_exposure_mode_enum_t exposure = OHOS_CAMERA_EXPOSURE_MODE_AUTO;
    camInput->SetExposureMode(exposure);

    camInput->UnlockForControl();

    Point exposurePointGet = camInput->GetExposurePoint();
    EXPECT_EQ(exposurePointGet.x, exposurePoint.x);
    EXPECT_EQ(exposurePointGet.y, exposurePoint.y);

    Point focusPointGet = camInput->GetFocusPoint();
    EXPECT_EQ(focusPointGet.x, focusPoint.x);
    EXPECT_EQ(focusPointGet.y, focusPoint.y);

    if (!zoomRatioRange.empty()) {
        EXPECT_EQ(camInput->GetZoomRatio(), zoomRatioRange[0]);
    }

    // exposureBiasRange
    if (!exposureBiasRange.empty()) {
        EXPECT_EQ(camInput->GetExposureValue(), exposureBiasRange[0]);
    }

    EXPECT_EQ(camInput->GetFlashMode(), flash);
    EXPECT_EQ(camInput->GetFocusMode(), focus);
    EXPECT_EQ(camInput->GetExposureMode(), exposure);
}

void CameraFrameworkModuleTest::TestCallbacksSession(sptr<CaptureOutput> photoOutput,
    sptr<CaptureOutput> videoOutput)
{
    int32_t intResult;

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
        EXPECT_EQ(intResult, 0);
        sleep(WAIT_TIME_AFTER_START);
    }

    if (photoOutput != nullptr) {
        intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
        EXPECT_EQ(intResult, 0);
    }

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
        EXPECT_EQ(intResult, 0);
    }

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

void CameraFrameworkModuleTest::TestCallbacks(sptr<CameraInfo> &cameraInfo, bool video)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    // Register error callback
    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input_;
    camInput->SetErrorCallback(callback);

    SetCameraParameters(camInput, video);

    EXPECT_EQ(g_camInputOnError, false);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

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

    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    // Register preview callback
    ((sptr<PreviewOutput> &)previewOutput)->SetCallback(callback);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    /* In case of wagner device, once commit config is done with flash on
    it is not giving the flash status callback, removing it */
#ifndef PRODUCT_M40
    EXPECT_TRUE(g_camFlashMap.count(cameraInfo->GetID()) != 0);
#endif
    EXPECT_TRUE(g_photoEvents.none());
    EXPECT_TRUE(g_previewEvents.none());
    EXPECT_TRUE(g_videoEvents.none());

    TestCallbacksSession(photoOutput, videoOutput);

    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_START)], 1);

    if (photoOutput != nullptr) {
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] == 1);
        /* In case of wagner device, frame shutter callback not working,
        hence removed. Once supported by hdi, the same needs to be
        enabled */
#ifndef PRODUCT_M40
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_FRAME_SHUTTER)] == 1);
#endif
        EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_END)] == 1);
        ((sptr<PhotoOutput> &)photoOutput)->Release();
    }

    if (videoOutput != nullptr) {
        TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

        EXPECT_EQ(g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_START)], 1);
        EXPECT_EQ(g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_END)], 1);

        ((sptr<VideoOutput> &)videoOutput)->Release();
    }

    ((sptr<PreviewOutput> &)previewOutput)->Release();

    EXPECT_EQ(g_camStatusMap.count(cameraInfo->GetID()), 0);
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
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(true, previewWidth, previewHeight);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = nullptr;
    sptr<CaptureOutput> photoOutput = nullptr;
    if ((videoWidth != videoWidth_) || (videoHeight != videoHeight_)) {
        videoOutput = CreateVideoOutput(videoWidth, videoHeight);
        ASSERT_NE(videoOutput, nullptr);
        intResult = session->AddOutput(videoOutput);
        EXPECT_EQ(intResult, 0);
    } else if ((photoWidth != photoWidth_) || (photoHeight != photoHeight_)) {
        photoOutput = CreatePhotoOutput(photoWidth, photoHeight);
        ASSERT_NE(photoOutput, nullptr);
        intResult = session->AddOutput(photoOutput);
        EXPECT_EQ(intResult, 0);
    }

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->Start();
    EXPECT_EQ(intResult, 0);

    if (photoOutput != nullptr) {
        intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
        EXPECT_EQ(intResult, 0);
        sleep(WAIT_TIME_AFTER_CAPTURE);
    }

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
        EXPECT_EQ(intResult, 0);
        sleep(WAIT_TIME_BEFORE_STOP);
    }

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
        EXPECT_EQ(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = nullptr;
    if ((previewWidth != previewWidth_) || (previewHeight != previewHeight_)) {
        previewOutput = CreatePreviewOutput(true, previewWidth, previewHeight);
    } else {
        previewOutput = CreatePreviewOutput();
    }
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

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
        EXPECT_EQ(intResult, 0);
    }

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);

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

#ifndef PRODUCT_M40
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
        unsigned int tokenIdOld = 0;
        MEDIA_DEBUG_LOG("Alloc TokenID failure, cleaning the old token ID \n");
        tokenIdOld = OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(
            1, permissionName, 0);
        if (tokenIdOld == 0) {
            MEDIA_DEBUG_LOG("Unable to get the Old Token ID, need to reflash the board");
            return;
        }
        ret = OHOS::Security::AccessToken::AccessTokenKit::DeleteToken(tokenIdOld);
        if (ret != 0) {
            MEDIA_DEBUG_LOG("Unable to delete the Old Token ID, need to reflash the board");
            return;
        }

        /* Retry the token allocation again */
        tokenIdEx = OHOS::Security::AccessToken::AccessTokenKit::AllocHapToken(
            g_infoManagerTestInfoParms,
            g_infoManagerTestPolicyPrams);
        if (tokenIdEx.tokenIdExStruct.tokenID == 0) {
            MEDIA_DEBUG_LOG("Alloc TokenID failure, need to reflash the board \n");
            return;
        }
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
    ASSERT_NE(manager_, nullptr);
    manager_->SetCallback(std::make_shared<AppCallback>());

    cameras_ = manager_->GetCameras();
    ASSERT_TRUE(cameras_.size() != 0);

    input_ = manager_->CreateCameraInput(cameras_[0]);
    ASSERT_NE(input_, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)input_;
#ifdef PRODUCT_M40
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
    if (session_) {
        session_->Release();
    }
    if (input_) {
        input_->Release();
    }
    (void)OHOS::Security::AccessToken::AccessTokenKit::DeleteToken(
        tokenIdEx.tokenIdExStruct.tokenID);
    MEDIA_DEBUG_LOG("End of camera test case");
}

#ifndef PRODUCT_M40
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    // Video mode without preview is not supported
    EXPECT_NE(intResult, 0);
#endif
}

#ifdef PRODUCT_M40
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
        std::vector<CameraPicSize> previewSizes =
            ((sptr<CameraInput> &)input_)->getSupportedSizes(previewFormat_);
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
    EXPECT_EQ(previewOutput, nullptr);
}

#ifdef PRODUCT_M40
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input1 = nullptr;
    intResult = session_->AddInput(input1);
    EXPECT_NE(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = nullptr;
    intResult = session_->AddOutput(previewOutput);
    EXPECT_NE(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
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
    EXPECT_NE(intResult, 0);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_NE(intResult, 0);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_NE(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);

    intResult = session_->Start();
    EXPECT_NE(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_NE(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput1 = CreatePhotoOutput();
    ASSERT_NE(photoOutput1, nullptr);

    intResult = session_->AddOutput(photoOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput2 = CreatePhotoOutput();
    ASSERT_NE(photoOutput2, nullptr);

    intResult = session_->AddOutput(photoOutput2);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif

    intResult = session_->Start();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput1)->Capture();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<PhotoOutput> &)photoOutput2)->Capture();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput1 = CreatePreviewOutput();
    ASSERT_NE(previewOutput1, nullptr);

    intResult = session_->AddOutput(previewOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput2, nullptr);

    intResult = session_->AddOutput(previewOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput1 = CreateVideoOutput();
    ASSERT_NE(videoOutput1, nullptr);

    intResult = session_->AddOutput(videoOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput2 = CreateVideoOutput();
    ASSERT_NE(videoOutput2, nullptr);

    intResult = session_->AddOutput(videoOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif

    intResult = session_->Start();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif

    intResult = ((sptr<VideoOutput> &)videoOutput1)->Start();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif

    intResult = ((sptr<VideoOutput> &)videoOutput2)->Start();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput1)->Stop();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
#endif

    intResult = ((sptr<VideoOutput> &)videoOutput2)->Stop();
#ifdef PRODUCT_M40
    EXPECT_EQ(intResult, 0);
#else
    EXPECT_NE(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

#ifdef PRODUCT_M40
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

#ifdef PRODUCT_M40
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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

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
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
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
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> output = nullptr;
    intResult = session_->RemoveOutput(output);
    EXPECT_NE(intResult, 0);
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
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = nullptr;
    intResult = session_->RemoveInput(input);
    EXPECT_NE(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test Capture with location setting [lat:1 ,long:1 ,alt:1]
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_035, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->latitude = 1;
    location->longitude = 1;
    location->altitude = 1;

    photoSetting->SetLocation(location);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Capture with location setting [lat:0.0 ,long:0.0 ,alt:0.0]
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_036, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->latitude = 0.0;
    location->longitude = 0.0;
    location->altitude = 0.0;

    photoSetting->SetLocation(location);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Capture with location setting [lat:-1 ,long:-1 ,alt:-1]
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_037, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->latitude = -1;
    location->longitude = -1;
    location->altitude = -1;

    photoSetting->SetLocation(location);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test snapshot
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test snapshot
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_038, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test snapshot with location setting
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test snapshot with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_039, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->latitude = 12.972442;
    location->longitude = 77.580643;
    location->altitude = 0;

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test snapshot with mirror setting
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test snapshot with mirror setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_040, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetMirror(true);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = manager_->CreateCameraInput(cameras_[1]);
    ASSERT_NE(input, nullptr);

    intResult = session_->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    if (!(((sptr<PhotoOutput> &)photoOutput)->IsMirrorSupported())) {
        return;
    }

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput> &)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Video frame rate with range.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video frame rate with range
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_041, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    std::vector<int32_t> videoFramerateRange = ((sptr<VideoOutput> &)videoOutput)->GetFrameRateRange();
    ASSERT_EQ(videoFramerateRange.empty(), false);
    ASSERT_GE(videoFramerateRange.size(), 2U);
    ((sptr<VideoOutput> &)videoOutput)->SetFrameRateRange(videoFramerateRange[0], videoFramerateRange[1]);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session with Video Stabilization Mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with Video Stabilization Mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_042, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::vector<VideoStabilizationMode> stabilizationmodes = session_->GetSupportedStabilizationMode();
    ASSERT_EQ(stabilizationmodes.empty(), false);

    VideoStabilizationMode stabilizationMode = stabilizationmodes.back();
    if (session_->IsVideoStabilizationModeSupported(stabilizationMode)) {
        session_->SetVideoStabilizationMode(stabilizationMode);
        EXPECT_EQ(session_->GetActiveVideoStabilizationMode(), stabilizationMode);
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput> &)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test camera preempted.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test camera preempted.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_043, TestSize.Level0)
{
    sptr<CaptureSession> session_2 = manager_->CreateCaptureSession();
    ASSERT_NE(session_2, nullptr);

    int32_t intResult = session_2->BeginConfig();
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    sptr<CaptureInput> input_2 = manager_->CreateCameraInput(cameras_[0]);
    ASSERT_NE(input_2, nullptr);
    sptr<CameraInput> camInput_2 = (sptr<CameraInput> &)input_2;
    camInput_2->SetErrorCallback(callback);

    intResult = session_2->AddInput(input_2);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput_2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput_2, nullptr);

    intResult = session_2->AddOutput(previewOutput_2);
    EXPECT_EQ(intResult, 0);

    intResult = session_2->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_2->Start();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureSession> session_3 = manager_->CreateCaptureSession();
    ASSERT_NE(session_3, nullptr);
    EXPECT_EQ(g_sessionclosed, true);

    intResult = session_3->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_3->AddInput(input_2);
    EXPECT_EQ(intResult, 0);

    intResult = session_3->AddOutput(previewOutput_2);
    EXPECT_EQ(intResult, 0);

    intResult = session_3->CommitConfig();
    EXPECT_EQ(intResult, 0);

    session_3->Stop();
}

#ifdef PRODUCT_M40
/*
 * Feature: Framework
 * Function: Test Preview + Metadata
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview + Metadata
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_044, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput> &)metadatOutput;
    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();
    ASSERT_NE(metadataObjectTypes.size(), 0U);

    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> {MetadataObjectType::FACE});

    std::shared_ptr<MetadataObjectCallback> metadataObjectCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataObjectCallback);
    std::shared_ptr<MetadataStateCallback> metadataStateCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataStateCallback);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = metaOutput->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = metaOutput->Stop();
    EXPECT_EQ(intResult, 0);

    session_->Stop();
    metaOutput->Release();
}
#endif
} // CameraStandard
} // OHOS
