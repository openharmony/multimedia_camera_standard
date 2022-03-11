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

#ifndef CAMERA_FRAMEWORK_UNITTEST_H
#define CAMERA_FRAMEWORK_UNITTEST_H

#include "gtest/gtest.h"
#include "hcamera_service.h"
#include "input/camera_manager.h"

namespace OHOS {
namespace CameraStandard {
class MockStreamOperator;
class MockCameraDevice;
class MockHCameraHostManager;
class CameraFrameworkUnitTest : public testing::Test {
public:
    static const int32_t PHOTO_DEFAULT_WIDTH = 1280;
    static const int32_t PHOTO_DEFAULT_HEIGHT = 960;
    static const int32_t PREVIEW_DEFAULT_WIDTH = 640;
    static const int32_t PREVIEW_DEFAULT_HEIGHT = 480;
    static const int32_t VIDEO_DEFAULT_WIDTH = 640;
    static const int32_t VIDEO_DEFAULT_HEIGHT = 360;

    sptr<MockStreamOperator> mockStreamOperator;
    sptr<MockCameraDevice> mockCameraDevice;
    sptr<MockHCameraHostManager> mockCameraHostManager;
    sptr<CameraManager> cameraManager;
    /* SetUpTestCase:The preset action of the test suite is executed before the first TestCase */
    static void SetUpTestCase(void);

    /* TearDownTestCase:The test suite cleanup action is executed after the last TestCase */
    static void TearDownTestCase(void);

    /* SetUp:Execute before each test case */
    void SetUp();

    /* TearDown:Execute after each test case */
    void TearDown();

    sptr<CaptureOutput> CreatePhotoOutput(int32_t width = PHOTO_DEFAULT_WIDTH, int32_t height = PHOTO_DEFAULT_HEIGHT);
    sptr<CaptureOutput> CreatePreviewOutput(int32_t width = PREVIEW_DEFAULT_WIDTH,
                                            int32_t height = PREVIEW_DEFAULT_HEIGHT);
    sptr<CaptureOutput> CreateVideoOutput(int32_t width = VIDEO_DEFAULT_WIDTH, int32_t height = VIDEO_DEFAULT_HEIGHT);
};
} // CameraStandard
} // OHOS
#endif // CAMERA_FRAMEWORK_UNITTEST_H
