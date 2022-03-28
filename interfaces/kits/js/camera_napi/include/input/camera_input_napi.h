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

#ifndef CAMERA_INPUT_NAPI_H_
#define CAMERA_INPUT_NAPI_H_

#include <securec.h>

#include "display_type.h"
#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "hilog/log.h"
#include "camera_napi_utils.h"

#include "input/camera_manager.h"
#include "input/camera_input.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "input/camera_size_napi.h"

namespace OHOS {
namespace CameraStandard {
static const std::string CAMERA_INPUT_NAPI_CLASS_NAME = "CameraInput";

class ExposureCallbackListener : public ExposureCallback {
public:
    ExposureCallbackListener(napi_env env, napi_ref ref) : env_(env), callbackRef_(ref) {}
    ~ExposureCallbackListener() = default;
    void OnExposureState(const ExposureState state) override;

private:
    void OnExposureStateCallback(ExposureState state) const;
    void OnExposureStateCallbackAsync(ExposureState state) const;

    napi_env env_;
    napi_ref callbackRef_ = nullptr;
};

struct ExposureCallbackInfo {
    ExposureCallback::ExposureState state_;
    const ExposureCallbackListener *listener_;
    ExposureCallbackInfo(ExposureCallback::ExposureState state, const ExposureCallbackListener *listener)
        : state_(state), listener_(listener) {}
};

class FocusCallbackListener : public FocusCallback {
public:
    FocusCallbackListener(napi_env env, napi_ref ref) : env_(env), callbackRef_(ref) {}
    ~FocusCallbackListener() = default;
    void OnFocusState(FocusState state) override;

private:
    void OnFocusStateCallback(FocusState state) const;
    void OnFocusStateCallbackAsync(FocusState state) const;

    napi_env env_;
    napi_ref callbackRef_ = nullptr;
};

struct FocusCallbackInfo {
    FocusCallback::FocusState state_;
    const FocusCallbackListener *listener_;
    FocusCallbackInfo(FocusCallback::FocusState state, const FocusCallbackListener *listener)
        : state_(state), listener_(listener) {}
};

class ErrorCallbackListener : public ErrorCallback {
public:
    ErrorCallbackListener(napi_env env, napi_ref ref) : env_(env), callbackRef_(ref) {}
    ~ErrorCallbackListener() = default;
    void OnError(const int32_t errorType, const int32_t errorMsg) const override;

private:
    void OnErrorCallback(const int32_t errorType, const int32_t errorMsg) const;
    void OnErrorCallbackAsync(const int32_t errorType, const int32_t errorMsg) const;

    napi_env env_;
    napi_ref callbackRef_ = nullptr;
};

struct ErrorCallbackInfo {
    int32_t errorType_;;
    int32_t errorMsg_;
    const ErrorCallbackListener *listener_;
    ErrorCallbackInfo(int32_t errorType, int32_t errorMsg, const ErrorCallbackListener *listener)
        : errorType_(errorType), errorMsg_(errorMsg), listener_(listener) {}
};

class CameraInputNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateCameraInput(napi_env env, std::string cameraId,
                                                sptr<CameraInput> cameraInput);
    CameraInputNapi();
    ~CameraInputNapi();
    sptr<CameraInput> GetCameraInput();

private:
    static void CameraInputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value CameraInputNapiConstructor(napi_env env, napi_callback_info info);
    static napi_value GetCameraId(napi_env env, napi_callback_info info);
    static napi_value HasFlash(napi_env env, napi_callback_info info);
    static napi_value IsFlashModeSupported(napi_env env, napi_callback_info info);
    static napi_value GetFlashMode(napi_env env, napi_callback_info info);
    static napi_value SetFlashMode(napi_env env, napi_callback_info info);
    static napi_value IsExposureModeSupported(napi_env env, napi_callback_info info);
    static napi_value GetExposureMode(napi_env env, napi_callback_info info);
    static napi_value SetExposureMode(napi_env env, napi_callback_info info);
    static napi_value IsFocusModeSupported(napi_env env, napi_callback_info info);
    static napi_value GetFocusMode(napi_env env, napi_callback_info info);
    static napi_value SetFocusMode(napi_env env, napi_callback_info info);
    static napi_value GetSupportedSizes(napi_env env, napi_callback_info info);
    static napi_value GetSupportedPhotoFormats(napi_env env, napi_callback_info info);
    static napi_value GetSupportedVideoFormats(napi_env env, napi_callback_info info);
    static napi_value GetSupportedPreviewFormats(napi_env env, napi_callback_info info);
    static napi_value GetZoomRatioRange(napi_env env, napi_callback_info info);
    static napi_value GetZoomRatio(napi_env env, napi_callback_info info);
    static napi_value SetZoomRatio(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);

    static bool IsFlashSupported(sptr<CameraInput> cameraInput, int flash);

    napi_env env_;
    napi_ref wrapper_;
    std::string cameraId_;
    sptr<CameraInput> cameraInput_;
    std::shared_ptr<ExposureCallbackListener> exposureCallback_ = nullptr;
    std::shared_ptr<FocusCallbackListener> focusCallback_ = nullptr;
    std::shared_ptr<ErrorCallbackListener> errorCallback_ = nullptr;

    void RegisterCallback(napi_env env, const std::string &eventType, napi_ref callbackRef);

    static napi_ref sConstructor_;
    static std::string sCameraId_;
    static sptr<CameraInput> sCameraInput_;
};

struct CameraInputAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    CameraInputNapi* objectInfo;
    bool status;
    std::string cameraId;
    std::string enumType;
    camera_format_t cameraFormat;
    int32_t flashMode;
    int32_t exposureMode;
    camera_af_mode_t focusMode;
    bool focusModeLocked;
    float zoomRatio;
    bool hasFlash;
    std::vector<camera_ae_mode_t> vecSupportedExposureModeList;
    std::vector<camera_flash_mode_enum_t> vecSupportedFlashModeList;
    std::vector<camera_format_t> vecSupportedPhotoFormatList;
    std::vector<camera_format_t> vecSupportedVideoFormatList;
    std::vector<camera_format_t> vecSupportedPreviewFormatList;
    std::vector<CameraPicSize> vecSupportedSizeList;
    std::vector<float> vecZoomRatioList;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_INPUT_NAPI_H_ */
