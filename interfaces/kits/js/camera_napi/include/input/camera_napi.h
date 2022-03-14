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

#ifndef CAMERA_NAPI_H_
#define CAMERA_NAPI_H_

#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "hilog/log.h"
#include "camera_napi_utils.h"

#include "input/camera_manager.h"
#include "output/capture_output.h"
#include "session/capture_session.h"
#include "input/capture_input.h"

#include "input/camera_input_napi.h"
#include "input/camera_manager_napi.h"
#include "output/preview_output_napi.h"
#include "output/photo_output_napi.h"
#include "output/video_output_napi.h"
#include "session/camera_session_napi.h"

#include <cinttypes>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

namespace OHOS {
namespace CameraStandard {
struct CamRecorderCallback;

static const std::string CAMERA_LIB_NAPI_CLASS_NAME = "camera";
// Photo default size
static const std::int32_t PHOTO_DEFAULT_WIDTH = 1280;
static const std::int32_t PHOTO_DEFAULT_HEIGHT = 960;

// Surface default size
static const std::int32_t SURFACE_DEFAULT_WIDTH = 640;
static const std::int32_t SURFACE_DEFAULT_HEIGHT = 480;

// Preview default size
static const std::int32_t PREVIEW_DEFAULT_WIDTH = 640;
static const std::int32_t PREVIEW_DEFAULT_HEIGHT = 480;

// Video default size
static const std::int32_t VIDEO_DEFAULT_WIDTH = 640;
static const std::int32_t VIDEO_DEFAULT_HEIGHT = 360;

static const std::int32_t SURFACE_QUEUE_SIZE = 10;

static const std::int32_t CAM_FORMAT_JPEG = 2000;
static const std::int32_t CAM_FORMAT_YCRCb_420_SP = 1003;

static const std::vector<std::string> vecFlashMode {
    "FLASH_MODE_CLOSE", "FLASH_MODE_OPEN", "FLASH_MODE_AUTO", "FLASH_MODE_ALWAYS_OPEN"
};

static const std::vector<std::string> vecExposureMode {
    "EXPOSURE_MODE_MANUAL", "EXPOSURE_MODE_CONTINUOUS_AUTO"
};

static const std::vector<std::string> vecFocusMode {
    "FOCUS_MODE_MANUAL", "FOCUS_MODE_CONTINUOUS_AUTO", "FOCUS_MODE_AUTO", "FOCUS_MODE_LOCKED"
};

static const std::vector<std::string> vecCameraPositionMode {
    "CAMERA_POSITION_UNSPECIFIED", "CAMERA_POSITION_BACK", "CAMERA_POSITION_FRONT"
};

static const std::vector<std::string> vecCameraTypeMode {
    "CAMERA_TYPE_UNSPECIFIED", "CAMERA_TYPE_WIDE_ANGLE", "CAMERA_TYPE_ULTRA_WIDE",
    "CAMERA_TYPE_TELEPHOTO", "CAMERA_TYPE_TRUE_DEPTH"
};

static const std::vector<std::string> vecConnectionTypeMode {
    "CAMERA_CONNECTION_BUILT_IN", "CAMERA_CONNECTION_USB_PLUGIN", "CAMERA_CONNECTION_REMOTE"
};

static const std::vector<std::string> vecCameraFormat {
    "CAMERA_FORMAT_YCRCb_420_SP", "CAMERA_FORMAT_JPEG"
};

static const std::vector<std::string> vecCameraStatus {
    "CAMERA_STATUS_APPEAR", "CAMERA_STATUS_DISAPPEAR", "CAMERA_STATUS_AVAILABLE", "CAMERA_STATUS_UNAVAILABLE"
};

enum FlashMode {
    FLASHMODE_CLOSE = 1,
    FLASHMODE_OPEN = 2,
    FLASHMODE_AUTO = 3,
    FLASHMODE_ALWAYS_OPEN = 4,
};

enum ExposureMode {
    EXPOSUREMODE_MANUAL = 1,
    EXPOSUREMODE_CONTINUOUS_AUTO = 2,
};

enum FocusMode {
    FOCUSMODE_MANUAL = 1,
    FOCUSMODE_CONTINUOUS_AUTO_FOCUS = 2,
    FOCUSMODE_AUTO_FOCUS = 3,
    FOCUSMODE_LOCKED = 4,
};

class CameraNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    napi_ref GetErrorCallbackRef();

    CameraNapi();
    ~CameraNapi();

private:
    static void CameraNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    static napi_status AddNamedProperty(napi_env env, napi_value object,
                                        const std::string name, int32_t enumValue);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value CameraNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value CreateCameraManagerInstance(napi_env env, napi_callback_info info);
    static napi_value CreateCameraSessionInstance(napi_env env, napi_callback_info info);
    static napi_value CreatePreviewOutputInstance(napi_env env, napi_callback_info info);
    static napi_value CreatePhotoOutputInstance(napi_env env, napi_callback_info info);
    static napi_value CreateVideoOutputInstance(napi_env env, napi_callback_info info);
    static napi_value CreateFlashModeObject(napi_env env);
    static napi_value CreateExposureModeObject(napi_env env);
    static napi_value CreateFocusModeObject(napi_env env);
    static napi_value CreateCameraPositionEnum(napi_env env);
    static napi_value CreateCameraTypeEnum(napi_env env);
    static napi_value CreateConnectionTypeEnum(napi_env env);
    static napi_value CreateCameraStatusObject(napi_env env);
    static napi_value CreateCameraFormatObject(napi_env env);
    static napi_value CreateImageRotationEnum(napi_env env);
    static napi_value CreateErrorUnknownEnum(napi_env env);
    static napi_value CreateExposureStateEnum(napi_env env);
    static napi_value CreateFocusStateEnum(napi_env env);
    static napi_value CreateQualityLevelEnum(napi_env env);

    static napi_ref sConstructor_;
    static sptr<Surface> captureSurface_;

    static napi_ref flashModeRef_;
    static napi_ref exposureModeRef_;
    static napi_ref exposureStateRef_;
    static napi_ref focusModeRef_;
    static napi_ref focusStateRef_;
    static napi_ref cameraFormatRef_;
    static napi_ref cameraStatusRef_;
    static napi_ref connectionTypeRef_;
    static napi_ref cameraPositionRef_;
    static napi_ref cameraTypeRef_;
    static napi_ref imageRotationRef_;
    static napi_ref qualityLevelRef_;
    static napi_ref errorUnknownRef_;

    napi_env env_;
    napi_ref wrapper_;
    sptr<CameraManager> cameraManager_;
};

struct CameraNapiAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    CameraNapi *objectInfo;
    std::string photoSurfaceId;
    uint64_t surfaceId;
    sptr<CameraManager> cameraManager;
    sptr<CaptureSession> cameraSession;
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> photoOutput;
    sptr<CaptureOutput> videoOutput;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_NAPI_H_ */
