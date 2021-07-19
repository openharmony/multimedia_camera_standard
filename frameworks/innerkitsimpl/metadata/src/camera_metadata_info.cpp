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
    valid_ = metadata_ != nullptr;
}

CameraMetadata::~CameraMetadata()
{
    if (metadata_) {
        free_camera_metadata_buffer(metadata_);
    }
}

bool CameraMetadata::addEntry(uint32_t item, const void *data, size_t data_count)
{
    if (!valid_) {
        return false;
    }

    if (!add_camera_metadata_item(metadata_, item, data, data_count)) {
        return true;
    }

    const char *name = get_camera_metadata_item_name(item);
    (void)name;

    if (name) {
        METADATA_ERR_LOG("Failed to add tag. tagname = %{public}s", name);
    } else {
         METADATA_ERR_LOG("Failed to add unknown tag");
    }

    valid_ = false;

    return false;
}

bool CameraMetadata::updateEntry(uint32_t tag, const void *data, size_t data_count)
{
    if (!valid_) {
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
    return valid_ ? metadata_ : nullptr;
}

const common_metadata_header_t *CameraMetadata::get() const
{
    return valid_ ? metadata_ : nullptr;
}

bool CameraMetadata::isValid() const
{
    return valid_;
}
}
}