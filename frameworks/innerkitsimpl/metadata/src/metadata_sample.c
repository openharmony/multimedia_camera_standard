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

    uint32_t item_capacity = 1;
    uint32_t data_capacity = 1;
    common_metadata_header_t *metadata = allocate_camera_metadata_buffer(item_capacity, data_capacity);
    METADATA_INFO_LOG("Test Metadata allocate_camera_metadata_buffer metadata: %p", metadata);

    uint8_t cameraPositon = OHOS_CAMERA_POSITION_FRONT;
    int result = add_camera_metadata_item(metadata,
        OHOS_ABILITY_CAMERA_POSITION, &cameraPositon, 1);
    METADATA_INFO_LOG("Test Metadata add_camera_metadata_item ret: %d", result);

    camera_metadata_item_entry_t item;
    result = find_camera_item(metadata, OHOS_ABILITY_CAMERA_POSITION, &item);
    METADATA_INFO_LOG("Test Metadata find_camera_item ret: %d", result);

    camera_metadata_item_entry_t updated_item;
    cameraPositon = OHOS_CAMERA_POSITION_BACK;
    result = update_camera_metadata_item(metadata, 0, &cameraPositon, 1, &updated_item);
    METADATA_INFO_LOG("Test Metadata update_camera_metadata_item ret: %d", result);

    result = delete_camera_metadata_item(metadata, 0);
    METADATA_INFO_LOG("Test Metadata delete_camera_metadata_item ret: %d", result);

    free_camera_metadata_buffer(metadata);
    METADATA_INFO_LOG("Test Metadata End");
    return 0;
}
