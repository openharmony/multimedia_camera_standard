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

using namespace testing::ext;
using ::testing::A;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

namespace OHOS {
namespace CameraStandard {
class MockStreamOperator : public IRemoteStub<Camera::IStreamOperator> {
public:
    MockStreamOperator()
    {
        ON_CALL(*this, CreateStreams(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, ReleaseStreams(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, CommitStreams(_, _)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, Capture(_, _, _)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, CancelCapture(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported(_, _, A<const std::shared_ptr<Camera::StreamInfo> &>(), _))
            .WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported(_, _, A<const std::vector<std::shared_ptr<Camera::StreamInfo>> &>(), _))
            .WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, GetStreamAttributes(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, AttachBufferQueue(_, _)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, DetachBufferQueue(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, ChangeToOfflineStream(_, _, _)).WillByDefault(Return(Camera::NO_ERROR));
    }
    ~MockStreamOperator() {}
    MOCK_METHOD1(CreateStreams, Camera::CamRetCode(
        const std::vector<std::shared_ptr<Camera::StreamInfo>> &streamInfos));
    MOCK_METHOD1(ReleaseStreams, Camera::CamRetCode(const std::vector<int> &streamIds));
    MOCK_METHOD1(CancelCapture, Camera::CamRetCode(int captureId));
    MOCK_METHOD1(GetStreamAttributes, Camera::CamRetCode(
        std::vector<std::shared_ptr<Camera::StreamAttribute>> &attributes));
    MOCK_METHOD1(DetachBufferQueue, Camera::CamRetCode(int streamId));
    MOCK_METHOD2(CommitStreams, Camera::CamRetCode(Camera::OperationMode mode,
        const std::shared_ptr<CameraStandard::CameraMetadata> &modeSetting));
    MOCK_METHOD2(AttachBufferQueue, Camera::CamRetCode(int streamId,
        const OHOS::sptr<OHOS::IBufferProducer> &producer));
    MOCK_METHOD3(Capture, Camera::CamRetCode(int captureId, const std::shared_ptr<Camera::CaptureInfo> &info,
        bool isStreaming));
    MOCK_METHOD3(ChangeToOfflineStream, Camera::CamRetCode(const std::vector<int> &streamIds,
        OHOS::sptr<Camera::IStreamOperatorCallback> &callback,
        OHOS::sptr<Camera::IOfflineStreamOperator> &offlineOperator));
    MOCK_METHOD4(IsStreamsSupported, Camera::CamRetCode(Camera::OperationMode mode,
        const std::shared_ptr<CameraStandard::CameraMetadata> &modeSetting,
        const std::shared_ptr<Camera::StreamInfo> &info, Camera::StreamSupportType &type));
    MOCK_METHOD4(IsStreamsSupported, Camera::CamRetCode(Camera::OperationMode mode,
        const std::shared_ptr<CameraStandard::CameraMetadata> &modeSetting,
        const std::vector<std::shared_ptr<Camera::StreamInfo>> &info, Camera::StreamSupportType &type));
};

class MockCameraDevice : public IRemoteStub<Camera::ICameraDevice> {
public:
    MockCameraDevice()
    {
        streamOperator = new MockStreamOperator();
        ON_CALL(*this, GetStreamOperator).WillByDefault([this](
            const OHOS::sptr<Camera::IStreamOperatorCallback> &callback,
            OHOS::sptr<Camera::IStreamOperator> &pStreamOperator) {
            pStreamOperator = streamOperator;
            return Camera::NO_ERROR;
        });
        ON_CALL(*this, UpdateSettings(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, SetResultMode(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, GetEnabledResults(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, EnableResult(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, DisableResult(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, DisableResult(_)).WillByDefault(Return(Camera::NO_ERROR));
        ON_CALL(*this, Close()).WillByDefault(Return());
    }
    ~MockCameraDevice() {}
    MOCK_METHOD0(Close, void());
    MOCK_METHOD1(UpdateSettings, Camera::CamRetCode(const std::shared_ptr<Camera::CameraSetting> &settings));
    MOCK_METHOD1(SetResultMode, Camera::CamRetCode(const Camera::ResultCallbackMode &mode));
    MOCK_METHOD1(GetEnabledResults, Camera::CamRetCode(std::vector<Camera::MetaType> &results));
    MOCK_METHOD1(EnableResult, Camera::CamRetCode(const std::vector<Camera::MetaType> &results));
    MOCK_METHOD1(DisableResult, Camera::CamRetCode(const std::vector<Camera::MetaType> &results));
    MOCK_METHOD2(GetStreamOperator, Camera::CamRetCode(const OHOS::sptr<Camera::IStreamOperatorCallback> &callback,
        OHOS::sptr<Camera::IStreamOperator> &pStreamOperator));
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
                                                            std::shared_ptr<CameraMetadata> &ability) {
            int32_t itemCount = 10;
            int32_t dataSize = 100;
            ability = std::make_shared<CameraMetadata>(itemCount, dataSize);
            int32_t streams[9] = {
                OHOS_CAMERA_FORMAT_YCRCB_420_SP, CameraFrameworkUnitTest::PREVIEW_DEFAULT_WIDTH,
                CameraFrameworkUnitTest::PREVIEW_DEFAULT_HEIGHT, OHOS_CAMERA_FORMAT_YCRCB_420_SP,
                CameraFrameworkUnitTest::VIDEO_DEFAULT_WIDTH, CameraFrameworkUnitTest::VIDEO_DEFAULT_HEIGHT,
                OHOS_CAMERA_FORMAT_JPEG, CameraFrameworkUnitTest::PHOTO_DEFAULT_WIDTH,
                CameraFrameworkUnitTest::PHOTO_DEFAULT_HEIGHT
            };
            ability->addEntry(OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, streams,
                              sizeof(streams) / sizeof(streams[0]));
            return CAMERA_OK;
        });
        ON_CALL(*this, OpenCameraDevice).WillByDefault([this](std::string &cameraId,
                                                            const sptr<Camera::ICameraDeviceCallback> &callback,
                                                            sptr<Camera::ICameraDevice> &pDevice) {
            pDevice = cameraDevice;
            return CAMERA_OK;
        });
        ON_CALL(*this, SetFlashlight(_, _)).WillByDefault(Return(CAMERA_OK));
        ON_CALL(*this, SetCallback(_)).WillByDefault(Return(CAMERA_OK));
    }
    ~MockHCameraHostManager() {}
    MOCK_METHOD1(GetCameras, int32_t(std::vector<std::string> &cameraIds));
    MOCK_METHOD1(SetCallback, int32_t(sptr<Camera::ICameraHostCallback> &callback));
    MOCK_METHOD2(GetCameraAbility, int32_t(std::string &cameraId, std::shared_ptr<CameraMetadata> &ability));
    MOCK_METHOD2(SetFlashlight, int32_t(const std::string &cameraId, bool isEnable));
    MOCK_METHOD3(OpenCameraDevice, int32_t(std::string &cameraId,
        const sptr<Camera::ICameraDeviceCallback> &callback, sptr<Camera::ICameraDevice> &pDevice));
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

sptr<CaptureOutput> CameraFrameworkUnitTest::CreatePhotoOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_JPEG));
    return cameraManager->CreatePhotoOutput(surface);
}

sptr<CaptureOutput> CameraFrameworkUnitTest::CreatePreviewOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
    return cameraManager->CreatePreviewOutput(surface);
}

sptr<CaptureOutput> CameraFrameworkUnitTest::CreateVideoOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    surface->SetDefaultWidthAndHeight(width, height);
    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
    return cameraManager->CreateVideoOutput(surface);
}

void CameraFrameworkUnitTest::SetUpTestCase(void) {}
void CameraFrameworkUnitTest::TearDownTestCase(void) {}

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
    return (arg->captureSetting_ == captureSetting);
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
 * CaseDescription: Test set camera parameters
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

    camera_flash_mode_enum_t flash = OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN;
    input->SetFlashMode(flash);

    camera_af_mode_t focus = OHOS_CAMERA_AF_MODE_AUTO;
    input->SetFocusMode(focus);

    camera_ae_mode_t exposure = OHOS_CAMERA_AE_MODE_ON;
    input->SetExposureMode(exposure);

    input->UnlockForControl();

    if (!zoomRatioRange.empty()) {
        EXPECT_TRUE(input->GetZoomRatio() == zoomRatioRange[0]);
    }
    EXPECT_TRUE(input->GetFlashMode() == flash);
    EXPECT_TRUE(input->GetFocusMode() == focus);
    EXPECT_TRUE(input->GetExposureMode() == exposure);
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
    EXPECT_TRUE(ret == 0);

    sptr<CaptureInput> input = nullptr;
    ret = session->AddInput(input);
    EXPECT_TRUE(ret != 0);
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
    EXPECT_TRUE(ret == 0);

    sptr<CaptureOutput> preview = nullptr;
    ret = session->AddOutput(preview);
    EXPECT_TRUE(ret != 0);
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
    EXPECT_TRUE(ret == 0);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    ret = session->AddOutput(preview);
    EXPECT_TRUE(ret == 0);

    ret = session->CommitConfig();
    EXPECT_TRUE(ret != 0);
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
    EXPECT_TRUE(ret == 0);

    ret = session->AddInput(input);
    EXPECT_TRUE(ret == 0);

    ret = session->CommitConfig();
    EXPECT_TRUE(ret != 0);
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
    EXPECT_TRUE(ret != 0);

    ret = session->AddOutput(preview);
    EXPECT_TRUE(ret != 0);

    ret = session->AddOutput(photo);
    EXPECT_TRUE(ret != 0);

    ret = session->CommitConfig();
    EXPECT_TRUE(ret != 0);

    ret = session->Start();
    EXPECT_TRUE(ret != 0);

    ret = ((sptr<PhotoOutput> &)photo)->Capture();
    EXPECT_TRUE(ret != 0);

    ret = session->Stop();
    EXPECT_TRUE(ret != 0);
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
    EXPECT_TRUE(ret == 0);

    ret = session->AddInput(input);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(photo);
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(Camera::ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef BALTIMORE_CAMERA
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<std::shared_ptr<Camera::StreamInfo>> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, Capture(PREVIEW_CAPTURE_ID_START, _, true)).Times(0);
    ret = session->Start();
    EXPECT_TRUE(ret != 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(PREVIEW_CAPTURE_ID_START)).Times(0);
    ret = session->Stop();
    EXPECT_TRUE(ret != 0);

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
    EXPECT_TRUE(ret == 0);

    ret = session->AddInput(input);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(preview);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(photo);
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(Camera::ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef BALTIMORE_CAMERA
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<std::shared_ptr<Camera::StreamInfo>> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, Capture(PREVIEW_CAPTURE_ID_START, _, true));
    ret = session->Start();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, Capture(PHOTO_CAPTURE_ID_START, _, false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(PREVIEW_CAPTURE_ID_START));
    ret = session->Stop();
    EXPECT_TRUE(ret == 0);

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
    EXPECT_TRUE(ret == 0);

    ret = session->AddInput(input);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(preview);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(photo);
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    if (!zoomRatioRange.empty()) {
        EXPECT_CALL(*mockCameraDevice, UpdateSettings(_));
    }
    EXPECT_CALL(*mockCameraDevice, SetResultMode(Camera::ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef BALTIMORE_CAMERA
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<std::shared_ptr<Camera::StreamInfo>> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_TRUE(ret == 0);

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
    EXPECT_TRUE(ret == 0);

    ret = session->AddInput(input);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(preview);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(video);
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(Camera::ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef BALTIMORE_CAMERA
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<std::shared_ptr<Camera::StreamInfo>> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, Capture(PREVIEW_CAPTURE_ID_START, _, true));
    ret = session->Start();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, Capture(VIDEO_CAPTURE_ID_START, _, true));
    ret = ((sptr<VideoOutput> &)video)->Start();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(VIDEO_CAPTURE_ID_START));
    ret = ((sptr<VideoOutput> &)video)->Stop();
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(PREVIEW_CAPTURE_ID_START));
    ret = session->Stop();
    EXPECT_TRUE(ret == 0);

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
    EXPECT_TRUE(ret == 0);

    sptr<CaptureOutput> output = nullptr;
    ret = session->RemoveOutput(output);
    EXPECT_TRUE(ret != 0);
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
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(video);
    EXPECT_TRUE(ret == 0);

    ret = session->RemoveOutput(video);
    EXPECT_TRUE(ret == 0);
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
    EXPECT_TRUE(ret == 0);

    sptr<CaptureInput> input = nullptr;
    ret = session->RemoveInput(input);
    EXPECT_TRUE(ret != 0);
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
    EXPECT_TRUE(ret == 0);

    ret = session->AddInput(input);
    EXPECT_TRUE(ret == 0);

    ret = session->RemoveInput(input);
    EXPECT_TRUE(ret == 0);
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
    EXPECT_TRUE(ret == 0);

    ret = session->AddInput(input);
    EXPECT_TRUE(ret == 0);

    ret = session->AddOutput(photo);
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(Camera::ON_CHANGED));
    EXPECT_CALL(*mockCameraDevice, GetStreamOperator(_, _));
    EXPECT_CALL(*mockCameraHostManager, GetCameraAbility(_, _));
#ifndef BALTIMORE_CAMERA
    EXPECT_CALL(*mockStreamOperator, IsStreamsSupported(_, _,
        A<const std::vector<std::shared_ptr<Camera::StreamInfo>> &>(), _));
#endif
    EXPECT_CALL(*mockStreamOperator, CreateStreams(_));
    EXPECT_CALL(*mockStreamOperator, CommitStreams(_, _));
    ret = session->CommitConfig();
    EXPECT_TRUE(ret == 0);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);
    photoSetting->SetQuality(PhotoCaptureSetting::NORMAL_QUALITY);
    EXPECT_TRUE(photoSetting->GetRotation() == PhotoCaptureSetting::Rotation_90);
    EXPECT_TRUE(photoSetting->GetQuality() == PhotoCaptureSetting::NORMAL_QUALITY);

    EXPECT_CALL(*mockStreamOperator, Capture(PHOTO_CAPTURE_ID_START,
        matchCaptureSetting(photoSetting->GetCaptureMetadataSetting()), false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture(photoSetting);
    EXPECT_TRUE(ret == 0);

    EXPECT_CALL(*mockStreamOperator, ReleaseStreams(_));
    EXPECT_CALL(*mockCameraDevice, Close());
    session->Release();
}
} // CameraStandard
} // OHOS
