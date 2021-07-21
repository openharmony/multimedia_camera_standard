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

#ifndef OHOS_CAMERA_PHOTO_OUTPUT_H
#define OHOS_CAMERA_PHOTO_OUTPUT_H

#include <iostream>
#include "capture_output.h"
#include "istream_capture.h"

namespace OHOS {
namespace CameraStandard {
class PhotoCallback {
public:
    PhotoCallback() = default;
    virtual ~PhotoCallback() = default;
    virtual void OnCaptureStarted(int32_t captureID) = 0;
    virtual void OnCaptureEnded(int32_t captureID, int32_t frameCount) = 0;
    virtual void OnFrameShutter(int32_t captureId, uint64_t timestamp) = 0;
    virtual void onCaptureError(int32_t captureId, int32_t errorCode) = 0;
};
class PhotoCaptureSetting {
public:
    enum QualityLevel {
        HIGH_QUALITY = 1,
        NORMAL_QUALITY,
        LOW_QUALITY
    };
    enum RotationConfig {
        Rotation_0 = 0,
        Rotation_90 = 90,
        Rotation_180 = 180,
        Rotation_270 = 270
    };
    PhotoCaptureSetting() = default;
    virtual ~PhotoCaptureSetting() = default;
    QualityLevel GetQuaility();
    void SetQuaility(QualityLevel qualityLevel);
    RotationConfig GetRotation();
    void SetRotation(RotationConfig rotationvalue);
    //Location &GetLocation();
    //void SetLocation(Location &location);
    bool IsMirrored();
    void SetMirror(bool enable);
};

class PhotoOutput : public CaptureOutput {
public:
    PhotoOutput (sptr<IStreamCapture> &streamCapture);
    sptr<IStreamCapture> GetStreamCapture();
    void SetCallback(std::shared_ptr<PhotoCallback> callback);
    PhotoCaptureSetting &GetCaptureSetting();
    int32_t Capture(std::shared_ptr<PhotoCaptureSetting> photoCaptureSettings);
    int32_t Capture();
    int32_t CancelCapture();
    void Release() override;

private:
    sptr<IStreamCapture> streamCapture_;
    std::shared_ptr<PhotoCallback> appCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_PHOTO_OUTPUT_H
