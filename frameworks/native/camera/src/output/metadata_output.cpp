/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "output/metadata_output.h"

#include <set>

#include "camera_util.h"
#include "camera_log.h"
#include "input/camera_input.h"
#include "session/capture_session.h"

namespace OHOS {
namespace CameraStandard {
MetadataFaceObject::MetadataFaceObject(double timestamp, Rect rect)
    : MetadataObject(MetadataObjectType::FACE, timestamp, rect)
{}

MetadataObject::MetadataObject(MetadataObjectType type, double timestamp, Rect rect)
    : type_(type), timestamp_(timestamp), box_(rect)
{}

MetadataObjectType MetadataObject::GetType()
{
    return type_;
}
double MetadataObject::GetTimestamp()
{
    return timestamp_;
}
Rect MetadataObject::GetBoundingBox()
{
    return box_;
}

MetadataOutput::MetadataOutput(sptr<Surface> surface, sptr<IStreamMetadata> &streamMetadata)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE_METADATA, StreamType::METADATA, streamMetadata)
{
    surface_ = surface;
}

std::vector<MetadataObjectType> MetadataOutput::GetSupportedMetadataObjectTypes()
{
    CaptureSession *captureSession = GetSession();
    if ((captureSession == nullptr) || (captureSession->inputDevice_ == nullptr)) {
        return {};
    }
    sptr<CameraInfo> cameraObj = captureSession->inputDevice_->GetCameraDeviceInfo();
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_STATISTICS_FACE_DETECT_MODE, &item);
    if (ret) {
        return {};
    }
    std::vector<MetadataObjectType> objectTypes;
    for (size_t index = 0; index < item.count; index++) {
        if (item.data.u8[index] == OHOS_CAMERA_FACE_DETECT_MODE_SIMPLE) {
            objectTypes.emplace_back(MetadataObjectType::FACE);
        }
    }
    return objectTypes;
}

void MetadataOutput::SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> metadataObjectTypes)
{
    CaptureSession *captureSession = GetSession();
    if ((captureSession == nullptr) || (captureSession->inputDevice_ == nullptr)) {
        return;
    }
    std::set<camera_face_detect_mode_t> objectTypes;
    for (auto &type : metadataObjectTypes) {
        if (type == MetadataObjectType::FACE) {
            objectTypes.insert(OHOS_CAMERA_FACE_DETECT_MODE_SIMPLE);
        }
    }
    if (objectTypes.empty()) {
        objectTypes.insert(OHOS_CAMERA_FACE_DETECT_MODE_OFF);
    }

    SetCaptureMetadataObjectTypes((sptr<CameraInput> &)captureSession->inputDevice_, objectTypes);
}

void MetadataOutput::SetCallback(std::shared_ptr<MetadataObjectCallback> metadataObjectCallback)
{
    appObjectCallback_ = metadataObjectCallback;
}

void MetadataOutput::SetCallback(std::shared_ptr<MetadataStateCallback> metadataStateCallback)
{
    appStateCallback_ = metadataStateCallback;
}

int32_t MetadataOutput::Start()
{
    return static_cast<IStreamMetadata *>(GetStream().GetRefPtr())->Start();
}

int32_t MetadataOutput::Stop()
{
    return static_cast<IStreamMetadata *>(GetStream().GetRefPtr())->Stop();
}

void MetadataOutput::Release()
{
    int32_t errCode = static_cast<IStreamMetadata *>(GetStream().GetRefPtr())->Release();
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release MetadataOutput!, errCode: %{public}d", errCode);
    }
    if (surface_) {
        SurfaceError ret = surface_->UnregisterConsumerListener();
        if (ret != SURFACE_ERROR_OK) {
            MEDIA_ERR_LOG("Failed to unregister surface consumer listener");
        }
        surface_ = nullptr;
    }
}

MetadataObjectListener::MetadataObjectListener(sptr<MetadataOutput> metadata) : metadata_(metadata)
{}

int32_t MetadataObjectListener::ProcessFaceRectangles(int64_t timestamp, const camera_metadata_item_t &metadataItem,
                                                      std::vector<sptr<MetadataObject>> &metaObjects)
{
    constexpr int32_t rectangleUnitLen = 4;

    if (metadataItem.count % rectangleUnitLen) {
        MEDIA_ERR_LOG("Metadata item: %{public}d count: %{public}d is invalid", metadataItem.item, metadataItem.count);
        return ERROR_UNKNOWN;
    }
    metaObjects.reserve(metadataItem.count / rectangleUnitLen);
    for (float *start = metadataItem.data.f, *end = metadataItem.data.f + metadataItem.count; start < end;
        start += rectangleUnitLen) {
        sptr<MetadataObject> metadataObject = new(std::nothrow) MetadataFaceObject(timestamp,
            (Rect) {start[0], start[1], start[2], start[3]});
        if (!metadataObject) {
            MEDIA_ERR_LOG("Failed to allocate MetadataFaceObject");
            return ERROR_INSUFFICIENT_RESOURCES;
        }
        metaObjects.emplace_back(metadataObject);
    }
    return CAMERA_OK;
}

int32_t MetadataObjectListener::ProcessMetadataBuffer(void *buffer, int64_t timestamp)
{
    if (!buffer) {
        MEDIA_ERR_LOG("Buffer is null");
        return ERROR_UNKNOWN;
    }
    common_metadata_header_t *metadata = (common_metadata_header_t *)buffer;
    uint32_t itemCount = Camera::GetCameraMetadataItemCount(metadata);
    camera_metadata_item_t metadataItem;
    std::vector<sptr<MetadataObject>> metaObjects;
    for (uint32_t i = 0; i < itemCount; i++) {
        int32_t ret = Camera::GetCameraMetadataItem(metadata, i, &metadataItem);
        if (ret) {
            MEDIA_ERR_LOG("Failed to get metadata item at index: %{public}d, with return code: %{public}d", i, ret);
            return ERROR_UNKNOWN;
        }
        if (metadataItem.item == OHOS_STATISTICS_FACE_RECTANGLES) {
            ret = ProcessFaceRectangles(timestamp, metadataItem, metaObjects);
            if (ret) {
                MEDIA_ERR_LOG("Failed to process face rectangles");
                return ret;
            }
        }
    }
    std::shared_ptr<MetadataObjectCallback> appObjectCallback = metadata_->appObjectCallback_;
    if (!metaObjects.empty() && appObjectCallback) {
        appObjectCallback->OnMetadataObjectsAvailable(metaObjects);
    }
    return CAMERA_OK;
}

void MetadataObjectListener::OnBufferAvailable()
{
    if (!metadata_) {
        MEDIA_ERR_LOG("Metadata is null");
        return;
    }
    sptr<Surface> surface = metadata_->surface_;
    if (!surface) {
        MEDIA_ERR_LOG("Metadata surface is null");
        return;
    }
    int32_t fence = -1;
    int64_t timestamp;
    OHOS::Rect damage;
    sptr<SurfaceBuffer> buffer = nullptr;
    SurfaceError surfaceRet = surface->AcquireBuffer(buffer, fence, timestamp, damage);
    if (surfaceRet != SURFACE_ERROR_OK) {
        MEDIA_ERR_LOG("Failed to acquire surface buffer");
        return;
    }
    int32_t ret = ProcessMetadataBuffer(buffer->GetVirAddr(), timestamp);
    if (ret) {
        std::shared_ptr<MetadataStateCallback> appStateCallback = metadata_->appStateCallback_;
        if (appStateCallback) {
            appStateCallback->OnError(ret);
        }
    }

    surface->ReleaseBuffer(buffer, -1);
}
} // CameraStandard
} // OHOS
