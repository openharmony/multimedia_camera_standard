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

#ifndef OHOS_CAMERA_CAMERA_INPUT_H
#define OHOS_CAMERA_CAMERA_INPUT_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include "camera_info.h"
#include "capture_input.h"
#include "icamera_device_service.h"
#include "icamera_device_service_callback.h"

namespace OHOS {
namespace CameraStandard {
typedef struct {
    uint32_t height;
    uint32_t width;
} CameraPicSize;

class ErrorCallback {
public:
    ErrorCallback() = default;
    virtual ~ErrorCallback() = default;
    virtual void OnError(const int32_t errorType, const int32_t errorMsg) const;
};

class ExposureCallback {
public:
    enum ExposureState {
        SCAN = 0,
        CONVERGED,
    };
    ExposureCallback() = default;
    virtual ~ExposureCallback() = default;
    virtual void OnExposureState(ExposureState state) = 0;
};

class FocusCallback {
public:
    enum FocusState {
        SCAN = 0,
        FOCUSED,
        UNFOCUSED
    };
    FocusCallback() = default;
    virtual ~FocusCallback() = default;
    virtual void OnFocusState(FocusState state) = 0;
};

class CameraInput : public CaptureInput {
public:
    CameraInput(sptr<ICameraDeviceService> &deviceObj, sptr<CameraInfo> &camera);
    ~CameraInput() {};
    void LockForControl();
    int32_t UnlockForControl();
    std::vector<camera_format_t> GetSupportedPhotoFormats();
    std::vector<camera_format_t> GetSupportedVideoFormats();
    std::vector<camera_format_t> GetSupportedPreviewFormats();
    std::vector<CameraPicSize> getSupportedSizes(camera_format_t format);
    std::vector<camera_ae_mode_t> GetSupportedExposureModes();
    void SetExposureMode(camera_ae_mode_t exposureMode);
    camera_ae_mode_t GetExposureMode();
    void SetExposureCallback(std::shared_ptr<ExposureCallback> exposureCallback);
    std::vector<camera_af_mode_t> GetSupportedFocusModes();
    void SetFocusCallback(std::shared_ptr<FocusCallback> focusCallback);
    void SetFocusMode(camera_af_mode_t focusMode);
    camera_af_mode_t GetFocusMode();
    std::vector<float> GetSupportedZoomRatioRange();
    float GetZoomRatio();
    void SetZoomRatio(float zoomRatio);
    std::vector<camera_flash_mode_enum_t> GetSupportedFlashModes();
    camera_flash_mode_enum_t GetFlashMode();
    void SetFlashMode(camera_flash_mode_enum_t flashMode);
    void SetErrorCallback(std::shared_ptr<ErrorCallback> errorCallback);
    void Release() override;
    sptr<ICameraDeviceService> GetCameraDevice();
    std::shared_ptr<ErrorCallback> GetErrorCallback();
    void ProcessAutoFocusUpdates(const std::shared_ptr<CameraMetadata> &result);
    std::string GetCameraSettings();
    int32_t SetCameraSettings(std::string setting);

private:
    std::mutex changeMetaMutex_;
    std::shared_ptr<CameraMetadata> changedMetadata_;
    sptr<CameraInfo> cameraObj_;
    sptr<ICameraDeviceService> deviceObj_;
    std::shared_ptr<ErrorCallback> errorCallback_;
    sptr<ICameraDeviceServiceCallback> CameraDeviceSvcCallback_;
    std::shared_ptr<ExposureCallback> exposurecallback_;
    std::shared_ptr<FocusCallback> focusCallback_;
    static const std::unordered_map<camera_af_state_t, FocusCallback::FocusState> mapFromMetadataFocus_;

    template<typename DataPtr, typename Vec, typename VecType>
    static void getVector(DataPtr data, size_t count, Vec &vect, VecType dataType);
    int32_t SetCropRegion(float zoomRatio);
    int32_t StartFocus(camera_af_mode_t focusMode);
    int32_t UpdateSetting(std::shared_ptr<CameraMetadata> changedMetadata);
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_INPUT_H
