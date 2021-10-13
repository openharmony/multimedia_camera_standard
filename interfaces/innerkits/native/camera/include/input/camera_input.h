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
    uint32_t width;
    uint32_t height;
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
    enum PhotoFormat {
        JPEG_FORMAT = 0
    };
    enum VideoFormat {
        YUV_FORMAT = 0,
        H264_FORMAT,
        H265_FORMAT
    };

    CameraInput(sptr<ICameraDeviceService> &deviceObj, sptr<CameraInfo> &camera);
    ~CameraInput() {};
    void LockForControl();
    int32_t UnlockForControl();
    std::vector<PhotoFormat> GetSupportedPhotoFormats();
    std::vector<VideoFormat> GetSupportedVideoFormats();
    bool IsPhotoFormatSupported(PhotoFormat photoFormat);
    bool IsVideoFormatSupported(VideoFormat videoFormat);
    std::vector<CameraPicSize *> GetSupportedSizesForPhoto(PhotoFormat format);
    std::vector<CameraPicSize *> GetSupportedSizesForVideo(VideoFormat format);
    std::vector<camera_exposure_mode_enum_t> GetSupportedExposureModes();
    void SetExposureMode(camera_exposure_mode_enum_t exposureMode);
    camera_exposure_mode_enum_t GetExposureMode();
    void SetExposureCallback(std::shared_ptr<ExposureCallback> exposureCallback);
    std::vector<camera_focus_mode_enum_t> GetSupportedFocusModes();
    void SetFocusCallback(std::shared_ptr<FocusCallback> focusCallback);
    void SetFocusMode(camera_focus_mode_enum_t focusMode);
    camera_focus_mode_enum_t GetFocusMode();
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

private:
    std::shared_ptr<CameraMetadata> changedMetadata_;
    sptr<CameraInfo> cameraObj_;
    sptr<ICameraDeviceService> deviceObj_;
    std::shared_ptr<ErrorCallback> errorCallback_;
    sptr<ICameraDeviceServiceCallback> CameraDeviceSvcCallback_;
    std::shared_ptr<ExposureCallback> exposurecallback_;
    std::shared_ptr<FocusCallback> focusCallback_;

    template<typename DataPtr, typename Vec, typename VecType>
    static void getVector(DataPtr data, size_t count, Vec &vect, VecType dataType);
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_INPUT_H
