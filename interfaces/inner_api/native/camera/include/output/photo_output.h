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
#include "camera_metadata_info.h"
#include "capture_output.h"
#include "istream_capture.h"

namespace OHOS {
namespace CameraStandard {
class PhotoCallback {
public:
    PhotoCallback() = default;
    virtual ~PhotoCallback() = default;
    virtual void OnCaptureStarted(const int32_t captureID) const = 0;
    virtual void OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const = 0;
    virtual void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const = 0;
    virtual void OnCaptureError(const int32_t captureId, const int32_t errorCode) const = 0;
};
class PhotoCaptureSetting {
public:
    enum QualityLevel {
        HIGH_QUALITY = 0,
        NORMAL_QUALITY,
        LOW_QUALITY
    };
    enum RotationConfig {
        Rotation_0 = 0,
        Rotation_90 = 90,
        Rotation_180 = 180,
        Rotation_270 = 270
    };
    PhotoCaptureSetting();
    virtual ~PhotoCaptureSetting() = default;
    QualityLevel GetQuality();
    void SetQuality(QualityLevel qualityLevel);
    RotationConfig GetRotation();
    void SetRotation(RotationConfig rotationvalue);
    void SetGpsLocation(double latitude, double longitude);
    bool IsMirrored();
    void SetMirror(bool enable);
    std::shared_ptr<CameraMetadata> GetCaptureMetadataSetting();

private:
    std::shared_ptr<CameraMetadata> captureMetadataSetting_;
};

class PhotoOutput : public CaptureOutput {
public:
    PhotoOutput(sptr<IStreamCapture> &streamCapture);
    sptr<IStreamCapture> GetStreamCapture();
    void SetCallback(std::shared_ptr<PhotoCallback> callback);
    int32_t Capture(std::shared_ptr<PhotoCaptureSetting> photoCaptureSettings);
    int32_t Capture();
    int32_t CancelCapture();
    void Release() override;
    std::shared_ptr<PhotoCallback> GetApplicationCallback();

private:
    sptr<IStreamCapture> streamCapture_;
    std::shared_ptr<PhotoCallback> appCallback_;
    sptr<IStreamCaptureCallback> cameraSvcCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_PHOTO_OUTPUT_H
