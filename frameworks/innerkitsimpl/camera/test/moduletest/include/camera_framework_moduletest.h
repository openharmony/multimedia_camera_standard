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

#ifndef CAMERA_FRAMEWORK_MODULETEST_H
#define CAMERA_FRAMEWORK_MODULETEST_H

#include "gtest/gtest.h"
#include "input/camera_input.h"
#include "input/camera_manager.h"
#include "session/capture_session.h"

namespace OHOS {
namespace CameraStandard {
class CameraFrameworkModuleTest : public testing::Test {
public:
    static const int32_t PHOTO_DEFAULT_WIDTH = 1280;
    static const int32_t PHOTO_DEFAULT_HEIGHT = 960;
    static const int32_t PREVIEW_DEFAULT_WIDTH = 640;
    static const int32_t PREVIEW_DEFAULT_HEIGHT = 480;
    static const int32_t VIDEO_DEFAULT_WIDTH = 640;
    static const int32_t VIDEO_DEFAULT_HEIGHT = 360;
    camera_format_t previewFormat_;
    camera_format_t photoFormat_;
    camera_format_t videoFormat_;
    int32_t previewWidth_;
    int32_t previewHeight_;
    int32_t photoWidth_;
    int32_t photoHeight_;
    int32_t videoWidth_;
    int32_t videoHeight_;
    sptr<CameraManager> manager_;
    sptr<CaptureSession> session_;
    sptr<CaptureInput> input_;
    std::vector<sptr<CameraInfo>> cameras_;
    std::vector<camera_format_t> previewFormats_;
    std::vector<camera_format_t> photoFormats_;
    std::vector<camera_format_t> videoFormats_;

    /* SetUpTestCase:The preset action of the test suite is executed before the first TestCase */
    static void SetUpTestCase(void);

    /* TearDownTestCase:The test suite cleanup action is executed after the last TestCase */
    static void TearDownTestCase(void);

    /* SetUp:Execute before each test case */
    void SetUp();

    /* TearDown:Execute after each test case */
    void TearDown();

    sptr<CaptureOutput> CreatePreviewOutput(bool customPreview, int32_t width, int32_t height);
    sptr<CaptureOutput> CreatePreviewOutput();
    sptr<CaptureOutput> CreatePhotoOutput(int32_t width, int32_t height);
    sptr<CaptureOutput> CreatePhotoOutput();
    sptr<CaptureOutput> CreateVideoOutput(int32_t width, int32_t height);
    sptr<CaptureOutput> CreateVideoOutput();
    void SetCameraParameters(sptr<CameraInput> &camInput);
    void TestCallbacks(sptr<CameraInfo> &cameraInfo, bool video);
    void TestSupportedResolution(int32_t previewWidth, int32_t previewHeight, int32_t photoWidth,
                                 int32_t photoHeight, int32_t videoWidth, int32_t videoHeight);
    void TestUnSupportedResolution(int32_t previewWidth, int32_t previewHeight, int32_t photoWidth,
                                   int32_t photoHeight, int32_t videoWidth, int32_t videoHeight);
};
} // CameraStandard
} // OHOS
#endif // CAMERA_FRAMEWORK_MODULETEST_H
