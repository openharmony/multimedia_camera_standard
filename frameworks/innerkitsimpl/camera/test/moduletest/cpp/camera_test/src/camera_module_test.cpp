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

#include "camera_module_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

void CameraModuleTest::SetUpTestCase(void) {}
void CameraModuleTest::TearDownTestCase(void) {}

/* SetUp:Execute before each test case */
void CameraModuleTest::SetUp()
{
    /* CameraSetUp */
    g_onGetCameraAbilityFlag = false;
    g_onConfigureFlag = false;
    g_onGetSupportedSizesFlag = false;
    /* CameraDeviceCallBack */
    g_onCameraAvailableFlag = false;
    g_onCameraUnavailableFlag = false;
    /* CameraStateCallback */
    g_onCreatedFlag = false;
    g_onCreateFailedFlag = false;
    g_onConfiguredFlag = false;
    g_onConfigureFailedFlag = false;
    g_onReleasedFlag = false;
    /* FrameStateCallback */
    g_onCaptureTriggerAbortedFlag = false;
    g_onCaptureTriggerCompletedFlag = false;
    g_onFrameFinishedFlag = false;
    g_onGetFrameConfigureType = false;
    g_onFrameErrorFlag = false;
    g_onFrameProgressedFlag = false;
}

void CameraModuleTest::TearDown(void) {}

/* *
 * Get camera Id
 */
void CameraModuleTest::GetCameraId(CameraKit *cameraKit, list<string> &camList,
    string &camId)
{
    camList = cameraKit->GetCameraIds();
    for (auto &cam : camList) {
        const CameraAbility *ability = cameraKit->GetCameraAbility(cam);
        ASSERT_NE(ability, nullptr);
        g_onGetCameraAbilityFlag = true;
        /* find camera which fits user's ability */
        list<CameraPicSize> sizeList = ability->GetSupportedSizes();
        if (sizeList.size() != 0) {
            g_onGetSupportedSizesFlag = true;
        }
        for (auto &pic : sizeList) {
            if (pic.width == WIDTH && pic.height == HEIGHT) {
                /* 1920:width,1080:height */
                camId = cam;
                break;
            }
        }
    }
    if (camId.empty()) {
        cout << "No available camera.(1080p wanted)" << endl;
        return;
    }
}

class SampleFrameStateCallback : public FrameStateCallback {
    void SampleSaveCapture(const char &buffer, uint32_t size)
    {
        ofstream pic("/data/Capture_sample.jpg", ofstream::out);
        pic.write(&buffer, size);
        pic.close();
    }

    void OnFrameFinished(const Camera &camera, const FrameConfig &fc,
        const FrameResult &result) override
    {
        g_onFrameProgressedFlag = true;
        if (((FrameConfig &) fc).GetFrameConfigType() == FRAME_CONFIG_CAPTURE) {
            g_onGetFrameConfigureType = true;
            list<Surface *> surfaceList = ((FrameConfig &) fc).GetSurfaces();
            for (Surface *surface : surfaceList) {
                sptr<SurfaceBuffer> buffer;
                int32_t flushFence;
                int64_t timestamp;
                Rect damage;
                SurfaceError ret = surface->AcquireBuffer(buffer, flushFence, timestamp, damage);
                if (ret == SURFACE_ERROR_OK) {
                    char *virtAddr = static_cast<char *>(buffer->GetVirAddr());
                    if (virtAddr != nullptr) {
                        int32_t bufferSize = stoi(surface->GetUserData("surface_buffer_size"));
                        SampleSaveCapture(*virtAddr, bufferSize);
                    } else {
                        g_onFrameErrorFlag = true;
                    }
                    surface->ReleaseBuffer(buffer, -1);
                } else {
                    g_onFrameErrorFlag = true;
                }
            }
            delete &fc;
        } else {
            g_onFrameErrorFlag = true;
        }
        g_onFrameFinishedFlag = true;
    }
};

class SurfaceListener : public IBufferConsumerListener {
public:
    void OnBufferAvailable() override {}
};

class SampleCameraStateMng : public CameraStateCallback {
public:
    Camera *cam_ = nullptr;
    SampleCameraStateMng() = delete;
    explicit SampleCameraStateMng(EventHandler &eventHdlr) : eventHdlr_(eventHdlr)
    {
        captureConSurface = nullptr;
    }

    ~SampleCameraStateMng()
    {
        if (cam_) {
            cam_->Release();
        }
    }

    void OnCreated(const Camera &c) override
    {
        g_onCreatedFlag = true;
        CameraConfig *config = CameraConfig::CreateCameraConfig();
        if (config != nullptr) {
            config->SetFrameStateCallback(fsCb_, eventHdlr_);
            ((Camera &)c).Configure(*config);
            g_onConfigureFlag = true;
            cam_ = (Camera *) &c;
        } else {
            cout << "Create Camera Config return null" << endl;
        }
    }

    void OnCreateFailed(const std::string cameraId, int32_t errorCode) override
    {
        g_onCreateFailedFlag = true;
    }

    void OnReleased(const Camera &c) override
    {
        g_onReleasedFlag = true;
    }

    void OnConfigured(const Camera &c) override
    {
        g_onConfiguredFlag = true;
    }

    void OnConfigureFailed(const std::string cameraId, int32_t errorCode) override
    {
        g_onConfigureFailedFlag = true;
    }

    void Capture()
    {
        if (cam_ == nullptr) {
            cout << "Camera capture is not ready." << endl;
            return;
        }
        FrameConfig *fc = new FrameConfig(FRAME_CONFIG_CAPTURE);

        captureConSurface = Surface::CreateSurfaceAsConsumer();
        if (captureConSurface == nullptr) {
            delete fc;
            cout << "CreateSurface failed" << endl;
            return;
        }

        sptr<IBufferConsumerListener> listener = new SurfaceListener();
        captureConSurface->RegisterConsumerListener(listener);
        auto refSurface = captureConSurface; // to keep the reference count

        int usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
        captureConSurface->SetUserData("surface_width", "1920");
        captureConSurface->SetUserData("surface_height", "1080");
        captureConSurface->SetUserData("surface_stride_Alignment", "8");
        captureConSurface->SetUserData("surface_format", to_string(PIXEL_FMT_RGBA_8888));
        captureConSurface->SetUserData("surface_usage", to_string(usage));
        captureConSurface->SetUserData("surface_timeout", "0");
        fc->AddSurface(*refSurface);
        int32_t ret = cam_->TriggerSingleCapture(*fc);
        if (ret != MEDIA_OK) {
            delete fc;
        } else {
            g_onCaptureTriggerCompletedFlag = true;
        }
    }

private:
    EventHandler &eventHdlr_;
    SampleFrameStateCallback fsCb_;
    sptr<Surface> captureConSurface;
};

class SampleCameraDeviceCallback : public CameraDeviceCallback {
public:
    SampleCameraDeviceCallback() {}

    ~SampleCameraDeviceCallback() {}

    void OnCameraStatus(const std::string cameraId, int32_t status) override
    {
        if (status == CAMERA_DEVICE_STATE_AVAILABLE) {
            g_onCameraAvailableFlag = true;
        } else if (status == CAMERA_DEVICE_STATE_UNAVAILABLE) {
            g_onCameraUnavailableFlag = true;
        }
    }
};

/*
 * Feature: Camera
 * Function: GetCameraIds
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Get Camera IDs, main camera should be returned
 */
HWTEST_F(CameraModuleTest, media_camera_GetCameraId_test_001, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    EXPECT_FALSE(camId.empty());
    EXPECT_EQ("main", camId);
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: GetCameraAbility
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Camera Ability should not be null
 */
HWTEST_F(CameraModuleTest, media_camera_GetCameraAbility_test_001, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    SampleCameraStateMng camStateMng(eventHdlr);
    EXPECT_EQ(g_onGetCameraAbilityFlag, true);
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: CreateCamera
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CreateCamera should create "main" CameraImpl object successfully
 */
HWTEST_F(CameraModuleTest, media_camera_CreateCamerakit_test_001, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    SampleCameraStateMng camStateMng(eventHdlr);
    sleep(1);
    cameraKit->CreateCamera(camId, camStateMng, eventHdlr);
    sleep(1);
    EXPECT_EQ(g_onCreatedFlag, true);
    EXPECT_NE(g_onCreateFailedFlag, true);
    EXPECT_NE(camStateMng.cam_, nullptr);
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: CreateCamera
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: CreateCamera should not create "0" camera object
 */
HWTEST_F(CameraModuleTest, media_camera_CreateCamerakit_test_002, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    cameraKit = CameraKit::GetInstance();
    string camId = "0";
    EventHandler eventHdlr;
    SampleCameraStateMng camStateMng(eventHdlr);
    sleep(1);
    cameraKit->CreateCamera(camId, camStateMng, eventHdlr);
    sleep(1);
    EXPECT_EQ(g_onCreatedFlag, false);
    EXPECT_EQ(g_onCreateFailedFlag, true);
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: CameraDeviceCallback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: creation of CameraDeviceCallback object should not fail
 */
HWTEST_F(CameraModuleTest, media_camera_DeviceCallback_test_001, TestSize.Level1)
{
    SampleCameraDeviceCallback *deviceCallback = nullptr;
    deviceCallback = new SampleCameraDeviceCallback();
    EXPECT_NE(nullptr, deviceCallback);
    delete deviceCallback;
    deviceCallback = nullptr;
}

/*
 * Feature: Camera
 * Function: RegisterCameraDeviceCallback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Registering of CameraDeviceCallback should be success
 */
HWTEST_F(CameraModuleTest, media_camera_RegisterCameraDeviceCallback_test_001,
                            TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    SampleCameraStateMng camStateMng(eventHdlr);
    SampleCameraDeviceCallback *deviceCallback = nullptr;
    deviceCallback = new SampleCameraDeviceCallback();
    ASSERT_NE(nullptr, deviceCallback);
    cameraKit->RegisterCameraDeviceCallback(*deviceCallback, eventHdlr);
    sleep(1);
    EXPECT_EQ(g_onCameraAvailableFlag, true);
    delete deviceCallback;
    deviceCallback = nullptr;
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: UnregisterCameraDeviceCallback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Unregistering of CameraDeviceCallback should be success
 */
HWTEST_F(CameraModuleTest, media_camera_UnregisterCameraDeviceCallback_test_001,
                            TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    SampleCameraStateMng camStateMng(eventHdlr);
    SampleCameraDeviceCallback *deviceCallback = nullptr;
    deviceCallback = new SampleCameraDeviceCallback();
    ASSERT_NE(nullptr, deviceCallback);
    cameraKit->RegisterCameraDeviceCallback(*deviceCallback, eventHdlr);
    sleep(1);
    EXPECT_EQ(g_onCameraAvailableFlag, true);
    cameraKit->UnregisterCameraDeviceCallback(*deviceCallback);
    sleep(1);
    EXPECT_EQ(g_onCameraUnavailableFlag, false);
    delete deviceCallback;
    deviceCallback = nullptr;
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: GetSupportedSizes
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Checks if GetSupportedSizes returns the correct resolution
 */
HWTEST_F(CameraModuleTest, media_camera_GetSupportedSizes_test_001, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    EXPECT_EQ(g_onGetSupportedSizesFlag, true);
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: GetCameraId
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Checks if setting of cameraconfig to camera is success
 */
HWTEST_F(CameraModuleTest, media_camera_OnConfigure_test_001, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    SampleCameraStateMng camStateMng(eventHdlr);
    sleep(1);
    cameraKit->CreateCamera(camId, camStateMng, eventHdlr);
    sleep(1);
    EXPECT_EQ(g_onConfigureFlag, true);
    EXPECT_NE(camStateMng.cam_, nullptr);
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: FrameConfig
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Checks if creation of FrameConfig object is success
 */
HWTEST_F(CameraModuleTest, media_camera_FrameConfig_test_001, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    SampleCameraStateMng camStateMng(eventHdlr);
    cameraKit->CreateCamera(camId, camStateMng, eventHdlr);
    sleep(1);
    FrameConfig *fc = new FrameConfig(FRAME_CONFIG_CAPTURE);
    EXPECT_NE(fc, nullptr);
    delete fc;
    cameraKit = nullptr;
}

/*
 * Feature: Camera
 * Function: GetCameraId
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Checks if the capture is done successfully
 */
HWTEST_F(CameraModuleTest, media_camera_Capture_test_003, TestSize.Level1)
{
    CameraKit *cameraKit = CameraKit::GetInstance();
    list<string> camList;
    string camId;
    EventHandler eventHdlr;
    CameraModuleTest::GetCameraId(cameraKit, camList, camId);
    SampleCameraStateMng camStateMng(eventHdlr);
    sleep(1);
    cameraKit->CreateCamera(camId, camStateMng, eventHdlr);
    sleep(1);
    camStateMng.Capture();
    sleep(1);
    EXPECT_EQ(g_onCaptureTriggerCompletedFlag, true);
    EXPECT_EQ(g_onGetFrameConfigureType, true);
    EXPECT_EQ(g_onFrameFinishedFlag, true);
    EXPECT_NE(g_onFrameErrorFlag, true);
    cameraKit = nullptr;
}
