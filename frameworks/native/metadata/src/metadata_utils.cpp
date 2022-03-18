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

#include "metadata_utils.h"
#include <securec.h>
#include "metadata_log.h"

namespace OHOS {
namespace CameraStandard {
bool MetadataUtils::EncodeCameraMetadata(const std::shared_ptr<CameraStandard::CameraMetadata> &metadata,
                                         MessageParcel &data)
{
    if (metadata == nullptr) {
        return false;
    }

    bool bRet = true;
    uint32_t tagCount = 0;
    common_metadata_header_t *meta = metadata->get();
    if (meta != nullptr) {
        tagCount = GetCameraMetadataItemCount(meta);
        bRet = bRet && data.WriteUint32(tagCount);
        bRet = bRet && data.WriteUint32(GetCameraMetadataItemCapacity(meta));
        bRet = bRet && data.WriteUint32(GetCameraMetadataDataSize(meta));
        for (uint32_t i = 0; i < tagCount; i++) {
            camera_metadata_item_t item;
            int ret = GetCameraMetadataItem(meta, i, &item);
            if (ret != CAM_META_SUCCESS) {
                return false;
            }

            bRet = bRet && data.WriteUint32(item.index);
            bRet = bRet && data.WriteUint32(item.item);
            bRet = bRet && data.WriteUint32(item.data_type);
            bRet = bRet && data.WriteUint32(item.count);
            bRet = bRet && MetadataUtils::WriteMetadata(item, data);
        }
    } else {
        bRet = data.WriteUint32(tagCount);
    }
    return bRet;
}

void MetadataUtils::DecodeCameraMetadata(MessageParcel &data, std::shared_ptr<CameraStandard::CameraMetadata> &metadata)
{
    uint32_t tagCount = data.ReadUint32();
    uint32_t itemCapacity = data.ReadUint32();
    uint32_t dataCapacity = data.ReadUint32();
    constexpr uint32_t MAX_SUPPORTED_TAGS = 1000;
    constexpr uint32_t MAX_SUPPORTED_ITEMS = 1000;
    constexpr uint32_t MAX_ITEM_CAPACITY = (1000 * 10);
    constexpr uint32_t MAX_DATA_CAPACITY = (1000 * 10 * 10);

    if (tagCount > MAX_SUPPORTED_TAGS) {
        tagCount = MAX_SUPPORTED_TAGS;
        METADATA_ERR_LOG("MetadataUtils::DecodeCameraMetadata tagCount is more than supported value");
    }

    if (itemCapacity > MAX_ITEM_CAPACITY) {
        itemCapacity = MAX_ITEM_CAPACITY;
        METADATA_ERR_LOG("MetadataUtils::DecodeCameraMetadata itemCapacity is more than supported value");
    }

    if (dataCapacity > MAX_DATA_CAPACITY) {
        dataCapacity = MAX_DATA_CAPACITY;
        METADATA_ERR_LOG("MetadataUtils::DecodeCameraMetadata dataCapacity is more than supported value");
    }

    std::vector<camera_metadata_item_t> items;
    for (uint32_t i = 0; i < tagCount; i++) {
        camera_metadata_item_t item;
        item.index = data.ReadUint32();
        item.item = data.ReadUint32();
        item.data_type = data.ReadUint32();
        item.count = data.ReadUint32();
        if (item.count > MAX_SUPPORTED_ITEMS) {
            item.count = MAX_SUPPORTED_ITEMS;
            METADATA_ERR_LOG("MetadataUtils::DecodeCameraMetadata item.count is more than supported value");
        }
        MetadataUtils::ReadMetadata(item, data);
        items.push_back(item);
    }

    metadata = std::make_shared<CameraMetadata>(itemCapacity, dataCapacity);
    common_metadata_header_t *meta = metadata->get();
    for (auto &item : items) {
        void *buffer = nullptr;
        MetadataUtils::ItemDataToBuffer(item, &buffer);
        (void)AddCameraMetadataItem(meta, item.item, buffer, item.count);
    }
}

bool MetadataUtils::WriteMetadata(const camera_metadata_item_t &item, MessageParcel &data)
{
    bool bRet = false;
    if (item.data_type == META_TYPE_BYTE) {
        std::vector<uint8_t> buffers;
        for (size_t i = 0; i < item.count; i++) {
            buffers.push_back(*(item.data.u8 + i));
        }
        bRet = data.WriteUInt8Vector(buffers);
    } else if (item.data_type == META_TYPE_INT32) {
        std::vector<int32_t> buffers;
        for (size_t i = 0; i < item.count; i++) {
            buffers.push_back(*(item.data.i32 + i));
        }
        bRet = data.WriteInt32Vector(buffers);
    } else if (item.data_type == META_TYPE_FLOAT) {
        std::vector<float> buffers;
        for (size_t i = 0; i < item.count; i++) {
            buffers.push_back(*(item.data.f + i));
        }
        bRet = data.WriteFloatVector(buffers);
    } else if (item.data_type == META_TYPE_UINT32) {
        std::vector<uint32_t> buffers;
        for (size_t i = 0; i < item.count; i++) {
            buffers.push_back(*(item.data.ui32 + i));
        }
        bRet = data.WriteUInt32Vector(buffers);
    } else if (item.data_type == META_TYPE_INT64) {
        std::vector<int64_t> buffers;
        for (size_t i = 0; i < item.count; i++) {
            buffers.push_back(*(item.data.i64 + i));
        }
        bRet = data.WriteInt64Vector(buffers);
    } else if (item.data_type == META_TYPE_DOUBLE) {
        std::vector<double> buffers;
        for (size_t i = 0; i < item.count; i++) {
            buffers.push_back(*(item.data.d + i));
        }
        bRet = data.WriteDoubleVector(buffers);
    } else if (item.data_type == META_TYPE_RATIONAL) {
        std::vector<int32_t> buffers;
        for (size_t i = 0; i < item.count; i++) {
            buffers.push_back((*(item.data.r + i)).numerator);
            buffers.push_back((*(item.data.r + i)).denominator);
        }
        bRet = data.WriteInt32Vector(buffers);
    }

    return bRet;
}

std::string MetadataUtils::EncodeToString(std::shared_ptr<CameraStandard::CameraMetadata> metadata)
{
    int32_t ret, dataLen;
    const int32_t headerLength = sizeof(common_metadata_header_t);
    const int32_t itemLen = sizeof(camera_metadata_item_entry_t);
    const int32_t itemFixedLen = static_cast<int32_t>(offsetof(camera_metadata_item_entry_t, data));

    if (metadata == nullptr || metadata->get() == nullptr) {
        METADATA_ERR_LOG("MetadataUtils::EncodeToString Metadata is invalid");
        return {};
    }

    common_metadata_header_t *meta = metadata->get();
    std::string s(headerLength + (itemLen * meta->item_count) + meta->data_count, '\0');
    char *encodeData = &s[0];
    ret = memcpy_s(encodeData, headerLength, meta, headerLength);
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("MetadataUtils::EncodeToString Failed to copy memory for metadata header");
        return {};
    }
    encodeData += headerLength;
    camera_metadata_item_entry_t *item = GetMetadataItems(meta);
    for (uint32_t index = 0; index < meta->item_count; index++, item++) {
        ret = memcpy_s(encodeData, itemFixedLen, item, itemFixedLen);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("MetadataUtils::EncodeToString Failed to copy memory for item fixed fields");
            return {};
        }
        encodeData += itemFixedLen;
        dataLen = itemLen - itemFixedLen;
        ret = memcpy_s(encodeData, dataLen,  &(item->data), dataLen);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("MetadataUtils::EncodeToString Failed to copy memory for item data field");
            return {};
        }
        encodeData += dataLen;
    }

    if (meta->data_count != 0) {
        ret = memcpy_s(encodeData, meta->data_count, GetMetadataData(meta), meta->data_count);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("MetadataUtils::EncodeToString Failed to copy memory for data");
            return {};
        }
        encodeData += meta->data_count;
    }
    METADATA_DEBUG_LOG("MetadataUtils::EncodeToString Calculated length: %{public}d, encoded length: %{public}d",
                       s.capacity(), (encodeData - &s[0]));

    return s;
}

std::shared_ptr<CameraStandard::CameraMetadata> MetadataUtils::DecodeFromString(std::string setting)
{
    uint32_t ret, dataLen;
    uint32_t totalLen = setting.capacity();
    const uint32_t headerLength = sizeof(common_metadata_header_t);
    const uint32_t itemLen = sizeof(camera_metadata_item_entry_t);
    const uint32_t itemFixedLen = offsetof(camera_metadata_item_entry_t, data);

    if (totalLen < headerLength) {
        METADATA_ERR_LOG("MetadataUtils::DecodeFromString Length is less than metadata header length");
        return {};
    }

    char *decodeData = &setting[0];
    common_metadata_header_t header;
    ret = memcpy_s(&header, headerLength, decodeData, headerLength);
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("MetadataUtils::DecodeFromString Failed to copy memory for metadata header");
        return {};
    }
    header.item_capacity = header.item_count;
    header.data_capacity = header.data_count;
    std::shared_ptr<CameraStandard::CameraMetadata> metadata
        = std::make_shared<CameraMetadata>(header.item_capacity, header.data_capacity);
    common_metadata_header_t *meta = metadata->get();
    ret = memcpy_s(meta, headerLength, &header, headerLength);
    if (ret != CAM_META_SUCCESS) {
        METADATA_ERR_LOG("MetadataUtils::DecodeFromString Failed to copy memory for metadata header");
        return {};
    }
    decodeData += headerLength;
    camera_metadata_item_entry_t *item = GetMetadataItems(meta);
    for (uint32_t index = 0; index < meta->item_count; index++, item++) {
        if (totalLen < ((decodeData - &setting[0]) + itemLen)) {
            METADATA_ERR_LOG("MetadataUtils::DecodeFromString Failed at item index: %{public}d", index);
            return {};
        }
        ret = memcpy_s(item, itemFixedLen, decodeData, itemFixedLen);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("MetadataUtils::DecodeFromString Failed to copy memory for item fixed fields");
            return {};
        }
        decodeData += itemFixedLen;
        dataLen = itemLen - itemFixedLen;
        ret = memcpy_s(&(item->data), dataLen,  decodeData, dataLen);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("MetadataUtils::DecodeFromString Failed to copy memory for item data field");
            return {};
        }
        decodeData += dataLen;
    }

    if (meta->data_count != 0) {
        if (totalLen < static_cast<uint32_t>(((decodeData - &setting[0]) + meta->data_count))) {
            METADATA_ERR_LOG("MetadataUtils::DecodeFromString Failed at data copy");
            return {};
        }
        ret = memcpy_s(GetMetadataData(meta), meta->data_count, decodeData, meta->data_count);
        if (ret != CAM_META_SUCCESS) {
            METADATA_ERR_LOG("MetadataUtils::DecodeFromString Failed to copy memory for data");
            return {};
        }
        decodeData += meta->data_count;
    }

    METADATA_DEBUG_LOG("MetadataUtils::DecodeFromString String length: %{public}d, Decoded length: %{public}d",
                       setting.capacity(), (decodeData - &setting[0]));
    return metadata;
}

bool MetadataUtils::ReadMetadata(camera_metadata_item_t &item, MessageParcel &data)
{
    if (item.data_type == META_TYPE_BYTE) {
        std::vector<uint8_t> buffers;
        data.ReadUInt8Vector(&buffers);
        item.data.u8 = new(std::nothrow) uint8_t[item.count];
        if (item.data.u8 != nullptr) {
            for (size_t i = 0; i < item.count; i++) {
                item.data.u8[i] = buffers.at(i);
            }
        }
    } else if (item.data_type == META_TYPE_INT32) {
        std::vector<int32_t> buffers;
        data.ReadInt32Vector(&buffers);
        item.data.i32 = new(std::nothrow) int32_t[item.count];
        if (item.data.i32 != nullptr) {
            for (size_t i = 0; i < item.count; i++) {
                item.data.i32[i] = buffers.at(i);
            }
        }
    } else if (item.data_type == META_TYPE_FLOAT) {
        std::vector<float> buffers;
        data.ReadFloatVector(&buffers);
        item.data.f = new(std::nothrow) float[item.count];
        if (item.data.f != nullptr) {
            for (size_t i = 0; i < item.count; i++) {
                item.data.f[i] = buffers.at(i);
            }
        }
    } else if (item.data_type == META_TYPE_UINT32) {
        std::vector<uint32_t> buffers;
        data.ReadUInt32Vector(&buffers);
        item.data.ui32 = new(std::nothrow) uint32_t[item.count];
        if (item.data.ui32 != nullptr) {
            for (size_t i = 0; i < item.count; i++) {
                item.data.ui32[i] = buffers.at(i);
            }
        }
    } else if (item.data_type == META_TYPE_INT64) {
        std::vector<int64_t> buffers;
        data.ReadInt64Vector(&buffers);
        item.data.i64 = new(std::nothrow) int64_t[item.count];
        if (item.data.i64 != nullptr) {
            for (size_t i = 0; i < item.count; i++) {
                item.data.i64[i] = buffers.at(i);
            }
        }
    } else if (item.data_type == META_TYPE_DOUBLE) {
        std::vector<double> buffers;
        data.ReadDoubleVector(&buffers);
        item.data.d = new(std::nothrow) double[item.count];
        if (item.data.d != nullptr) {
            for (size_t i = 0; i < item.count; i++) {
                item.data.d[i] = buffers.at(i);
            }
        }
    } else if (item.data_type == META_TYPE_RATIONAL) {
        std::vector<int32_t> buffers;
        data.ReadInt32Vector(&buffers);
        item.data.r = new(std::nothrow) camera_rational_t[item.count];
        if (item.data.r != nullptr) {
            for (size_t i = 0, j = 0;
                    i < item.count && j < static_cast<size_t>(buffers.size());
                    i++, j += INDEX_COUNTER) {
                item.data.r[i].numerator = buffers.at(j);
                item.data.r[i].denominator = buffers.at(j + 1);
            }
        }
    }
    return true;
}

void MetadataUtils::ItemDataToBuffer(const camera_metadata_item_t &item, void **buffer)
{
    if (buffer == nullptr) {
        METADATA_ERR_LOG("MetadataUtils::ItemDataToBuffer buffer is null");
        return;
    }
    if (item.data_type == META_TYPE_BYTE) {
        *buffer = (void*)item.data.u8;
    } else if (item.data_type == META_TYPE_INT32) {
        *buffer = (void*)item.data.i32;
    } else if (item.data_type == META_TYPE_FLOAT) {
        *buffer = (void*)item.data.f;
    } else if (item.data_type == META_TYPE_UINT32) {
        *buffer = (void*)item.data.ui32;
    } else if (item.data_type == META_TYPE_INT64) {
        *buffer = (void*)item.data.i64;
    } else if (item.data_type == META_TYPE_DOUBLE) {
        *buffer = (void*)item.data.d;
    } else if (item.data_type == META_TYPE_RATIONAL) {
        *buffer = (void*)item.data.r;
    }
}
} // CameraStandard
} // OHOS
