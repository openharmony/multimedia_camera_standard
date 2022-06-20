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

#ifndef OHOS_CAMERA_UTIL_H
#define OHOS_CAMERA_UTIL_H

#include "display_type.h"
#include <limits.h>
#include "types.h"

namespace OHOS {
namespace CameraStandard {
static constexpr int32_t CAMERA_COLOR_SPACE = 8;

enum CamServiceError {
    CAMERA_OK = 0,
    CAMERA_ALLOC_ERROR,
    CAMERA_INVALID_ARG,
    CAMERA_UNSUPPORTED,
    CAMERA_DEVICE_BUSY,
    CAMERA_DEVICE_CLOSED,
    CAMERA_DEVICE_REQUEST_TIMEOUT,
    CAMERA_STREAM_BUFFER_LOST,
    CAMERA_INVALID_SESSION_CFG,
    CAMERA_CAPTURE_LIMIT_EXCEED,
    CAMERA_INVALID_STATE,
    CAMERA_UNKNOWN_ERROR
};

extern std::unordered_map<int32_t, int32_t> g_cameraToPixelFormat;
extern std::map<int, std::string> g_cameraPos;
extern std::map<int, std::string> g_cameraType;
extern std::map<int, std::string> g_cameraConType;
extern std::map<int, std::string> g_cameraFormat;
extern std::map<int, std::string> g_cameraFocusMode;
extern std::map<int, std::string> g_cameraExposureMode;
extern std::map<int, std::string> g_cameraFlashMode;

int32_t HdiToServiceError(Camera::CamRetCode ret);

std::string CreateMsg(const char *format, ...);

int32_t AllocateCaptureId(int32_t &captureId);

void ReleaseCaptureId(int32_t captureId);

bool IsValidSize(std::shared_ptr<Camera::CameraMetadata> cameraAbility, int32_t format, int32_t width, int32_t height);
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_UTIL_H
