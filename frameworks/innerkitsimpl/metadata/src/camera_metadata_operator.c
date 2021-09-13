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


#include "camera_metadata_item_info.c"
#include "metadata_log.h"
#include "securec.h"

#include <string.h>

#define METADATA_HEADER_DATA_SIZE 4

uint8_t *get_metadata_data(const common_metadata_header_t *metadata_header)
{
    return (uint8_t *)metadata_header + metadata_header->data_start;
}

camera_metadata_item_entry_t *get_metadata_items(const common_metadata_header_t *metadata_header)
{
    return (camera_metadata_item_entry_t *)
        ((uint8_t *)metadata_header + metadata_header->items_start);
}

common_metadata_header_t *fill_camera_metadata(void *buffer, size_t memory_required,
                                               uint32_t item_capacity, uint32_t data_capacity)
{
    METADATA_DEBUG_LOG("fill_camera_metadata start");
    if (buffer == NULL) {
        METADATA_ERR_LOG("fill_camera_metadata buffer is null");
        return NULL;
    }

    common_metadata_header_t *metadata_header = (common_metadata_header_t *)buffer;
    metadata_header->version = CURRENT_CAMERA_METADATA_VERSION;
    metadata_header->size = memory_required;
    metadata_header->item_count = 0;
    metadata_header->item_capacity = item_capacity;
    metadata_header->items_start = ALIGN_TO(sizeof(common_metadata_header_t), ITEM_ALIGNMENT);
    metadata_header->data_count = 0;
    metadata_header->data_capacity = data_capacity;
    size_t data_unaligned = (uint8_t *)(get_metadata_items(metadata_header) +
                                metadata_header->item_capacity) - (uint8_t *)metadata_header;
    metadata_header->data_start = ALIGN_TO(data_unaligned, DATA_ALIGNMENT);

    METADATA_DEBUG_LOG("fill_camera_metadata end");
    return metadata_header;
}

size_t calculate_camera_metadata_memory_required(uint32_t item_count, uint32_t data_count)
{
    METADATA_DEBUG_LOG("calculate_camera_metadata_memory_required start");
    size_t memory_required = sizeof(common_metadata_header_t);
    memory_required = ALIGN_TO(memory_required, ITEM_ALIGNMENT);

    memory_required += sizeof(camera_metadata_item_entry_t[item_count]);
    memory_required = ALIGN_TO(memory_required, DATA_ALIGNMENT);

    memory_required += sizeof(uint8_t[data_count]);
    memory_required = ALIGN_TO(memory_required, METADATA_PACKET_ALIGNMENT);

    METADATA_DEBUG_LOG("calculate_camera_metadata_memory_required memory_required: %{public}zu", memory_required);
    METADATA_DEBUG_LOG("calculate_camera_metadata_memory_required end");
    return memory_required;
}

common_metadata_header_t *allocate_camera_metadata_buffer(uint32_t item_capacity, uint32_t data_capacity)
{
    METADATA_DEBUG_LOG("allocate_camera_metadata_buffer start");
    METADATA_DEBUG_LOG("allocate_camera_metadata_buffer item_capacity: %{public}d, data_capacity: %{public}d",
        item_capacity, data_capacity);
    size_t memory_required = calculate_camera_metadata_memory_required(item_capacity,
                                                                       data_capacity);
    void *buffer = calloc(1, memory_required);
    if (buffer == NULL) {
        METADATA_ERR_LOG("allocate_camera_metadata_buffer memory allocation failed");
        return buffer;
    }

    common_metadata_header_t *metadata_header = fill_camera_metadata(buffer, memory_required,
                                                                     item_capacity, data_capacity);
    if (metadata_header == NULL) {
        METADATA_ERR_LOG("allocate_camera_metadata_buffer metadata_header is null");
        free(buffer);
    }

    METADATA_DEBUG_LOG("allocate_camera_metadata_buffer end");
    return metadata_header;
}

int32_t get_metadata_section(uint32_t item_section, uint32_t *section)
{
    METADATA_DEBUG_LOG("get_metadata_section start");
    if (item_section < OHOS_CAMERA_PROPERTIES ||
        item_section >= OHOS_ABILITY_SECTION_END) {
        METADATA_ERR_LOG("get_metadata_section item_section is not valid");
        return CAM_META_FAILURE;
    }

    int32_t ret = CAM_META_SUCCESS;
    switch (item_section) {
        case OHOS_CAMERA_PROPERTIES:
            *section = OHOS_SECTION_CAMERA_PROPERTIES;
            break;
        case OHOS_CAMERA_SENSOR:
            *section = OHOS_SECTION_CAMERA_SENSOR;
            break;
        case OHOS_CAMERA_SENSOR_INFO:
            *section = OHOS_SECTION_CAMERA_SENSOR_INFO;
            break;
        case OHOS_CAMERA_STATISTICS:
            *section = OHOS_SECTION_CAMERA_STATISTICS;
            break;
        case OHOS_DEVICE_CONTROL:
            *section = OHOS_SECTION_CAMERA_CONTROL;
            break;
        case OHOS_DEVICE_EXPOSURE:
            *section = OHOS_SECTION_DEVICE_EXPOSURE;
            break;
        case OHOS_DEVICE_FOCUS:
            *section = OHOS_SECTION_DEVICE_FOCUS;
            break;
        case OHOS_DEVICE_FLASH:
            *section = OHOS_SECTION_DEVICE_FLASH;
            break;
        case OHOS_DEVICE_ZOOM:
            *section = OHOS_SECTION_DEVICE_ZOOM;
            break;
        case OHOS_STREAM_ABILITY:
            *section = OHOS_SECTION_STREAM_ABILITY;
            break;
        case OHOS_STREAM_JPEG:
            *section = OHOS_SECTION_STREAM_JPEG;
            break;
        default:
            METADATA_ERR_LOG("get_metadata_section item section is not defined");
            ret = CAM_META_FAILURE;
            break;
    }

    METADATA_DEBUG_LOG("get_metadata_section end");
    return ret;
}

int32_t get_camera_metadata_item_type(uint32_t item, uint32_t *data_type)
{
    METADATA_DEBUG_LOG("get_camera_metadata_item_type start");
    uint32_t section;
    int32_t ret = get_metadata_section(item >> BITWISE_SHIFT_16, &section);
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("get_camera_metadata_item_type section is not valid");
        return ret;
    }

    if (item >= ohos_camera_section_bounds[section][1]) {
        METADATA_ERR_LOG("get_camera_metadata_item_type item is not in section bound");
        return CAM_META_FAILURE;
    }

    uint32_t item_index = item & 0xFFFF;
    if (ohos_item_info[section][item_index].item_type < META_TYPE_BYTE) {
        METADATA_ERR_LOG("get_camera_metadata_item_type item is not initialized");
        return CAM_META_FAILURE;
    }

    *data_type = ohos_item_info[section][item_index].item_type;

    METADATA_DEBUG_LOG("get_camera_metadata_item_type end");
    return CAM_META_SUCCESS;
}

const char *get_camera_metadata_item_name(uint32_t item)
{
    METADATA_DEBUG_LOG("get_camera_metadata_item_name start");
    METADATA_DEBUG_LOG("get_camera_metadata_item_name item: %{public}d", item);
    uint32_t section;
    int32_t ret = get_metadata_section(item >> BITWISE_SHIFT_16, &section);
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("get_camera_metadata_item_name section is not valid");
        return NULL;
    }

    if (item >= ohos_camera_section_bounds[section][1]) {
        METADATA_ERR_LOG("get_camera_metadata_item_name item is not in section bound");
        return NULL;
    }

    uint32_t item_index = item & 0xFFFF;
    METADATA_DEBUG_LOG("get_camera_metadata_item_name end");
    return ohos_item_info[section][item_index].item_name;
}

size_t calculate_camera_metadata_item_data_size(uint32_t type, size_t data_count)
{
    METADATA_DEBUG_LOG("calculate_camera_metadata_item_data_size start");
    if (type < META_TYPE_BYTE || type >= META_NUM_TYPES) {
        METADATA_ERR_LOG("calculate_camera_metadata_item_data_size invalid type");
        return 0;
    }

    size_t data_bytes = data_count * ohos_camera_metadata_type_size[type];

    METADATA_DEBUG_LOG("calculate_camera_metadata_item_data_size end");
    return (data_bytes <= METADATA_HEADER_DATA_SIZE) ? 0 : ALIGN_TO(data_bytes, DATA_ALIGNMENT);
}

int add_camera_metadata_item(common_metadata_header_t *dst, uint32_t item, const void *data,
                             size_t data_count)
{
    METADATA_DEBUG_LOG("add_camera_metadata_item start");
    METADATA_DEBUG_LOG("add_camera_metadata_item item: %{public}d, data_count: %{public}zu", item, data_count);
    if (dst == NULL) {
        METADATA_ERR_LOG("add_camera_metadata_item common_metadata_header_t is null");
        return CAM_META_INVALID_PARAM;
    }

    if (dst->item_count == dst->item_capacity) {
        METADATA_ERR_LOG("add_camera_metadata_item item_capacity limit reached. "
                         "item_count: %{public}d, item_capacity: %{public}d", dst->item_count, dst->item_capacity);
        return CAM_META_ITEM_CAP_EXCEED;
    }

    if (data_count && data == NULL) {
        METADATA_ERR_LOG("add_camera_metadata_item data is not valid. data_count: %{public}zu, data: %{public}p",
                         data_count, data);
        return CAM_META_INVALID_PARAM;
    }

    uint32_t data_type;
    int32_t ret = get_camera_metadata_item_type(item, &data_type);
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("add_camera_metadata_item invalid item type");
        return CAM_META_INVALID_PARAM;
    }

    size_t data_bytes =
            calculate_camera_metadata_item_data_size(data_type, data_count);
    if (data_bytes + dst->data_count > dst->data_capacity) {
        METADATA_ERR_LOG("add_camera_metadata_item data_capacity limit reached");
        return CAM_META_DATA_CAP_EXCEED;
    }

    size_t data_payload_bytes =
            data_count * ohos_camera_metadata_type_size[data_type];
    camera_metadata_item_entry_t *metadata_item = get_metadata_items(dst) + dst->item_count;
    memset_s(metadata_item, sizeof(camera_metadata_item_entry_t), 0, sizeof(camera_metadata_item_entry_t));
    metadata_item->item = item;
    metadata_item->data_type = data_type;
    metadata_item->count = data_count;

    if (data_bytes == 0) {
        ret = memcpy_s(metadata_item->data.value, data_payload_bytes, data, data_payload_bytes);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("add_camera_metadata_item memory copy failed");
            return CAM_META_FAILURE;
        }
    } else {
        metadata_item->data.offset = dst->data_count;
        ret = memcpy_s(get_metadata_data(dst) + metadata_item->data.offset, data_payload_bytes, data,
            data_payload_bytes);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("add_camera_metadata_item memory copy failed");
            return CAM_META_FAILURE;
        }
        dst->data_count += data_bytes;
    }
    dst->item_count++;

    METADATA_DEBUG_LOG("add_camera_metadata_item end");
    return CAM_META_SUCCESS;
}

int get_camera_metadata_item(common_metadata_header_t *src, uint32_t index, camera_metadata_item_t *item)
{
    METADATA_DEBUG_LOG("get_camera_metadata_item start");
    if (src == NULL || item == NULL) {
        METADATA_ERR_LOG("get_camera_metadata_item src or item is null");
        return CAM_META_INVALID_PARAM;
    }

    memset_s(item, sizeof(camera_metadata_item_t), 0, sizeof(camera_metadata_item_t));

    if (index >= src->item_count) {
        METADATA_ERR_LOG("get_camera_metadata_item index is greater than item count");
        return CAM_META_INVALID_PARAM;
    }

    camera_metadata_item_entry_t *local_item = get_metadata_items(src) + index;

    item->index = index;
    item->item = local_item->item;
    item->data_type = local_item->data_type;
    item->count = local_item->count;

    size_t data_bytes = calculate_camera_metadata_item_data_size(local_item->data_type, local_item->count);
    if (data_bytes == 0) {
        item->data.u8 = local_item->data.value;
    } else {
        item->data.u8 = get_metadata_data(src) + local_item->data.offset;
    }

    METADATA_DEBUG_LOG("get_camera_metadata_item end");
    return CAM_META_SUCCESS;
}

int find_camera_metadata_item_index(common_metadata_header_t *src, uint32_t item, uint32_t *idx)
{
    METADATA_DEBUG_LOG("find_camera_metadata_item_index start");
    METADATA_DEBUG_LOG("find_camera_metadata_item_index item: %{public}d", item);
    if (src == NULL) {
        METADATA_ERR_LOG("find_camera_metadata_item_index src is null");
        return CAM_META_INVALID_PARAM;
    }

    camera_metadata_item_entry_t *search_item = get_metadata_items(src);
    uint32_t index;
    for (index = 0; index < src->item_count; index++, search_item++) {
        if (search_item->item == item) {
            break;
        }
    }

    if (index == src->item_count) {
        METADATA_ERR_LOG("find_camera_metadata_item_index item not found");
        return CAM_META_ITEM_NOT_FOUND;
    }

    *idx = index;
    METADATA_DEBUG_LOG("find_camera_metadata_item_index index: %{public}d", index);
    METADATA_DEBUG_LOG("find_camera_metadata_item_index end");
    return CAM_META_SUCCESS;
}

int find_camera_metadata_item(common_metadata_header_t *src, uint32_t item, camera_metadata_item_t *metadata_item)
{
    uint32_t index = 0;
    int ret = find_camera_metadata_item_index(src, item, &index);
    if (ret != CAM_META_SUCCESS) {
        return ret;
    }

    return get_camera_metadata_item(src, index, metadata_item);
}

int update_camera_metadata_item_by_index(common_metadata_header_t *dst, uint32_t index,
                                         const void *data, uint32_t data_count,
                                         camera_metadata_item_t *updated_item)
{
    METADATA_DEBUG_LOG("update_camera_metadata_item_by_index start");
    if (dst == NULL) {
        METADATA_ERR_LOG("update_camera_metadata_item_by_index dst is null");
        return CAM_META_INVALID_PARAM;
    }

    if (index >= dst->item_count) {
        METADATA_ERR_LOG("update_camera_metadata_item_by_index index not valid");
        return CAM_META_INVALID_PARAM;
    }

    int32_t ret;
    camera_metadata_item_entry_t *item = get_metadata_items(dst) + index;
    size_t data_size = calculate_camera_metadata_item_data_size(item->data_type, data_count);
    size_t data_payload_size = data_count * ohos_camera_metadata_type_size[item->data_type];

    size_t old_item_size = calculate_camera_metadata_item_data_size(item->data_type, item->count);
    if (data_size != old_item_size) {
        if (dst->data_capacity < (dst->data_count + data_size - old_item_size)) {
            METADATA_ERR_LOG("update_camera_metadata_item_by_index data_capacity limit reached");
            return CAM_META_DATA_CAP_EXCEED;
        }

        if (old_item_size != 0) {
            uint8_t *start = get_metadata_data(dst) + item->data.offset;
            uint8_t *end = start + old_item_size;
            size_t length = dst->data_count - item->data.offset - old_item_size;
            ret = (length != 0) ? memmove_s(start, length, end, length) : CAM_META_SUCCESS;
            if (ret != CAM_META_SUCCESS) {
                METADATA_ERR_LOG("update_camera_metadata_item_by_index memory move failed");
                return CAM_META_FAILURE;
            }
            dst->data_count -= old_item_size;

            camera_metadata_item_entry_t *metadata_items = get_metadata_items(dst);
            for (uint32_t i = 0; i < dst->item_count; i++, ++metadata_items) {
                if (calculate_camera_metadata_item_data_size(
                    metadata_items->data_type, metadata_items->count) > 0 &&
                    metadata_items->data.offset > item->data.offset) {
                    metadata_items->data.offset -= old_item_size;
                }
            }
        }

        if (data_size != 0) {
            item->data.offset = dst->data_count;
            ret = memcpy_s(get_metadata_data(dst) + item->data.offset, data_payload_size, data, data_payload_size);
            if (ret != CAM_META_SUCCESS) {
                METADATA_ERR_LOG("update_camera_metadata_item_by_index memory copy failed");
                return CAM_META_FAILURE;
            }
            dst->data_count += data_size;
        }
    } else if (data_size != 0) {
        ret = memcpy_s(get_metadata_data(dst) + item->data.offset, data_payload_size, data, data_payload_size);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("update_camera_metadata_item_by_index memory copy failed");
            return CAM_META_FAILURE;
        }
    }

    if (data_size == 0) {
        ret = memcpy_s(item->data.value, data_payload_size, data, data_payload_size);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("update_camera_metadata_item_by_index memory copy failed");
            return CAM_META_FAILURE;
        }
    }

    item->count = data_count;
    if (updated_item != NULL) {
        get_camera_metadata_item(dst, index, updated_item);
    }

    METADATA_DEBUG_LOG("update_camera_metadata_item_by_index end");
    return CAM_META_SUCCESS;
}

int update_camera_metadata_item(common_metadata_header_t *dst, uint32_t item, const void *data,
                                uint32_t data_count, camera_metadata_item_t *updated_item)
{
    METADATA_DEBUG_LOG("update_camera_metadata_item item: %{public}d, data_count: %{public}d", item, data_count);
    uint32_t index = 0;
    uint32_t ret = find_camera_metadata_item_index(dst, item, &index);
    if (ret != CAM_META_SUCCESS) {
        return ret;
    }

    return update_camera_metadata_item_by_index(dst, index, data, data_count, updated_item);
}

int delete_camera_metadata_item_by_index(common_metadata_header_t *dst, uint32_t index)
{
    METADATA_DEBUG_LOG("delete_camera_metadata_item_by_index start");
    if (dst == NULL) {
        METADATA_ERR_LOG("delete_camera_metadata_item_by_index dst is null");
        return CAM_META_INVALID_PARAM;
    }

    if (index >= dst->item_count) {
        METADATA_ERR_LOG("delete_camera_metadata_item_by_index item not valid");
        return CAM_META_INVALID_PARAM;
    }

    int32_t ret;
    camera_metadata_item_entry_t *item_to_delete = get_metadata_items(dst) + index;
    size_t data_bytes = calculate_camera_metadata_item_data_size(item_to_delete->data_type, item_to_delete->count);
    if (data_bytes > 0) {
        uint8_t *start = get_metadata_data(dst) + item_to_delete->data.offset;
        uint8_t *end = start + data_bytes;
        size_t length = dst->data_count - item_to_delete->data.offset - data_bytes;
        ret = (length != 0) ? memmove_s(start, length, end, length) : CAM_META_SUCCESS;
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("delete_camera_metadata_item_by_index memory move failed");
            return CAM_META_FAILURE;
        }
        dst->data_count -= data_bytes;

        camera_metadata_item_entry_t *metadata_items = get_metadata_items(dst);
        for (uint32_t i = 0; i < dst->item_count; i++, ++metadata_items) {
            if (calculate_camera_metadata_item_data_size(
                metadata_items->data_type, metadata_items->count) > 0 &&
                metadata_items->data.offset > item_to_delete->data.offset) {
                metadata_items->data.offset -= data_bytes;
            }
        }
    }

    int32_t length = sizeof(camera_metadata_item_entry_t) * (dst->item_count - index - 1);
    ret = (length != 0) ? memmove_s(item_to_delete, length, item_to_delete + 1, length) : CAM_META_SUCCESS;
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("delete_camera_metadata_item_by_index memory move failed");
        return CAM_META_FAILURE;
    }
    dst->item_count -= 1;

    METADATA_DEBUG_LOG("delete_camera_metadata_item_by_index end");
    return CAM_META_SUCCESS;
}

int delete_camera_metadata_item(common_metadata_header_t *dst, uint32_t item)
{
    METADATA_DEBUG_LOG("delete_camera_metadata_item item: %{public}d", item);
    uint32_t index = 0;
    uint32_t ret = find_camera_metadata_item_index(dst, item, &index);
    if (ret != CAM_META_SUCCESS) {
        return ret;
    }

    return delete_camera_metadata_item_by_index(dst, index);
}

void free_camera_metadata_buffer(common_metadata_header_t *dst)
{
    if (dst != NULL) {
        free(dst);
    }
}

uint32_t get_camera_metadata_item_count(const common_metadata_header_t *metadata_header)
{
    return metadata_header->item_count;
}

uint32_t get_camera_metadata_item_capacity(const common_metadata_header_t *metadata_header)
{
    return metadata_header->item_capacity;
}

uint32_t get_camera_metadata_data_size(const common_metadata_header_t *metadata_header)
{
    return metadata_header->data_capacity;
}

uint32_t copy_camera_metadata_items(common_metadata_header_t *newMetadata, common_metadata_header_t *oldMetadata)
{
    if (newMetadata == NULL || oldMetadata == NULL) {
        return CAM_META_INVALID_PARAM;
    }

    int32_t ret;
    ret = memcpy_s(get_metadata_items(newMetadata), sizeof(camera_metadata_item_entry_t[oldMetadata->item_count]),
        get_metadata_items(oldMetadata), sizeof(camera_metadata_item_entry_t[oldMetadata->item_count]));
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("copy_camera_metadata_items memory copy failed");
        return CAM_META_FAILURE;
    }

    ret = memcpy_s(get_metadata_data(newMetadata), sizeof(uint8_t[oldMetadata->data_count]),
        get_metadata_data(oldMetadata), sizeof(uint8_t[oldMetadata->data_count]));
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("copy_camera_metadata_items memory copy failed");
        return CAM_META_FAILURE;
    }

    newMetadata->item_count = oldMetadata->item_count;
    newMetadata->data_count = oldMetadata->data_count;

    return CAM_META_SUCCESS;
}
