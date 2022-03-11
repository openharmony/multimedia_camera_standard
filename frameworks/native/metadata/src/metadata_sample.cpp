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

#include "camera_metadata_operator.h"
#include "metadata_log.h"

int main(void)
{
    METADATA_INFO_LOG("Test Metadata");

    uint32_t itemCapacity = 1;
    uint32_t dataCapacity = 1;
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(itemCapacity, dataCapacity);
    METADATA_INFO_LOG("Test Metadata AllocateCameraMetadataBuffer metadata");

    uint8_t cameraPositon = OHOS_CAMERA_POSITION_FRONT;
    int result = AddCameraMetadataItem(metadata,
        OHOS_ABILITY_CAMERA_POSITION, &cameraPositon, 1);
    METADATA_INFO_LOG("Test Metadata AddCameraMetadataItem ret: %d", result);

    camera_metadata_item_t item;
    result = FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &item);
    METADATA_INFO_LOG("Test Metadata find_camera_item ret: %d", result);

    camera_metadata_item_t updatedItem;
    cameraPositon = OHOS_CAMERA_POSITION_BACK;
    result = UpdateCameraMetadataItem(metadata, 0, &cameraPositon, 1, &updatedItem);
    METADATA_INFO_LOG("Test Metadata UpdateCameraMetadataItem ret: %d", result);

    result = DeleteCameraMetadataItem(metadata, 0);
    METADATA_INFO_LOG("Test Metadata DeleteCameraMetadataItem ret: %d", result);

    FreeCameraMetadataBuffer(metadata);
    METADATA_INFO_LOG("Test Metadata End");
    return 0;
}
