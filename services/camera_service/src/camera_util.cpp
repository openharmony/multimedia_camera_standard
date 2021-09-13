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

#include "camera_util.h"
#include "media_log.h"

namespace OHOS {
namespace CameraStandard {
int HdiToServiceError(Camera::CamRetCode ret)
{
    enum CamServiceError err = CAMERA_UNKNOWN_ERROR;

    switch (ret) {
        case Camera::NO_ERROR:
            err = CAMERA_OK;
            break;
        case Camera::CAMERA_BUSY:
            err = CAMERA_DEVICE_BUSY;
            break;
        case Camera::INVALID_ARGUMENT:
            err = CAMERA_INVALID_ARG;
            break;
        case Camera::CAMERA_CLOSED:
            err = CAMERA_DEVICE_CLOSED;
            break;
        default:
            MEDIA_ERR_LOG("HdiToServiceError() error code from hdi: %{public}d", ret);
            break;
    }
    return err;
}

CaptureType GetCaptureType(int32_t captureId)
{
    CaptureType capType;

    if (captureId >= PREVIEW_CAPTURE_ID_START && captureId <= PREVIEW_CAPTURE_ID_END) {
        capType = CAPTURE_TYPE_PREVIEW;
    } else if (captureId >= PHOTO_CAPTURE_ID_START && captureId <= PHOTO_CAPTURE_ID_END) {
        capType = CAPTURE_TYPE_PHOTO;
    } else {
        capType = CAPTURE_TYPE_VIDEO;
    }
    return capType;
}

bool IsValidSize(int32_t width, int32_t height, std::vector<std::pair<int32_t, int32_t>> validSizes)
{
    auto curPair = std::make_pair(width, height);
    if (std::find(validSizes.begin(), validSizes.end(), curPair) != validSizes.end()) {
        return true;
    } else {
        MEDIA_ERR_LOG("Width %{public}d and height %{public}d not found in suported sizes", width, height);
    }
    return false;
}
} // namespace CameraStandard
} // namespace OHOS
