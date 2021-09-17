/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "metadata_log.h"

namespace OHOS {
namespace CameraStandard {
CameraMetadata::CameraMetadata(size_t itemCapacity, size_t dataCapacity)
{
    metadata_ = allocate_camera_metadata_buffer(itemCapacity, ALIGN_TO(dataCapacity, DATA_ALIGNMENT));
}

CameraMetadata::~CameraMetadata()
{
    if (metadata_) {
        free_camera_metadata_buffer(metadata_);
        metadata_ = nullptr;
    }
}

bool CameraMetadata::addEntry(uint32_t item, const void *data, size_t data_count)
{
    if (metadata_ == nullptr) {
        METADATA_ERR_LOG("metadata_ is null");
        return false;
    }

    auto result = add_camera_metadata_item(metadata_, item, data, data_count);
    if (!result) {
        return true;
    }

    if (result != CAM_META_ITEM_CAP_EXCEED && result != CAM_META_DATA_CAP_EXCEED) {
        const char *name = get_camera_metadata_item_name(item);
        (void)name;

        if (name) {
            METADATA_ERR_LOG("Failed to add tag. tagname = %{public}s", name);
        } else {
            METADATA_ERR_LOG("Failed to add unknown tag");
        }
        return false;
    }

    return resize_add_metadata(item, data, data_count);
}

bool CameraMetadata::resize_add_metadata(uint32_t item, const void *data, size_t data_count)
{
    uint32_t data_type;

    auto itemCapacity = get_camera_metadata_item_capacity(metadata_);
    auto data_capacity = get_camera_metadata_data_size(metadata_);

    int32_t ret = get_camera_metadata_item_type(item, &data_type);
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("get_camera_metadata_item_type invalid item type");
        return false;
    }
    size_t size = calculate_camera_metadata_item_data_size(data_type, data_count);

    common_metadata_header_t *newMetadata = allocate_camera_metadata_buffer((itemCapacity + 1) * 2,
        ALIGN_TO((data_capacity + size) * 2, DATA_ALIGNMENT));

    if (newMetadata == nullptr) {
        METADATA_ERR_LOG("Failed to resize the metadata buffer");
        return false;
    }

    auto result = copy_camera_metadata_items(newMetadata, metadata_);
    if (result != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("Failed to copy the old metadata to new metadata");
        free_camera_metadata_buffer(newMetadata);
        return false;
    }

    result = add_camera_metadata_item(newMetadata, item, data, data_count);
    if (result != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("Failed to add new entry");
        free_camera_metadata_buffer(newMetadata);
        return false;
    }
    replace_metadata(newMetadata);

    return true;
}

void CameraMetadata::replace_metadata(common_metadata_header_t *newMetadata)
{
    if (metadata_ == newMetadata) {
        return;
    }

    free_camera_metadata_buffer(metadata_);
    metadata_ = newMetadata;
}

bool CameraMetadata::updateEntry(uint32_t tag, const void *data, size_t data_count)
{
    if (metadata_ == nullptr) {
        METADATA_ERR_LOG("metadata_ is null");
        return false;
    }

    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata_, tag, &item);
    if (ret) {
        const char *name = get_camera_metadata_item_name(tag);
        (void)name;
        METADATA_ERR_LOG("Failed to update tag tagname = %{public}s : not present", (name ? name : "<unknown>"));
        return false;
    }

    ret = update_camera_metadata_item_by_index(metadata_, item.index, data,
                                               data_count, nullptr);
    if (ret) {
        const char *name = get_camera_metadata_item_name(tag);
        (void)name;
        METADATA_ERR_LOG("Failed to update tag tagname = %{public}s", (name ? name : "<unknown>"));
        return false;
    }

    return true;
}

common_metadata_header_t *CameraMetadata::get()
{
    return metadata_;
}

const common_metadata_header_t *CameraMetadata::get() const
{
    return metadata_;
}

bool CameraMetadata::isValid() const
{
    return metadata_ != nullptr;
}
}
}
