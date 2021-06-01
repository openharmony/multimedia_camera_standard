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

#ifndef CAMERA_MODULE_TEST_H
#define CAMERA_MODULE_TEST_H

#include <fstream>

#include "gtest/gtest.h"
#include "camera_kit.h"
#include "display_type.h"

// VideoSize
enum TestVideoSize {
    WIDTH = 1920,
    HEIGHT = 1080,
};

// CameraSetupFlag
bool g_onGetCameraAbilityFlag;
bool g_onConfigureFlag;
bool g_onGetSupportedSizesFlag;
// CameraDeviceCallBack Flag
bool g_onCameraAvailableFlag;
bool g_onCameraUnavailableFlag;
// CameraStateCallback
bool g_onCreatedFlag;
bool g_onCreateFailedFlag;
bool g_onReleasedFlag;
bool g_onConfiguredFlag;
bool g_onConfigureFailedFlag;
// FrameStateCallback
bool g_onCaptureTriggerAbortedFlag;
bool g_onCaptureTriggerCompletedFlag;
bool g_onCaptureTriggerStartedFlag;
bool g_onGetFrameConfigureType;
bool g_onFrameFinishedFlag;
bool g_onFrameErrorFlag;
bool g_onFrameProgressedFlag;

class CameraModuleTest : public testing::Test {
public:
    /* SetUpTestCase:The preset action of the test suite is executed before the first TestCase */
    static void SetUpTestCase(void);

    /* TearDownTestCase:The test suite cleanup action is executed after the last TestCase */
    static void TearDownTestCase(void);

    /* SetUp:Execute before each test case */
    void SetUp();

    /* TearDown:Execute after each test case */
    void TearDown();

    /* Gets the camera Id */
    static void GetCameraId(OHOS::Media::CameraKit *cameraKit,
        std::list<std::string> &camList, std::string &camId);
};
#endif // CAMERA_MODULE_TEST_H
