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

#include "camera_framework_unittest.h"
#include "camera_util.h"
#include "gmock/gmock.h"
#include "input/camera_input.h"
#include "surface.h"
#include "test_common.h"

#include "ipc_skeleton.h"
#include "access_token.h"
#include "hap_token_info.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"
#include "metadata_utils.h"

using namespace testing::ext;
using ::testing::A;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Camera::V1_0;
class MockStreamOperator : public IStreamOperator {
public:
    MockStreamOperator()
    {
        ON_CALL(*this, CreateStreams(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, ReleaseStreams(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, CommitStreams(_, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, Capture(_, _, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, CancelCapture(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported(_, _, A<const std::shared_ptr<StreamInfo> &>(), _))
            .WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported(_, _, A<const std::vector<StreamInfo> &>(), _))
            .WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, GetStreamAttributes(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, AttachBufferQueue(_, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, DetachBufferQueue(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, ChangeToOfflineStream(_, _, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
    }
    ~MockStreamOperator() {}
    MOCK_METHOD1(CreateStreams, int32_t(
        const std::vector<StreamInfo>& streamInfos));
    MOCK_METHOD1(ReleaseStreams, int32_t(const std::vector<int32_t>& streamIds));
    MOCK_METHOD1(CancelCapture, int32_t(int32_t captureId));
    MOCK_METHOD1(GetStreamAttributes, int32_t(
        std::vector<StreamAttribute>& attributes));
    MOCK_METHOD1(DetachBufferQueue, int32_t(int32_t streamId));
    MOCK_METHOD2(CommitStreams, int32_t(OperationMode mode, const std::vector<uint8_t>& modeSetting));
    MOCK_METHOD2(AttachBufferQueue, int32_t(int32_t streamId,
        const sptr<BufferProducerSequenceable>& bufferProducer));
    MOCK_METHOD3(Capture, int32_t(int32_t captureId, const CaptureInfo& info, bool isStreaming));
    MOCK_METHOD3(ChangeToOfflineStream, int32_t(const std::vector<int32_t>& streamIds,
         const sptr<IStreamOperatorCallback>& callbackObj, sptr<IOfflineStreamOperator>& offlineOperator));
    MOCK_METHOD4(IsStreamsSupported, int32_t(OperationMode mode,
        const std::shared_ptr<OHOS::Camera::CameraMetadata> &modeSetting,
        const std::shared_ptr<StreamInfo> &info, StreamSupportType &type));
    MOCK_METHOD4(IsStreamsSupported, int32_t(OperationMode mode, const std::vector<uint8_t>& modeSetting,
        const std::vector<StreamInfo>& infos, StreamSupportType& type));
};

class MockCameraDevice : public ICameraDevice {
public:
    MockCameraDevice()
    {
        streamOperator = new MockStreamOperator();
        ON_CALL(*this, GetStreamOperator).WillByDefault([this](
            const OHOS::sptr<IStreamOperatorCallback> &callback,
            OHOS::sptr<IStreamOperator> &pStreamOperator) {
            pStreamOperator = streamOperator;
            return HDI::Camera::V1_0::NO_ERROR;
        });
        ON_CALL(*this, UpdateSettings(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, SetResultMode(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, GetEnabledResults(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, EnableResult(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, DisableResult(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, DisableResult(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, Close()).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
    }
    ~MockCameraDevice() {}
    MOCK_METHOD0(Close, int32_t());
    MOCK_METHOD1(UpdateSettings, int32_t(const std::vector<uint8_t>& settings));
    MOCK_METHOD1(SetResultMode, int32_t(ResultCallbackMode mode));
    MOCK_METHOD1(GetEnabledResults, int32_t(std::vector<int32_t>& results));
    MOCK_METHOD1(EnableResult, int32_t(const std::vector<int32_t>& results));
    MOCK_METHOD1(DisableResult, int32_t(const std::vector<int32_t>& results));
    MOCK_METHOD2(GetStreamOperator, int32_t(const sptr<IStreamOperatorCallback>& callbackObj,
        sptr<IStreamOperator>& streamOperator));
    sptr<MockStreamOperator> streamOperator;
};

class MockHCameraHostManager : public HCameraHostManager {
public:
    explicit MockHCameraHostManager(StatusCallback* statusCallback) : HCameraHostManager(statusCallback)
    {
        cameraDevice = new MockCameraDevice();
        ON_CALL(*this, GetCameras).WillByDefault([this](std::vector<std::string> &cameraIds) {
            cameraIds.emplace_back("cam0");
            return CAMERA_OK;
        });
        ON_CALL(*this, GetCameraAbility).WillByDefault([this](std::string &cameraId,
                                                            std::shared_ptr<OHOS::Camera::CameraMetadata> &ability) {
            int32_t itemCount = 10;
            int32_t dataSize = 100;
            ability = std::make_shared<OHOS::Camera::CameraMetadata>(itemCount, dataSize);
            int32_t streams[9] = {
                OHOS_CAMERA_FORMAT_YCRCB_420_SP, CameraFrameworkUnitTest::PREVIEW_DEFAULT_WIDTH,
                CameraFrameworkUnitTest::PREVIEW_DEFAULT_HEIGHT, OHOS_CAMERA_FORMAT_YCRCB_420_SP,
                CameraFrameworkUnitTest::VIDEO_DEFAULT_WIDTH, CameraFrameworkUnitTest::VIDEO_DEFAULT_HEIGHT,
                OHOS_CAMERA_FORMAT_JPEG, CameraFrameworkUnitTest::PHOTO_DEFAULT_WIDTH,
                CameraFrameworkUnitTest::PHOTO_DEFAULT_HEIGHT
            };
            ability->addEntry(OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, streams,
                              sizeof(streams) / sizeof(streams[0]));

            int32_t compensationRange[2] = {-2, 3};
            ability->addEntry(OHOS_CONTROL_AE_COMPENSATION_RANGE, compensationRange,
                              sizeof(compensationRange) / sizeof(compensationRange[0]));

            float focalLength = 1.5;
            ability->addEntry(OHOS_ABILITY_FOCAL_LENGTH, &focalLength, sizeof(float));

            uint8_t faceDetectModes[] = {OHOS_CAMERA_FACE_DETECT_MODE_OFF, OHOS_CAMERA_FACE_DETECT_MODE_SIMPLE};
            ability->addEntry(OHOS_STATISTICS_FACE_DETECT_MODE, faceDetectModes, sizeof(faceDetectModes));
            return CAMERA_OK;
        });
        ON_CALL(*this, OpenCameraDevice).WillByDefault([this](std::string &cameraId,
                                                            const sptr<ICameraDeviceCallback> &callback,
                                                            sptr<ICameraDevice> &pDevice) {
            pDevice = cameraDevice;
            return CAMERA_OK;
        });
        ON_CALL(*this, SetFlashlight(_, _)).WillByDefault(Return(CAMERA_OK));
        ON_CALL(*this, SetCallback(_)).WillByDefault(Return(CAMERA_OK));
    }
    ~MockHCameraHostManager() {}
    MOCK_METHOD1(GetCameras, int32_t(std::vector<std::string> &cameraIds));
    MOCK_METHOD1(SetCallback, int32_t(sptr<ICameraHostCallback> &callback));
    MOCK_METHOD2(GetCameraAbility, int32_t(std::string &cameraId,
        std::shared_ptr<OHOS::Camera::CameraMetadata> &ability));
    MOCK_METHOD2(SetFlashlight, int32_t(const std::string &cameraId, bool isEnable));
    MOCK_METHOD3(OpenCameraDevice, int32_t(std::string &cameraId,
        const sptr<ICameraDeviceCallback> &callback, sptr<ICameraDevice> &pDevice));
    sptr<MockCameraDevice> cameraDevice;
};

class FakeHCameraService : public HCameraService {
public:
    explicit FakeHCameraService(sptr<HCameraHostManager> hostManager) : HCameraService(hostManager) {}
    ~FakeHCameraService() {}
};

class FakeCameraManager : public CameraManager {
public:
    explicit FakeCameraManager(sptr<HCameraService> service) : CameraManager(service) {}
    ~FakeCameraManager() {}
};

class AppMetadataCallback : public MetadataObjectCallback, public MetadataStateCallback {
public:
    void OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const {}
    void OnError(int32_t errorCode) const {}
};

sptr<CaptureOutput> CameraFrameworkUnitTest::CreatePhotoOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    if (surface == nullptr) {
        return nullptr;
    }
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_JPEG));
    return cameraManager->CreatePhotoOutput(surface);
}

sptr<CaptureOutput> CameraFrameworkUnitTest::CreatePreviewOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    if (surface == nullptr) {
        return nullptr;
    }
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
    return cameraManager->CreatePreviewOutput(surface);
}

sptr<CaptureOutput> CameraFrameworkUnitTest::CreateVideoOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    if (surface == nullptr) {
        return nullptr;
    }
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
    return cameraManager->CreateVideoOutput(surface);
}

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

OHOS::Security::AccessToken::AccessTokenIDEx tokenIdEx = {0};

void CameraFrameworkUnitTest::SetUpTestCase(void)
{
    int32_t ret = -1;

    /* Grant the permission so that create camera test can be success */
    tokenIdEx = OHOS::Security::AccessToken::AccessTokenKit::AllocHapToken(
        g_infoManagerTestInfoParms,
        g_infoManagerTestPolicyPrams);
    if (tokenIdEx.tokenIdExStruct.tokenID == 0) {
        unsigned int tokenIdOld = 0;
        tokenIdOld = OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(
            1, permissionName, 0);
        if (tokenIdOld == 0) {
            return;
        }
        ret = OHOS::Security::AccessToken::AccessTokenKit::DeleteToken(tokenIdOld);
        if (ret != 0) {
            return;
        }

        /* Retry the token allocation again */
        tokenIdEx = OHOS::Security::AccessToken::AccessTokenKit::AllocHapToken(
            g_infoManagerTestInfoParms,
            g_infoManagerTestPolicyPrams);
        if (tokenIdEx.tokenIdExStruct.tokenID == 0) {
            return;
        }
    }

    (void)SetSelfTokenID(tokenIdEx.tokenIdExStruct.tokenID);

    ret = Security::AccessToken::AccessTokenKit::GrantPermission(
        tokenIdEx.tokenIdExStruct.tokenID,
        permissionName, OHOS::Security::AccessToken::PERMISSION_USER_FIXED);
    if (ret != 0) {
        return;
    }
}

void CameraFrameworkUnitTest::TearDownTestCase(void)
{
    (void)OHOS::Security::AccessToken::AccessTokenKit::DeleteToken(
        tokenIdEx.tokenIdExStruct.tokenID);
}

void CameraFrameworkUnitTest::SetUp()
{
    mockCameraHostManager = new MockHCameraHostManager(nullptr);
    mockCameraDevice = mockCameraHostManager->cameraDevice;
    mockStreamOperator = mockCameraDevice->streamOperator;
    cameraManager = new FakeCameraManager(new FakeHCameraService(mockCameraHostManager));
}

void CameraFrameworkUnitTest::TearDown()
{
    Mock::AllowLeak(mockCameraHostManager);
    Mock::AllowLeak(mockCameraDevice);
    Mock::AllowLeak(mockStreamOperator);
}

MATCHER_P(matchCaptureSetting, captureSetting, "Match Capture Setting")
{
    std::vector<uint8_t> result;
    OHOS::Camera::MetadataUtils::ConvertMetadataToVec(captureSetting, result);
    return (arg.captureSetting_ == result);
}

/*
 * Feature: Framework
 * Function: Test get cameras
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test get cameras
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_001, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();
    ASSERT_TRUE(cameras.size() != 0);
}

/*
 * Feature: Framework
 * Function: Test create input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create input
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_002, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();
    ASSERT_TRUE(cameras.size() != 0);

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    ASSERT_NE(input->GetCameraDevice(), nullptr);

    input->Release();
}

/*
 * Feature: Framework
 * Function: Test create session
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create session
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_003, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test create preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create preview output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_004, TestSize.Level0)
{
    int32_t width = PREVIEW_DEFAULT_WIDTH;
    int32_t height = PREVIEW_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(surface);
    ASSERT_NE(preview, nullptr);
    preview->Release();
}

/*
 * Feature: Framework
 * Function: Test create preview output with surface as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create preview output with surface as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_005, TestSize.Level0)
{
    sptr<Surface> surface = nullptr;
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(surface);
    ASSERT_EQ(preview, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create preview output with buffer producer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create preview output with buffer producer
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_006, TestSize.Level0)
{
    int32_t width = PREVIEW_DEFAULT_WIDTH;
    int32_t height = PREVIEW_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(surface->GetProducer(),
                                                                     OHOS_CAMERA_FORMAT_YCRCB_420_SP);
    ASSERT_NE(preview, nullptr);
    preview->Release();
}

/*
 * Feature: Framework
 * Function: Test create preview output with buffer producer as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create preview output with buffer producer as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_007, TestSize.Level0)
{
    sptr<OHOS::IBufferProducer> producer = nullptr;
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(producer, OHOS_CAMERA_FORMAT_YCRCB_420_SP);
    ASSERT_EQ(preview, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create custom preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create custom preview output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_008, TestSize.Level0)
{
    int32_t width = PREVIEW_DEFAULT_WIDTH;
    int32_t height = PREVIEW_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
    sptr<CaptureOutput> preview = cameraManager->CreateCustomPreviewOutput(surface, width, height);
    ASSERT_NE(preview, nullptr);
    preview->Release();
}

/*
 * Feature: Framework
 * Function: Test create custom preview output with surface as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create custom preview output with surface as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_009, TestSize.Level0)
{
    int32_t width = PREVIEW_DEFAULT_WIDTH;
    int32_t height = PREVIEW_DEFAULT_HEIGHT;
    sptr<Surface> surface = nullptr;
    sptr<CaptureOutput> preview = cameraManager->CreateCustomPreviewOutput(surface, width, height);
    ASSERT_EQ(preview, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create custom preview output with width and height as 0
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create custom preview output with width and height as 0
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_010, TestSize.Level0)
{
    int32_t width = 0;
    int32_t height = 0;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    sptr<CaptureOutput> preview = cameraManager->CreateCustomPreviewOutput(surface, width, height);
    ASSERT_EQ(preview, nullptr);
}


/*
 * Feature: Framework
 * Function: Test create photo output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create photo output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_011, TestSize.Level0)
{
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_JPEG));
    sptr<PhotoOutput> photo = cameraManager->CreatePhotoOutput(surface);
    ASSERT_NE(photo, nullptr);
    photo->Release();
}

/*
 * Feature: Framework
 * Function: Test create photo output with surface as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create photo output with surface as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_012, TestSize.Level0)
{
    sptr<Surface> surface = nullptr;
    sptr<PhotoOutput> photo = cameraManager->CreatePhotoOutput(surface);
    ASSERT_EQ(photo, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create photo output with buffer producer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create photo output with buffer producer
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_013, TestSize.Level0)
{
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    sptr<PhotoOutput> photo = cameraManager->CreatePhotoOutput(surface->GetProducer(), OHOS_CAMERA_FORMAT_JPEG);
    ASSERT_NE(photo, nullptr);
    photo->Release();
}

/*
 * Feature: Framework
 * Function: Test create photo output with buffer producer as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create photo output with buffer producer as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_014, TestSize.Level0)
{
    sptr<OHOS::IBufferProducer> producer = nullptr;
    sptr<PhotoOutput> photo = cameraManager->CreatePhotoOutput(producer, OHOS_CAMERA_FORMAT_JPEG);
    ASSERT_EQ(photo, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create video output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create video output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_015, TestSize.Level0)
{
    int32_t width = VIDEO_DEFAULT_WIDTH;
    int32_t height = VIDEO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(surface);
    ASSERT_NE(video, nullptr);
    video->Release();
}

/*
 * Feature: Framework
 * Function: Test create video output with surface as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create video output with surface as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_016, TestSize.Level0)
{
    sptr<Surface> surface = nullptr;
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(surface);
    ASSERT_EQ(video, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create video output with buffer producer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create video output with buffer producer
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_017, TestSize.Level0)
{
    int32_t width = VIDEO_DEFAULT_WIDTH;
    int32_t height = VIDEO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(surface->GetProducer(), OHOS_CAMERA_FORMAT_YCRCB_420_SP);
    ASSERT_NE(video, nullptr);
    video->Release();
}

/*
 * Feature: Framework
 * Function: Test create video output with buffer producer as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create video output with buffer producer as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_018, TestSize.Level0)
{
    sptr<OHOS::IBufferProducer> producer = nullptr;
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(producer, OHOS_CAMERA_FORMAT_YCRCB_420_SP);
    ASSERT_EQ(video, nullptr);
}

/*
 * Feature: Framework
 * Function: Test manager callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test manager callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_019, TestSize.Level0)
{
    std::shared_ptr<TestCameraMngerCallback> setCallback = std::make_shared<TestCameraMngerCallback>("MgrCallback");
    cameraManager->SetCallback(setCallback);
    std::shared_ptr<CameraManagerCallback> getCallback = cameraManager->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test set camera parameters
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set camera parameters zoom, focus, flash & exposure
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_020, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    input->LockForControl();

    std::vector<float> zoomRatioRange = input->GetSupportedZoomRatioRange();
    if (!zoomRatioRange.empty()) {
        input->SetZoomRatio(zoomRatioRange[0]);
    }

    std::vector<int32_t> exposurebiasRange = input->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        input->SetExposureBias(exposurebiasRange[0]);
    }

    camera_flash_mode_enum_t flash = OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN;
    input->SetFlashMode(flash);

    camera_focus_mode_enum_t focus = OHOS_CAMERA_FOCUS_MODE_AUTO;
    input->SetFocusMode(focus);

    camera_exposure_mode_enum_t exposure = OHOS_CAMERA_EXPOSURE_MODE_AUTO;
    input->SetExposureMode(exposure);

    input->UnlockForControl();

    if (!zoomRatioRange.empty()) {
        EXPECT_EQ(input->GetZoomRatio(), zoomRatioRange[0]);
    }

    if (!exposurebiasRange.empty()) {
        EXPECT_EQ(input->GetExposureValue(), exposurebiasRange[0]);
    }

    EXPECT_EQ(input->GetFlashMode(), flash);
    EXPECT_EQ(input->GetFocusMode(), focus);
    EXPECT_EQ(input->GetExposureMode(), exposure);
}

/*
 * Feature: Framework
 * Function: Test input callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test input callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_021, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    std::shared_ptr<TestDeviceCallback> setCallback = std::make_shared<TestDeviceCallback>("InputCallback");
    input->SetErrorCallback(setCallback);
    std::shared_ptr<ErrorCallback> getCallback = input->GetErrorCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test preview callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_022, TestSize.Level0)
{
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    std::shared_ptr<PreviewCallback> setCallback = std::make_shared<TestPreviewOutputCallback>("PreviewCallback");
    ((sptr<PreviewOutput> &)preview)->SetCallback(setCallback);
    std::shared_ptr<PreviewCallback> getCallback = ((sptr<PreviewOutput> &)preview)->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test photo callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_023, TestSize.Level0)
{
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    std::shared_ptr<PhotoCallback> setCallback = std::make_shared<TestPhotoOutputCallback>("PhotoCallback");
    ((sptr<PhotoOutput> &)photo)->SetCallback(setCallback);
    std::shared_ptr<PhotoCallback> getCallback = ((sptr<PhotoOutput> &)photo)->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test video callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test video callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_024, TestSize.Level0)
{
    sptr<CaptureOutput> video = CreateVideoOutput();
    ASSERT_NE(video, nullptr);

    std::shared_ptr<VideoCallback> setCallback = std::make_shared<TestVideoOutputCallback>("VideoCallback");
    ((sptr<VideoOutput> &)video)->SetCallback(setCallback);
    std::shared_ptr<VideoCallback> getCallback = ((sptr<VideoOutput> &)video)->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test capture session add input with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add input with invalid value
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_025, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureInput> input = nullptr;
    ret = session->AddInput(input);
    EXPECT_NE(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session add output with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add output with invalid value
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_026, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureOutput> preview = nullptr;
    ret = session->AddOutput(preview);
    EXPECT_NE(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding input
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_027, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_NE(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_028, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_NE(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session without begin config
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session without begin config
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_029, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->AddInput(input);
    EXPECT_NE(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_NE(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_NE(ret, 0);

    ret = session->CommitConfig();
    EXPECT_NE(ret, 0);

    ret = session->Start();
    EXPECT_NE(ret, 0);

    ret = ((sptr<PhotoOutput> &)photo)->Capture();
    EXPECT_NE(ret, 0);

    ret = session->Stop();
    EXPECT_NE(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop without adding preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop without adding preview output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_030, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef PRODUCT_M40
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<StreamInfo> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(_, _, true)).Times(0);
    ret = session->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(_)).Times(0);
    ret = session->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, ReleaseStreams(_));
    EXPECT_CALL(*mockCameraDevice, Close());
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + photo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + photo
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_031, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef PRODUCT_M40
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<StreamInfo> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(1, _, true));
    ret = session->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(2, _, false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(1));
    ret = session->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, ReleaseStreams(_));
    EXPECT_CALL(*mockCameraDevice, Close());
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + photo with camera configuration
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + photo with camera configuration
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_032, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;

    std::vector<float> zoomRatioRange = camInput->GetSupportedZoomRatioRange();
    if (!zoomRatioRange.empty()) {
        camInput->LockForControl();
        camInput->SetZoomRatio(zoomRatioRange[0]);
        camInput->UnlockForControl();
    }

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    if (!zoomRatioRange.empty()) {
        EXPECT_CALL(*mockCameraDevice, UpdateSettings(_));
    }
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef PRODUCT_M40
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<StreamInfo> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, ReleaseStreams(_));
    EXPECT_CALL(*mockCameraDevice, Close());
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + video
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_033, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> video = CreateVideoOutput();
    ASSERT_NE(video, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(video);
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef PRODUCT_M40
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<StreamInfo> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(3, _, true));
    ret = session->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(4, _, true));
    ret = ((sptr<VideoOutput> &)video)->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(4));
    ret = ((sptr<VideoOutput> &)video)->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(3));
    ret = session->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, ReleaseStreams(_));
    EXPECT_CALL(*mockCameraDevice, Close());
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session remove output with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove output with null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_034, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureOutput> output = nullptr;
    ret = session->RemoveOutput(output);
    EXPECT_NE(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session remove output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_035, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    sptr<CaptureOutput> video = CreateVideoOutput();
    ASSERT_NE(video, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(video);
    EXPECT_EQ(ret, 0);

    ret = session->RemoveOutput(video);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session remove input with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove input with null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_036, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureInput> input = nullptr;
    ret = session->RemoveInput(input);
    EXPECT_NE(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session remove input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove input
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_037, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->RemoveInput(input);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test photo capture with photo settings
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo capture with photo settings
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_038, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef PRODUCT_M40
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<StreamInfo> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);
    photoSetting->SetQuality(PhotoCaptureSetting::NORMAL_QUALITY);
    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_90);
    EXPECT_EQ(photoSetting->GetQuality(), PhotoCaptureSetting::NORMAL_QUALITY);

    EXPECT_CALL(*mockStreamOperator, Capture(_,
        matchCaptureSetting(photoSetting->GetCaptureMetadataSetting()), false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture(photoSetting);
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, ReleaseStreams(_));
    EXPECT_CALL(*mockCameraDevice, Close());
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test GetFocalLength
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetFocalLength
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_040, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    float focalLength = input->GetFocalLength();
    ASSERT_EQ(focalLength, 1.5);
}


/*
 * Feature: Framework
 * Function: Test SetExposurePoint & GetExposurePoint
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetExposurePoint & GetExposurePoint
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_041, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    Point exposurePoint = {1.0, 2.0};
    input->LockForControl();
    input->SetExposurePoint(exposurePoint);
    input->UnlockForControl();
    ASSERT_EQ((input->GetExposurePoint().x), exposurePoint.x);
    ASSERT_EQ((input->GetExposurePoint().y), exposurePoint.y);
}


/*
 * Feature: Framework
 * Function: Test SetFocusPoint & GetFousPoint
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetFocusPoint & GetFousPoint
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_042, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    Point FocusPoint = {1.0, 2.0};
    input->LockForControl();
    input->SetFocusPoint(FocusPoint);
    input->UnlockForControl();
    ASSERT_EQ((input->GetFocusPoint().x), FocusPoint.x);
    ASSERT_EQ((input->GetFocusPoint().y), FocusPoint.y);
}

/*
 * Feature: Framework
 * Function: Test GetExposureValue and SetExposureBias
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureValue and SetExposureBias with value less then the range
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_043, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();
    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    std::vector<int32_t> exposurebiasRange = input->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        input->LockForControl();
        input->SetExposureBias(exposurebiasRange[0]-1);
        input->UnlockForControl();
    }
    ASSERT_EQ(input->GetExposureValue(), exposurebiasRange[0]);
}

/*
 * Feature: Framework
 * Function: Test GetExposureValue and SetExposureBias
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureValue and SetExposureBias with value between the range
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_044, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    std::vector<int32_t> exposurebiasRange = input->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        input->LockForControl();
        input->SetExposureBias(exposurebiasRange[0]+1);
        input->UnlockForControl();
    }
    EXPECT_TRUE((input->GetExposureValue()>=exposurebiasRange[0] &&
                 input->GetExposureValue()<=exposurebiasRange[1]));
}


/*
 * Feature: Framework
 * Function: Test GetExposureValue and SetExposureBias
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureValue and SetExposureBias with value more then the range
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_045, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    std::vector<int32_t> exposurebiasRange = input->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        input->LockForControl();
        input->SetExposureBias(exposurebiasRange[1]+1);
        input->UnlockForControl();
    }
    ASSERT_EQ(input->GetExposureValue(), exposurebiasRange[1]);
}

/*
 * Feature: Framework
 * Function: Test create metadata output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create metadata output
 * @tc.require: SR000GVK5P SR000GVO5O
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_046, TestSize.Level0)
{
    sptr<CaptureOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);
    metadata->Release();
}

/*
 * Feature: Framework
 * Function: Test set metadata object callback and state callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set metadata object callback and state callback
 * @tc.require: SR000GVK5P SR000GVO5O
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_047, TestSize.Level0)
{
    sptr<MetadataOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);

    std::shared_ptr<MetadataObjectCallback> metadataObjectCallback = std::make_shared<AppMetadataCallback>();
    metadata->SetCallback(metadataObjectCallback);

    std::shared_ptr<MetadataStateCallback> metadataStateCallback = std::make_shared<AppMetadataCallback>();
    metadata->SetCallback(metadataStateCallback);

    metadata->Release();
}

/*
 * Feature: Framework
 * Function: Test get supported metadata object types and set capturing metadata object type as face
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test get supported metadata object types and set capturing metadata object type as face
 * @tc.require: SR000GVK5P SR000GVO5O
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_048, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(metadata);
    EXPECT_EQ(ret, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput> &)metadata;
    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();
    ASSERT_NE(metadataObjectTypes.size(), 0U);

    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> {MetadataObjectType::FACE});
    metaOutput->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + metadata
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + metadata
 * @tc.require: SR000GVK5P SR000GVO5O
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_049, TestSize.Level0)
{
    InSequence s;
    EXPECT_CALL(*mockCameraHostManager, GetCameras(_));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
    std::vector<sptr<CameraInfo>> cameras = cameraManager->GetCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(metadata);
    EXPECT_EQ(ret, 0);

    ((sptr<MetadataOutput> &)metadata)->SetCapturingMetadataObjectTypes(
        std::vector<MetadataObjectType> {MetadataObjectType::FACE});

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, UpdateSettings(_));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef PRODUCT_M40
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<StreamInfo> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(_, _, true));
    ret = session->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(_, _, true));
    ret = ((sptr<MetadataOutput> &)metadata)->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(_));
    ret = ((sptr<MetadataOutput> &)metadata)->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(_));
    ret = session->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, ReleaseStreams(_));
    EXPECT_CALL(*mockCameraDevice, Close());
    session->Release();
}
} // CameraStandard
} // OHOS
