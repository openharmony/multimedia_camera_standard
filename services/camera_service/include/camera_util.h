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
enum {
    CAMERA_PREVIEW_WIDTH = 640,
    CAMERA_PREVIEW_HEIGHT = 480,
    CAMERA_PREVIEW_COLOR_SPACE = 8,
    CAMERA_PREVIEW_CAPTURE_ID = 101,
    CAMERA_PREVIEW_STREAM_ID = 1001,

    CAMERA_PHOTO_WIDTH = 1280,
    CAMERA_PHOTO_HEIGHT = 960,
    CAMERA_PHOTO_COLOR_SPACE = 8,
    CAMERA_PHOTO_CAPTURE_ID = 102,
    CAMERA_PHOTO_STREAM_ID = 1002,

    CAMERA_VIDEO_WIDTH = 1280,
    CAMERA_VIDEO_HEIGHT = 720,
    CAMERA_VIDEO_COLOR_SPACE = 8,
    CAMERA_VIDEO_CAPTURE_ID = 103,
    CAMERA_VIDEO_STREAM_ID = 1003
};

enum CamServiceError {
    CAMERA_OK,
    CAMERA_ALLOC_ERROR,
    CAMERA_INVALID_ARG,
    CAMERA_DEVICE_BUSY,
    CAMERA_DEVICE_CLOSED,
    CAMERA_DEVICE_REQUEST_TIMEOUT,
    CAMERA_STREAM_BUFFER_LOST,
    CAMERA_UNKNOWN_ERROR
};

int HdiToServiceError(Camera::CamRetCode ret);
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_UTIL_H
