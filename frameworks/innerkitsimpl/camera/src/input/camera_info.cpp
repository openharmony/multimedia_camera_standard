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

#include "camera_metadata_info.h"
#include "input/camera_info.h"
#include "media_log.h"

using namespace std;

namespace OHOS {
namespace CameraStandard {
CameraInfo::CameraInfo(std::string cameraID, std::shared_ptr<CameraMetadata> metadata)
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

    int ret = FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &item);
    if (ret == CAM_META_SUCCESS) {
        cameraPosition_ = static_cast<camera_position_enum_t>(item.data.u8[0]);
    }

    ret = FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        cameraType_ = static_cast<camera_type_enum_t>(item.data.u8[0]);
    }

    ret = FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        connectionType_ = static_cast<camera_connection_type_t>(item.data.u8[0]);
    }

    ret = FindCameraMetadataItem(metadata_->get(), OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
    if (ret == CAM_META_SUCCESS) {
        isMirrorSupported_ = (item.data.u8[0] > 0) ? true : false;
    }
    MEDIA_INFO_LOG("camera position: %{public}d, camera type: %{public}d, camera connection type: %{public}d",
                   cameraPosition_, cameraType_, connectionType_);
}

std::string CameraInfo::GetID()
{
    return cameraID_;
}

std::shared_ptr<CameraMetadata> CameraInfo::GetMetadata()
{
    return metadata_;
}

void CameraInfo::SetMetadata(std::shared_ptr<CameraMetadata> metadata)
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
} // CameraStandard
} // OHOS
