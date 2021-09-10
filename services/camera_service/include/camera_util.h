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

#ifndef OHOS_CAMERA_UTIL_H
#define OHOS_CAMERA_UTIL_H

#include "types.h"

namespace OHOS {
namespace CameraStandard {
static const std::int32_t CAMERA_PREVIEW_COLOR_SPACE = 8;
static const std::int32_t CAMERA_PREVIEW_CAPTURE_ID = 101;
static const std::int32_t CAMERA_PREVIEW_STREAM_ID = 1001;

static const std::int32_t CAMERA_PHOTO_COLOR_SPACE = 8;
static const std::int32_t CAMERA_PHOTO_CAPTURE_ID = 102;
static const std::int32_t CAMERA_PHOTO_STREAM_ID = 1002;

static const std::int32_t CAMERA_VIDEO_COLOR_SPACE = 8;
static const std::int32_t CAMERA_VIDEO_CAPTURE_ID = 103;
static const std::int32_t CAMERA_VIDEO_STREAM_ID = 1003;

enum CamServiceError {
    CAMERA_OK = 0,
    CAMERA_ALLOC_ERROR,
    CAMERA_INVALID_ARG,
    CAMERA_DEVICE_BUSY,
    CAMERA_DEVICE_CLOSED,
    CAMERA_DEVICE_REQUEST_TIMEOUT,
    CAMERA_STREAM_BUFFER_LOST,
    CAMERA_INVALID_OUTPUT_CFG,
    CAMERA_UNKNOWN_ERROR
};

int HdiToServiceError(Camera::CamRetCode ret);

bool IsValidSize(int32_t width, int32_t height, std::vector<std::pair<int32_t, int32_t>> validSizes);
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_UTIL_H
