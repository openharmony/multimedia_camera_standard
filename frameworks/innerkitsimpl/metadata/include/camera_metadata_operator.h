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

#ifndef CAMERA_METADATA_OPERATOR_H
#define CAMERA_METADATA_OPERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "camera_device_ability_items.h"
#include <stdio.h>
#include <stdint.h>

/** Versioning information */
#define CURRENT_CAMERA_METADATA_VERSION 1

#define ALIGN_TO(val, alignment) \
    (((uintptr_t)(val) + ((alignment) - 1)) & ~((alignment) - 1))

#define ITEM_ALIGNMENT ((size_t) 4)

#define DATA_ALIGNMENT ((size_t) 8)

#define METADATA_ALIGNMENT ((size_t) 4)

#define MAX_ALIGNMENT(A, B) (((A) > (B)) ? (A) : (B))
#define METADATA_PACKET_ALIGNMENT \
    MAX_ALIGNMENT(MAX_ALIGNMENT(DATA_ALIGNMENT, METADATA_ALIGNMENT), ITEM_ALIGNMENT)

// data type
enum {
    // uint8_t
    META_TYPE_BYTE = 0,
    // int32_t
    META_TYPE_INT32 = 1,
    // uint32_t
    META_TYPE_UINT32 = 2,
    // float
    META_TYPE_FLOAT = 3,
    // int64_t
    META_TYPE_INT64 = 4,
    // double
    META_TYPE_DOUBLE = 5,
    // A 64-bit fraction (camera_metadata_rational_t)
    META_TYPE_RATIONAL = 6,
    // Number of data type
    META_NUM_TYPES
};

typedef struct camera_rational {
    int32_t numerator;
    int32_t denominator;
} camera_rational_t;

// common metadata header
typedef struct common_metadata_header {
    uint32_t version;
    uint32_t size;
    uint32_t item_count;
    uint32_t item_capacity;
    uint32_t items_start; // Offset from common_metadata_header
    uint32_t data_count;
    uint32_t data_capacity;
    uint32_t data_start; // Offset from common_metadata_header
} common_metadata_header_t;

typedef struct camera_metadata_item_entry {
    uint32_t item;
    uint32_t data_type;
    uint32_t count;
    union {
        uint32_t offset;
        uint8_t  value[4];
    } data;
} camera_metadata_item_entry_t;

typedef struct camera_metadata_item {
    uint32_t index;
    uint32_t item;
    uint32_t data_type;
    uint32_t count;
    union {
        uint8_t  *u8;
        int32_t  *i32;
        uint32_t *ui32;
        float    *f;
        int64_t  *i64;
        double   *d;
        camera_rational_t *r;
      } data;
} camera_metadata_item_t;

typedef struct item_info {
    const char *item_name;
    uint8_t item_type;
    int32_t dataCnt;
} item_info_t;

/* camera_metadata_section */
typedef enum camera_metadata_sec {
    OHOS_SECTION_CAMERA_PROPERTIES,
    OHOS_SECTION_CAMERA_SENSOR,
    OHOS_SECTION_CAMERA_SENSOR_INFO,
    OHOS_SECTION_CAMERA_STATISTICS,
    OHOS_SECTION_CAMERA_CONTROL,
    OHOS_SECTION_DEVICE_EXPOSURE,
    OHOS_SECTION_DEVICE_FOCUS,
    OHOS_SECTION_DEVICE_FLASH,
    OHOS_SECTION_DEVICE_ZOOM,
    OHOS_SECTION_STREAM_ABILITY,
    OHOS_SECTION_STREAM_JPEG,
    OHOS_SECTION_COUNT,
} camera_metadata_sec_t;

/* Return codes */
#define CAM_META_FAILURE         -1
#define CAM_META_SUCCESS         0
#define CAM_META_INVALID_PARAM   2
#define CAM_META_ITEM_NOT_FOUND  3
#define CAM_META_ITEM_CAP_EXCEED 4
#define CAM_META_DATA_CAP_EXCEED 5

// Allocate a new camera metadata buffer and return the metadata header
common_metadata_header_t *allocate_camera_metadata_buffer(uint32_t item_capacity,
        uint32_t data_capacity);

// Find camera metadata item and fill the found item
int find_camera_metadata_item(common_metadata_header_t *src,
        uint32_t item,
        camera_metadata_item_t *metadata_item);

// Get camera metadata item name
const char *get_camera_metadata_item_name(uint32_t item);

// Update camera metadata item and fill the updated item
int update_camera_metadata_item(common_metadata_header_t *dst,
        uint32_t item,
        const void *data,
        uint32_t data_count,
        camera_metadata_item_t *updated_item);

// Update camera metadata item by index and fill the updated item
int update_camera_metadata_item_by_index(common_metadata_header_t *dst,
        uint32_t index,
        const void *data,
        uint32_t data_count,
        camera_metadata_item_t *updated_item);

// Add camera metadata item
int add_camera_metadata_item(common_metadata_header_t *dst,
        uint32_t item,
        const void *data,
        size_t data_count);

// Delete camera metadata item
int delete_camera_metadata_item(common_metadata_header_t *dst, uint32_t item);

// Delete camera metadata item by index
int delete_camera_metadata_item_by_index(common_metadata_header_t *dst, uint32_t index);

// Free camera metadata buffer
void free_camera_metadata_buffer(common_metadata_header_t *dst);

// Internal use
camera_metadata_item_entry_t *get_metadata_items(const common_metadata_header_t *metadata_header);
uint8_t *get_metadata_data(const common_metadata_header_t *metadata_header);
int get_camera_metadata_item(common_metadata_header_t *src, uint32_t index,
                             camera_metadata_item_t *item);
uint32_t get_camera_metadata_item_count(const common_metadata_header_t *metadata_header);
uint32_t get_camera_metadata_item_capacity(const common_metadata_header_t *metadata_header);
uint32_t get_camera_metadata_data_size(const common_metadata_header_t *metadata_header);
uint32_t copy_camera_metadata(common_metadata_header_t *newMetadata, common_metadata_header_t *oldMetadata);
size_t calculate_camera_metadata_item_data_size(uint32_t type, size_t data_count);
int32_t get_camera_metadata_item_type(uint32_t item, uint32_t *data_type);

#ifdef __cplusplus
}
#endif

#endif

