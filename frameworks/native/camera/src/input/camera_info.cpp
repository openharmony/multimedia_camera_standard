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
#include "input/camera_info.h"
#include <securec.h>
#include "camera_metadata_info.h"
#include "camera_log.h"

using namespace std;

namespace OHOS {
namespace CameraStandard {
CameraInfo::CameraInfo(std::string cameraID, std::shared_ptr<Camera::CameraMetadata> metadata)
{
    cameraID_ = cameraID;
    metadata_ = metadata;
    init(metadata->get());
}

CameraInfo::~CameraInfo()
{
    metadata_.reset();
}

void CameraInfo::init(common_metadata_header_t *metadata)
{
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &item);
    if (ret == CAM_META_SUCCESS) {
        cameraPosition_ = static_cast<camera_position_enum_t>(item.data.u8[0]);
    }

    ret = Camera::FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        cameraType_ = static_cast<camera_type_enum_t>(item.data.u8[0]);
    }

    ret = Camera::FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        connectionType_ = static_cast<camera_connection_type_t>(item.data.u8[0]);
    }

    ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
    if (ret == CAM_META_SUCCESS) {
        isMirrorSupported_ = ((item.data.u8[0] == 1) || (item.data.u8[0] == 0));
    }
    MEDIA_INFO_LOG("camera position: %{public}d, camera type: %{public}d, camera connection type: %{public}d, "
                    "Mirror Supported: %{public}d ",
                   cameraPosition_, cameraType_, connectionType_, isMirrorSupported_);
}

std::string CameraInfo::GetID()
{
    return cameraID_;
}

std::shared_ptr<Camera::CameraMetadata> CameraInfo::GetMetadata()
{
    return metadata_;
}

void CameraInfo::SetMetadata(std::shared_ptr<Camera::CameraMetadata> metadata)
{
    metadata_ = metadata;
}

camera_position_enum_t CameraInfo::GetPosition()
{
    return cameraPosition_;
}

camera_type_enum_t CameraInfo::GetCameraType()
{
    return cameraType_;
}

camera_connection_type_t CameraInfo::GetConnectionType()
{
    return connectionType_;
}

bool CameraInfo::IsMirrorSupported()
{
    return isMirrorSupported_;
}

std::vector<float> CameraInfo::CalculateZoomRange()
{
    int32_t ret;
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    uint32_t zoomRangeCount = 2;
    constexpr float factor = 100.0;
    float minZoom;
    float maxZoom;
    float tempZoom;
    camera_metadata_item_t item;

    ret = Camera::FindCameraMetadataItem(metadata_->get(), OHOS_ABILITY_ZOOM_CAP, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get zoom cap with return code %{public}d", ret);
        return {};
    }
    if (item.count != zoomRangeCount) {
        MEDIA_ERR_LOG("Invalid zoom cap count: %{public}d", item.count);
        return {};
    }
    MEDIA_DEBUG_LOG("Zoom cap min: %{public}d, max: %{public}d",
                    item.data.i32[minIndex], item.data.i32[maxIndex]);
    minZoom = item.data.i32[minIndex] / factor;
    maxZoom = item.data.i32[maxIndex] / factor;

    ret = Camera::FindCameraMetadataItem(metadata_->get(), OHOS_ABILITY_SCENE_ZOOM_CAP, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get scene zoom cap with return code %{public}d", ret);
        return {};
    }
    if (item.count != zoomRangeCount) {
        MEDIA_ERR_LOG("Invalid zoom cap count: %{public}d", item.count);
        return {};
    }
    MEDIA_DEBUG_LOG("Scene zoom cap min: %{public}d, max: %{public}d",
                    item.data.i32[minIndex], item.data.i32[maxIndex]);
    tempZoom = item.data.i32[minIndex] / factor;
    if (minZoom < tempZoom) {
        minZoom = tempZoom;
    }
    tempZoom = item.data.i32[maxIndex] / factor;
    if (maxZoom > tempZoom) {
        maxZoom = tempZoom;
    }
    return {minZoom, maxZoom};
}

std::vector<float> CameraInfo::GetZoomRatioRange()
{
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    std::vector<float> range;

    if (!zoomRatioRange_.empty()) {
        return zoomRatioRange_;
    }

    int ret;
    uint32_t zoomRangeCount = 2;
    camera_metadata_item_t item;

    ret = Camera::FindCameraMetadataItem(metadata_->get(), OHOS_ABILITY_ZOOM_RATIO_RANGE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get zoom ratio range with return code %{public}d", ret);
        return {};
    }
    if (item.count != zoomRangeCount) {
        MEDIA_ERR_LOG("Invalid zoom ratio range count: %{public}d", item.count);
        return {};
    }
    range = {item.data.f[minIndex], item.data.f[maxIndex]};

    if (range[minIndex] > range[maxIndex]) {
        MEDIA_ERR_LOG("Invalid zoom range. min: %{public}f, max: %{public}f", range[minIndex], range[maxIndex]);
        return {};
    }
    MEDIA_DEBUG_LOG("Zoom range min: %{public}f, max: %{public}f", range[minIndex], range[maxIndex]);

    zoomRatioRange_ = range;
    return zoomRatioRange_;
}

std::vector<int32_t> CameraInfo::GetExposureBiasRange()
{
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    std::vector<int32_t> range;

    if (!exposureBiasRange_.empty()) {
        return exposureBiasRange_;
    }

    int ret;
    uint32_t biasRangeCount = 2;
    camera_metadata_item_t item;

    ret = Camera::FindCameraMetadataItem(metadata_->get(), OHOS_CONTROL_AE_COMPENSATION_RANGE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get exposure compensation range with return code %{public}d", ret);
        return {};
    }
    if (item.count != biasRangeCount) {
        MEDIA_ERR_LOG("Invalid exposure compensation range count: %{public}d", item.count);
        return {};
    }

    range = {item.data.i32[minIndex], item.data.i32[maxIndex]};
    if (range[minIndex] > range[maxIndex]) {
        MEDIA_ERR_LOG("Invalid exposure compensation range. min: %{public}d, max: %{public}d",
                      range[minIndex], range[maxIndex]);
        return {};
    }
    MEDIA_DEBUG_LOG("Exposure compensation min: %{public}d, max: %{public}d", range[minIndex], range[maxIndex]);

    exposureBiasRange_ = range;
    return exposureBiasRange_;
}
} // CameraStandard
} // OHOS
