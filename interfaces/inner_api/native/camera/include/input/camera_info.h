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

#ifndef OHOS_CAMERA_CAMERA_INFO_H
#define OHOS_CAMERA_CAMERA_INFO_H

#include <iostream>
#include <vector>
#include <refbase.h>
#include "camera_metadata_info.h"

namespace OHOS {
namespace CameraStandard {
class CameraInfo : public RefBase {
public:
    CameraInfo() = default;
    CameraInfo(std::string cameraID, std::shared_ptr<CameraMetadata> metadata);
    ~CameraInfo();
    std::string GetID();
    std::shared_ptr<CameraMetadata> GetMetadata();
    void SetMetadata(std::shared_ptr<CameraMetadata> metadata);
    camera_position_enum_t GetPosition();
    camera_type_enum_t GetCameraType();
    camera_connection_type_t GetConnectionType();
    bool IsMirrorSupported();
    std::vector<float> GetZoomRatioRange();

private:
    std::string cameraID_;
    std::shared_ptr<CameraMetadata> metadata_;
    camera_position_enum_t cameraPosition_ = OHOS_CAMERA_POSITION_OTHER;
    camera_type_enum_t cameraType_ = OHOS_CAMERA_TYPE_UNSPECIFIED;
    camera_connection_type_t connectionType_ = OHOS_CAMERA_CONNECTION_TYPE_BUILTIN;
    bool isMirrorSupported_ = false;
    std::vector<float> zoomRatioRange_;

    void init(common_metadata_header_t *metadataHeader);
    std::vector<float> CalculateZoomRange();
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_INFO_H
