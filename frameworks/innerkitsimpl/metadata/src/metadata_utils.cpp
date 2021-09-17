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

#include "metadata_utils.h"

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
        tagCount = get_camera_metadata_item_count(meta);
        bRet = bRet && data.WriteUint32(tagCount);
        bRet = bRet && data.WriteUint32(get_camera_metadata_item_capacity(meta));
        bRet = bRet && data.WriteUint32(get_camera_metadata_data_size(meta));
        for (uint32_t i = 0; i < tagCount; i++) {
            camera_metadata_item_t item;
            int ret = get_camera_metadata_item(meta, i, &item);
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

    std::vector<camera_metadata_item_t> items;
    for (uint32_t i = 0; i < tagCount; i++) {
        camera_metadata_item_t item;
        item.index = data.ReadUint32();
        item.item = data.ReadUint32();
        item.data_type = data.ReadUint32();
        item.count = data.ReadUint32();
        MetadataUtils::ReadMetadata(item, data);
        items.push_back(item);
    }

    metadata = std::make_shared<CameraMetadata>(itemCapacity, dataCapacity);
    common_metadata_header_t *meta = metadata->get();
    for (auto &item : items) {
        void *buffer = nullptr;
        MetadataUtils::ItemDataToBuffer(item, &buffer);
        (void)add_camera_metadata_item(meta, item.item, buffer, item.count);
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
                    i++, j += 2) {
                item.data.r[i].numerator = buffers.at(j);
                item.data.r[i].denominator = buffers.at(j + 1);
            }
        }
    }
    return true;
}

void MetadataUtils::ItemDataToBuffer(const camera_metadata_item_t &item, void **buffer)
{
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
}
}
