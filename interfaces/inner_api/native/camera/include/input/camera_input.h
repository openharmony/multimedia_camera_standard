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

#ifndef OHOS_CAMERA_CAMERA_INPUT_H
#define OHOS_CAMERA_CAMERA_INPUT_H

#include <iostream>
#include <unordered_map>
#include <set>
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

typedef struct {
    float x;
    float y;
}Point;

class ErrorCallback {
public:
    ErrorCallback() = default;
    virtual ~ErrorCallback() = default;
    virtual void OnError(const int32_t errorType, const int32_t errorMsg) const = 0;
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
    ~CameraInput() {}

    /**
    * @brief create new device control setting.
    */
    void LockForControl();

    /**
    * @brief submit device control setting.
    *
    * @return Returns CAMERA_OK is success.
    */
    int32_t UnlockForControl();

    /**
    * @brief Get the supported format for photo.
    *
    * @return Returns vector of camera_format_t supported format for photo.
    */
    std::vector<camera_format_t> GetSupportedPhotoFormats();

    /**
    * @brief Get the supported format for video.
    *
    * @return Returns vector of camera_format_t supported format for video.
    */
    std::vector<camera_format_t> GetSupportedVideoFormats();

    /**
    * @brief Get the supported format for preview.
    *
    * @return Returns vector of camera_format_t supported format for preview.
    */
    std::vector<camera_format_t> GetSupportedPreviewFormats();

    /**
    * @brief Get the supported sizes for given format.
    *
    * @param camera_format_t for which you want to get supported sizes.
    * @return Returns vector of CameraPicSize supported sizes.
    */
    std::vector<CameraPicSize> getSupportedSizes(camera_format_t format);

    /**
    * @brief Get the supported exposure modes.
    *
    * @return Returns vector of camera_exposure_mode_enum_t supported exposure modes.
    */
    std::vector<camera_exposure_mode_enum_t> GetSupportedExposureModes();

    /**
    * @brief Query whether given exposure mode supported.
    *
    * @param camera_exposure_mode_enum_t exposure mode to query.
    * @return True is supported false otherwise.
    */
    bool IsExposureModeSupported(camera_exposure_mode_enum_t exposureMode);

    /**
    * @brief Set exposure mode.
    *
    * @param camera_exposure_mode_enum_t exposure mode to be set.
    */
    void SetExposureMode(camera_exposure_mode_enum_t exposureMode);

    /**
    * @brief Get the current exposure mode.
    *
    * @return Returns current exposure mode.
    */
    camera_exposure_mode_enum_t GetExposureMode();

    /**
    * @brief Set the exposure area.
    *
    * @param Point which specifies the area to expose.
    */
    void SetExposurePoint(Point exposurePoint);

    /**
    * @brief Get the supported Focus modes.
    *
    * @return Returns vector of camera_focus_mode_enum_t supported exposure modes.
    */
    std::vector<camera_focus_mode_enum_t> GetSupportedFocusModes();

    /**
    * @brief Get centre point of exposure area.
    *
    * @return Returns current exposure point.
    */
    Point GetExposurePoint();

    /**
    * @brief Get exposure compensation range.
    *
    * @return Returns supported exposure compensation range.
    */
    std::vector<int32_t> GetExposureBiasRange();

    /**
    * @brief Set exposure compensation value.
    *
    * @param exposure compensation value to be set.
    */
    void SetExposureBias(int32_t exposureBias);

    /**
    * @brief Get exposure compensation value.
    *
    * @return Returns current exposure compensation value.
    */
    int32_t GetExposureValue();

    /**
    * @brief Set the exposure callback.
    * which will be called when there is exposure state change.
    *
    * @param The ExposureCallback pointer.
    */
    void SetExposureCallback(std::shared_ptr<ExposureCallback> exposureCallback);

    /**
    * @brief Set the focus callback.
    * which will be called when there is focus state change.
    *
    * @param The ExposureCallback pointer.
    */
    void SetFocusCallback(std::shared_ptr<FocusCallback> focusCallback);

    /**
    * @brief Query whether given focus mode supported.
    *
    * @param camera_focus_mode_enum_t focus mode to query.
    * @return True is supported false otherwise.
    */
    bool IsFocusModeSupported(camera_focus_mode_enum_t focusMode);

    /**
    * @brief Set Focus mode.
    *
    * @param camera_focus_mode_enum_t focus mode to be set.
    */
    void SetFocusMode(camera_focus_mode_enum_t focusMode);

    /**
    * @brief Get the current focus mode.
    *
    * @return Returns current focus mode.
    */
    camera_focus_mode_enum_t GetFocusMode();

    /**
    * @brief Set the Focus area.
    *
    * @param Point which specifies the area to focus.
    */
    void SetFocusPoint(Point focusPoint);

    /**
    * @brief Get centre point of focus area.
    *
    * @return Returns current focus point.
    */
    Point GetFocusPoint();

    /**
    * @brief Get focal length.
    *
    * @return Returns focal length value.
    */
    float GetFocalLength();

    /**
    * @brief Get the supported Zoom Ratio range.
    *
    * @return Returns vector<float> of supported Zoom ratio range.
    */
    std::vector<float> GetSupportedZoomRatioRange();

    /**
    * @brief Get the current Zoom Ratio.
    *
    * @return Returns current Zoom Ratio.
    */
    float GetZoomRatio();

    /**
    * @brief Set Zoom ratio.
    *
    * @param Zoom ratio to be set.
    */
    void SetZoomRatio(float zoomRatio);

    /**
    * @brief Get the supported Focus modes.
    *
    * @return Returns vector of camera_focus_mode_enum_t supported exposure modes.
    */
    std::vector<camera_flash_mode_enum_t> GetSupportedFlashModes();

    /**
    * @brief Get the current flash mode.
    *
    * @return Returns current flash mode.
    */
    camera_flash_mode_enum_t GetFlashMode();

    /**
    * @brief Set flash mode.
    *
    * @param camera_flash_mode_enum_t flash mode to be set.
    */
    void SetFlashMode(camera_flash_mode_enum_t flashMode);

    /**
    * @brief Set the error callback.
    * which will be called when error occurs.
    *
    * @param The ErrorCallback pointer.
    */
    void SetErrorCallback(std::shared_ptr<ErrorCallback> errorCallback);

    /**
    * @brief Release camera input.
    */
    void Release() override;

    /**
    * @brief Get Camera Device.
    *
    * @return Returns Camera Device pointer.
    */
    sptr<ICameraDeviceService> GetCameraDevice();

    /**
    * @brief Get ErrorCallback pointer.
    *
    * @return Returns ErrorCallback pointer.
    */
    std::shared_ptr<ErrorCallback> GetErrorCallback();

    /**
    * @brief This function is called when there is focus state change
    * and process the focus state callback.
    *
    * @param result metadata got from callback from service layer.
    */
    void ProcessAutoFocusUpdates(const std::shared_ptr<OHOS::Camera::CameraMetadata> &result);

    /**
    * @brief This function is called when there is exposure state change
    * and process the exposure state callback.
    *
    * @param result metadata got from callback from service layer.
    */
    void ProcessAutoExposureUpdates(const std::shared_ptr<OHOS::Camera::CameraMetadata> &result);

    /**
    * @brief Get current Camera Settings.
    *
    * @return Returns string encoded metadata setting.
    */
    std::string GetCameraSettings();

    /**
    * @brief set the camera metadata setting.
    *
    * @param string encoded camera metadata setting.
    * @return Returns 0 if success or appropriate error code if failed.
    */
    int32_t SetCameraSettings(std::string setting);

    /**
    * @brief get the camera info associated with the device.
    *
    * @return Returns camera info.
    */
    sptr<CameraInfo> GetCameraDeviceInfo() override;

    friend void SetVideoStabilizingMode(sptr<CameraInput> device, CameraVideoStabilizationMode VideoStabilizationMode);
    friend void SetRecordingFrameRateRange(sptr<CameraInput> device, int32_t minFpsVal, int32_t maxFpsVal);
    friend void SetCaptureMetadataObjectTypes(sptr<CameraInput> device,
                                              std::set<camera_face_detect_mode_t> metadataObjectTypes);

private:
    std::mutex mutex_;
    std::shared_ptr<OHOS::Camera::CameraMetadata> changedMetadata_;
    sptr<CameraInfo> cameraObj_;
    sptr<ICameraDeviceService> deviceObj_;
    std::shared_ptr<ErrorCallback> errorCallback_;
    sptr<ICameraDeviceServiceCallback> CameraDeviceSvcCallback_;
    std::shared_ptr<ExposureCallback> exposureCallback_;
    std::shared_ptr<FocusCallback> focusCallback_;
    static const std::unordered_map<camera_focus_state_t, FocusCallback::FocusState> mapFromMetadataFocus_;
    static const std::unordered_map<camera_exposure_state_t, ExposureCallback::ExposureState> mapFromMetadataExposure_;

    template<typename DataPtr, typename Vec, typename VecType>
    static void getVector(DataPtr data, size_t count, Vec &vect, VecType dataType);
    int32_t SetCropRegion(float zoomRatio);
    int32_t StartFocus(camera_focus_mode_enum_t focusMode);
    int32_t UpdateSetting(std::shared_ptr<OHOS::Camera::CameraMetadata> changedMetadata);
};
    void SetVideoStabilizingMode(sptr<CameraInput> device, CameraVideoStabilizationMode VideoStabilizationMode);
    void SetRecordingFrameRateRange(sptr<CameraInput> device, int32_t minFpsVal, int32_t maxFpsVal);
    void SetCaptureMetadataObjectTypes(sptr<CameraInput> device,
                                       std::set<camera_face_detect_mode_t> metadataObjectTypes);
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_INPUT_H

