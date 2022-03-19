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

#include "camera_util.h"
#include "media_log.h"

namespace OHOS {
namespace CameraStandard {
std::unordered_map<int32_t, int32_t> g_cameraToPixelFormat = {
    {OHOS_CAMERA_FORMAT_RGBA_8888, PIXEL_FMT_RGBA_8888},
    {OHOS_CAMERA_FORMAT_YCBCR_420_888, PIXEL_FMT_YCBCR_420_SP},
    {OHOS_CAMERA_FORMAT_YCRCB_420_SP, PIXEL_FMT_YCRCB_420_SP},
    {OHOS_CAMERA_FORMAT_JPEG, PIXEL_FMT_YCRCB_420_SP},
};

std::map<int, std::string> g_cameraPos = {
    {0, "Front"},
    {1, "Back"},
    {2, "Other"},
};

std::map<int, std::string> g_cameraType = {
    {0, "Wide-Angle"},
    {1, "Ultra-Wide"},
    {2, "TelePhoto"},
    {3, "TrueDepth"},
    {4, "Logical"},
    {5, "Unspecified"},
};

std::map<int, std::string> g_cameraConType = {
    {0, "Builtin"},
    {1, "USB-Plugin"},
    {2, "Remote"},
};

std::map<int, std::string> g_cameraFormat = {
    {1, "RGBA_8888"},
    {2, "YCBCR_420_888"},
    {3, "YCRCB_420_SP"},
    {4, "JPEG"},
};

std::map<int, std::string> g_cameraFocusMode = {
    {0, "Manual"},
    {1, "Continuous-Auto"},
    {2, "Auto"},
    {3, "Locked"},
};

std::map<int, std::string> g_cameraFlashMode = {
    {0, "Close"},
    {1, "Open"},
    {2, "Auto"},
    {3, "Always-Open"},
};

int32_t HdiToServiceError(Camera::CamRetCode ret)
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

bool IsValidSize(std::shared_ptr<CameraMetadata> cameraAbility, int32_t format, int32_t width, int32_t height)
{
#ifndef BALTIMORE_CAMERA
    return true;
#endif
    uint32_t unitLen = 3;
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(cameraAbility->get(), OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to find stream configuration in camera ability with return code %{public}d", ret);
        return false;
    }
    if (item.count % unitLen != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return false;
    }
    for (uint32_t index = 0; index < item.count; index += 3) {
        if (item.data.i32[index] == format) {
            if (item.data.i32[index + 1] == width && item.data.i32[index + 2] == height) {
                MEDIA_INFO_LOG("Format:%{public}d, width:%{public}d, height:%{public}d found in supported streams",
                               format, width, height);
                return true;
            }
        }
    }
    MEDIA_ERR_LOG("Format:%{public}d, width:%{public}d, height:%{public}d not found in supported streams",
                  format, width, height);
    return false;
}
} // namespace CameraStandard
} // namespace OHOS
