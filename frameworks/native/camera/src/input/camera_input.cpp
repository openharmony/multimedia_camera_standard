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

#include "input/camera_input.h"

#include <cinttypes>
#include <set>
#include "camera_device_ability_items.h"
#include "camera_util.h"
#include "hcamera_device_callback_stub.h"
#include "media_log.h"
#include "metadata_utils.h"

namespace OHOS {
namespace CameraStandard {
class CameraDeviceServiceCallback : public HCameraDeviceCallbackStub {
public:
    sptr<CameraInput> camInput_ = nullptr;
    CameraDeviceServiceCallback() : camInput_(nullptr) {
    }

    explicit CameraDeviceServiceCallback(const sptr<CameraInput>& camInput) : camInput_(camInput) {
    }

    ~CameraDeviceServiceCallback()
    {
        camInput_ = nullptr;
    }

    int32_t OnError(const int32_t errorType, const int32_t errorMsg) override
    {
        MEDIA_ERR_LOG("CameraDeviceServiceCallback::OnError() is called!, errorType: %{public}d, errorMsg: %{public}d",
                      errorType, errorMsg);
        if (camInput_ != nullptr && camInput_->GetErrorCallback() != nullptr) {
            camInput_->GetErrorCallback()->OnError(errorType, errorMsg);
        } else {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() is called!, Discarding callback");
        }
        return CAMERA_OK;
    }

    int32_t OnResult(const uint64_t timestamp, const std::shared_ptr<CameraMetadata> &result) override
    {
        MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() is called!, timestamp: %{public}" PRIu64, timestamp);
        camera_metadata_item_t item;
        int ret = FindCameraMetadataItem(result->get(), OHOS_CONTROL_FLASH_STATE, &item);
        if (ret == 0) {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() OHOS_CONTROL_FLASH_STATE is %{public}d",
                           item.data.u8[0]);
        }
        ret = FindCameraMetadataItem(result->get(), OHOS_CONTROL_FLASHMODE, &item);
        if (ret == 0) {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() OHOS_CONTROL_FLASHMODE is %{public}d",
                           item.data.u8[0]);
        }
        ret = FindCameraMetadataItem(result->get(), OHOS_CONTROL_AE_MODE, &item);
        if (ret == 0) {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() OHOS_CONTROL_AE_MODE is %{public}d",
                           item.data.u8[0]);
        }
        camInput_->ProcessAutoFocusUpdates(result);
        return CAMERA_OK;
    }
};

const std::unordered_map<camera_af_state_t, FocusCallback::FocusState> CameraInput::mapFromMetadataFocus_ = {
    {OHOS_CAMERA_AF_STATE_PASSIVE_SCAN, FocusCallback::SCAN},
    {OHOS_CAMERA_AF_STATE_ACTIVE_SCAN, FocusCallback::SCAN},
    {OHOS_CAMERA_AF_STATE_PASSIVE_FOCUSED, FocusCallback::FOCUSED},
    {OHOS_CAMERA_AF_STATE_FOCUSED_LOCKED, FocusCallback::FOCUSED},
    {OHOS_CAMERA_AF_STATE_PASSIVE_UNFOCUSED, FocusCallback::UNFOCUSED},
    {OHOS_CAMERA_AF_STATE_NOT_FOCUSED_LOCKED, FocusCallback::UNFOCUSED},
};

CameraInput::CameraInput(sptr<ICameraDeviceService> &deviceObj,
                         sptr<CameraInfo> &cameraObj) : cameraObj_(cameraObj), deviceObj_(deviceObj)
{
    CameraDeviceSvcCallback_ = new CameraDeviceServiceCallback(this);
    deviceObj_->SetCallback(CameraDeviceSvcCallback_);
}

void CameraInput::Release()
{
    int32_t retCode = deviceObj_->Release();
    if (retCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release Camera Input, retCode: %{public}d", retCode);
    }
    return;
}

void CameraInput::LockForControl()
{
    int32_t items = 10;
    int32_t dataLength = 100;
    changedMetadata_ = std::make_shared<CameraMetadata>(items, dataLength);
    return;
}

int32_t CameraInput::UpdateSetting(std::shared_ptr<CameraMetadata> changedMetadata)
{
    if (!GetCameraMetadataItemCount(changedMetadata->get())) {
        MEDIA_INFO_LOG("CameraInput::UpdateSetting No configuration to update");
        return CAMERA_OK;
    }

    int32_t ret = deviceObj_->UpdateSetting(changedMetadata);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("CameraInput::UpdateSetting Failed to update settings");
        return ret;
    }

    int32_t length;
    int32_t count = changedMetadata->get()->item_count;
    uint8_t *data = GetMetadataData(changedMetadata->get());
    camera_metadata_item_entry_t *itemEntry = GetMetadataItems(changedMetadata->get());
    std::shared_ptr<CameraMetadata> baseMetadata = cameraObj_->GetMetadata();
    for (int32_t i = 0; i < count; i++, itemEntry++) {
        bool status = false;
        camera_metadata_item_t item;
        length = CalculateCameraMetadataItemDataSize(itemEntry->data_type, itemEntry->count);
        ret = FindCameraMetadataItem(baseMetadata->get(), itemEntry->item, &item);
        if (ret == CAM_META_SUCCESS) {
            status = baseMetadata->updateEntry(itemEntry->item,
                                               (length == 0) ? itemEntry->data.value : (data + itemEntry->data.offset),
                                               itemEntry->count);
        } else if (ret == CAM_META_ITEM_NOT_FOUND) {
            status = baseMetadata->addEntry(itemEntry->item,
                                            (length == 0) ? itemEntry->data.value : (data + itemEntry->data.offset),
                                            itemEntry->count);
        }
        if (!status) {
            MEDIA_ERR_LOG("CameraInput::UpdateSetting Failed to add/update metadata item: %{public}d",
                          itemEntry->item);
        }
    }
    return CAMERA_OK;
}

int32_t CameraInput::UnlockForControl()
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::UnlockForControl Need to call LockForControl() before UnlockForControl()");
        return CAMERA_INVALID_ARG;
    }

    UpdateSetting(changedMetadata_);
    changedMetadata_ = nullptr;
    return CAMERA_OK;
}

template<typename DataPtr, typename Vec, typename VecType>
void CameraInput::getVector(DataPtr data, size_t count, Vec &vect, VecType dataType)
{
    for (size_t index = 0; index < count; index++) {
        vect.emplace_back(static_cast<VecType>(data[index]));
    }
}

std::vector<camera_format_t> CameraInput::GetSupportedPhotoFormats()
{
    int32_t unitLen = 3;
    camera_format_t format;
    std::set<camera_format_t> formats;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % unitLen != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return {};
    }
    for (uint32_t index = 0; index < item.count; index += unitLen) {
        format = static_cast<camera_format_t>(item.data.i32[index]);
        if (format == OHOS_CAMERA_FORMAT_JPEG) {
            formats.insert(format);
        }
    }
    return std::vector<camera_format_t>(formats.begin(), formats.end());
}

std::vector<camera_format_t> CameraInput::GetSupportedVideoFormats()
{
    int32_t unitLen = 3;
    camera_format_t format;
    std::set<camera_format_t> formats;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % unitLen != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return {};
    }
    for (uint32_t index = 0; index < item.count; index += unitLen) {
        format = static_cast<camera_format_t>(item.data.i32[index]);
        if (format != OHOS_CAMERA_FORMAT_JPEG) {
            formats.insert(format);
        }
    }
    return std::vector<camera_format_t>(formats.begin(), formats.end());
}

std::vector<camera_format_t> CameraInput::GetSupportedPreviewFormats()
{
    int32_t unitLen = 3;
    camera_format_t format;
    std::set<camera_format_t> formats;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % unitLen != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return {};
    }
    for (uint32_t index = 0; index < item.count; index += unitLen) {
        format = static_cast<camera_format_t>(item.data.i32[index]);
        if (format != OHOS_CAMERA_FORMAT_JPEG) {
            formats.insert(format);
        }
    }
    return std::vector<camera_format_t>(formats.begin(), formats.end());
}

std::vector<CameraPicSize> CameraInput::getSupportedSizes(camera_format_t format)
{
    int32_t unitLen = 3;
    int32_t widthOffset = 1;
    int32_t heightOffset = 2;
    camera_metadata_item_t item;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % unitLen != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return {};
    }
    int32_t count = 0;
    for (uint32_t index = 0; index < item.count; index += unitLen) {
        if (item.data.i32[index] == format) {
            count++;
        }
    }
    if (count == 0) {
        MEDIA_ERR_LOG("Format: %{public}d is not found in stream configuration", format);
        return {};
    }

    std::vector<CameraPicSize> sizes(count);
    CameraPicSize *size = &sizes[0];
    for (uint32_t index = 0; index < item.count; index += unitLen) {
        if (item.data.i32[index] == format) {
            size->width = item.data.i32[index + widthOffset];
            size->height = item.data.i32[index + heightOffset];
            size++;
        }
    }
    return sizes;
}

std::vector<camera_ae_mode_t> CameraInput::GetSupportedExposureModes()
{
    std::vector<camera_ae_mode_t> supportedExposureModes;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_AVAILABLE_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedExposureModes Failed with return code %{public}d", ret);
        return supportedExposureModes;
    }
    getVector(item.data.u8, item.count, supportedExposureModes, camera_ae_mode_t(0));
    return supportedExposureModes;
}

void CameraInput::SetExposureMode(camera_ae_mode_t exposureMode)
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetExposureMode Need to call LockForControl() before setting camera properties");
        return;
    }
    bool status = false;
    uint32_t count = 1;
    uint8_t exposure = exposureMode;
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AE_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AE_MODE, &exposure, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AE_MODE, &exposure, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetExposureMode Failed to set exposure mode");
    }

    return;
}

camera_ae_mode_t CameraInput::GetExposureMode()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetExposureMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_AE_MODE_OFF;
    }
    return static_cast<camera_ae_mode_t>(item.data.u8[0]);
}

void CameraInput::SetExposureCallback(std::shared_ptr<ExposureCallback> exposureCallback)
{
    exposurecallback_ = exposureCallback;
    return;
}

std::vector<camera_af_mode_t> CameraInput::GetSupportedFocusModes()
{
    std::vector<camera_af_mode_t> supportedFocusModes;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AF_AVAILABLE_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedFocusModes Failed with return code %{public}d", ret);
        return supportedFocusModes;
    }
    getVector(item.data.u8, item.count, supportedFocusModes, camera_af_mode_t(0));
    return supportedFocusModes;
}

void CameraInput::SetFocusCallback(std::shared_ptr<FocusCallback> focusCallback)
{
    focusCallback_ = focusCallback;
    return;
}

int32_t CameraInput::StartFocus(camera_af_mode_t focusMode)
{
    bool status = false;
    int32_t ret;
    static int32_t triggerId = 0;
    uint32_t count = 1;
    uint8_t trigger = OHOS_CAMERA_AF_TRIGGER_START;
    camera_metadata_item_t item;

    if ((focusMode == OHOS_CAMERA_AF_MODE_OFF) || (focusMode == OHOS_CAMERA_AF_MODE_CONTINUOUS_VIDEO)
        || (focusMode == OHOS_CAMERA_AF_MODE_CONTINUOUS_PICTURE)) {
        return CAM_META_SUCCESS;
    }

    ret = FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AF_TRIGGER, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AF_TRIGGER, &trigger, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AF_TRIGGER, &trigger, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::StartFocus Failed to set trigger");
        return CAM_META_FAILURE;
    }

    triggerId++;
    ret = FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AF_TRIGGER_ID, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AF_TRIGGER_ID, &triggerId, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AF_TRIGGER_ID, &triggerId, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetFocusMode Failed to set trigger Id");
        return CAM_META_FAILURE;
    }
    return CAM_META_SUCCESS;
}

void CameraInput::SetFocusMode(camera_af_mode_t focusMode)
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetFocusMode Need to call LockForControl() before setting camera properties");
        return;
    }

    bool status = false;
    int32_t ret;
    uint32_t count = 1;
    uint8_t focus = focusMode;
    camera_metadata_item_t item;

    MEDIA_DEBUG_LOG("CameraInput::SetFocusMode Focus mode: %{public}d", focusMode);

#ifdef BALTIMORE_CAMERA
    ret = StartFocus(focusMode);
    if (ret != CAM_META_SUCCESS) {
        return;
    }
#endif

    ret = FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AF_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AF_MODE, &focus, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AF_MODE, &focus, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetFocusMode Failed to set focus mode");
    }
    return;
}

camera_af_mode_t CameraInput::GetFocusMode()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AF_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFocusMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_AF_MODE_OFF;
    }
    return static_cast<camera_af_mode_t>(item.data.u8[0]);
}

std::vector<float> CameraInput::GetSupportedZoomRatioRange()
{
    return cameraObj_->GetZoomRatioRange();
}

float CameraInput::GetZoomRatio()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetZoomRatio Failed with return code %{public}d", ret);
        return 0;
    }
    return static_cast<float>(item.data.f[0]);
}

int32_t CameraInput::SetCropRegion(float zoomRatio)
{
    bool status = false;
    int32_t ret;
    int32_t leftIndex = 0;
    int32_t topIndex = 1;
    int32_t rightIndex = 2;
    int32_t bottomIndex = 3;
    int32_t factor = 2;
    int32_t sensorRight;
    int32_t sensorBottom;
    const uint32_t arrayCount = 4;
    int32_t cropRegion[arrayCount] = {};
    camera_metadata_item_t item;

    if (zoomRatio == 0) {
        MEDIA_ERR_LOG("CameraInput::SetCropRegion Invaid zoom ratio");
        return CAM_META_FAILURE;
    }

    ret = FindCameraMetadataItem(cameraObj_->GetMetadata()->get(), OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::SetCropRegion Failed to get sensor active array size with return code %{public}d",
                      ret);
        return ret;
    }
    if (item.count != arrayCount) {
        MEDIA_ERR_LOG("CameraInput::SetCropRegion Invalid sensor active array size count: %{public}d", item.count);
        return CAM_META_FAILURE;
    }

    MEDIA_DEBUG_LOG("CameraInput::SetCropRegion Sensor active array left: %{public}d, top: %{public}d, "
                    "right: %{public}d, bottom: %{public}d", item.data.i32[leftIndex], item.data.i32[topIndex],
                    item.data.i32[rightIndex], item.data.i32[bottomIndex]);

    sensorRight = item.data.i32[rightIndex];
    sensorBottom = item.data.i32[bottomIndex];
    cropRegion[leftIndex] = (sensorRight - (sensorRight / zoomRatio)) / factor;
    cropRegion[topIndex] = (sensorBottom - (sensorBottom / zoomRatio)) / factor;
    cropRegion[rightIndex] = cropRegion[leftIndex] + (sensorRight / zoomRatio);
    cropRegion[bottomIndex] = cropRegion[topIndex] + (sensorBottom / zoomRatio);

    MEDIA_DEBUG_LOG("CameraInput::SetCropRegion Crop region left: %{public}d, top: %{public}d, "
                    "right: %{public}d, bottom: %{public}d", cropRegion[leftIndex], cropRegion[topIndex],
                    cropRegion[rightIndex], cropRegion[bottomIndex]);

    ret = FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_ZOOM_CROP_REGION, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_ZOOM_CROP_REGION, cropRegion, arrayCount);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_ZOOM_CROP_REGION, cropRegion, arrayCount);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetCropRegion Failed to set zoom crop region");
        return CAM_META_FAILURE;
    }
    return CAM_META_SUCCESS;
}

void CameraInput::SetZoomRatio(float zoomRatio)
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetZoomRatio Need to call LockForControl() before setting camera properties");
        return;
    }

    bool status = false;
    int32_t ret;
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    int32_t count = 1;
    camera_metadata_item_t item;

    MEDIA_DEBUG_LOG("CameraInput::SetZoomRatio Zoom ratio: %{public}f", zoomRatio);

    std::vector<float> zoomRange = cameraObj_->GetZoomRatioRange();
    if (zoomRange.empty()) {
        MEDIA_ERR_LOG("CameraInput::SetZoomRatio Zoom range is empty");
        return;
    }
    if (zoomRatio < zoomRange[minIndex]) {
        MEDIA_DEBUG_LOG("CameraInput::SetZoomRatio Zoom ratio: %{public}f is lesser than minumum zoom: %{public}f",
                        zoomRatio, zoomRange[minIndex]);
        zoomRatio = zoomRange[minIndex];
    } else if (zoomRatio > zoomRange[maxIndex]) {
        MEDIA_DEBUG_LOG("CameraInput::SetZoomRatio Zoom ratio: %{public}f is greater than maximum zoom: %{public}f",
                        zoomRatio, zoomRange[maxIndex]);
        zoomRatio = zoomRange[maxIndex];
    }

    if (zoomRatio == 0) {
        MEDIA_ERR_LOG("CameraInput::SetZoomRatio Invaid zoom ratio");
        return;
    }

#ifdef BALTIMORE_CAMERA
    ret = SetCropRegion(zoomRatio);
    if (ret != CAM_META_SUCCESS) {
        return;
    }
#endif

    ret = FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_ZOOM_RATIO, &zoomRatio, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_ZOOM_RATIO, &zoomRatio, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetZoomRatio Failed to set zoom mode");
    }
    return;
}

std::vector<camera_flash_mode_enum_t> CameraInput::GetSupportedFlashModes()
{
    std::vector<camera_flash_mode_enum_t> supportedFlashModes;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_DEVICE_AVAILABLE_FLASHMODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedFlashModes Failed with return code %{public}d", ret);
        return supportedFlashModes;
    }
    getVector(item.data.u8, item.count, supportedFlashModes, camera_flash_mode_enum_t(0));
    return supportedFlashModes;
}

camera_flash_mode_enum_t CameraInput::GetFlashMode()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FLASHMODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFlashMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_FLASH_MODE_CLOSE;
    }
    return static_cast<camera_flash_mode_enum_t>(item.data.u8[0]);
}

void CameraInput::SetFlashMode(camera_flash_mode_enum_t flashMode)
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetFlashMode Need to call LockForControl() before setting camera properties");
        return;
    }

    bool status = false;
    uint32_t count = 1;
    uint8_t flash = flashMode;
    camera_metadata_item_t item;
    int ret = FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_FLASHMODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_FLASHMODE, &flash, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_FLASHMODE, &flash, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetFlashMode Failed to set flash mode");
    }
    return;
}

void CameraInput::SetErrorCallback(std::shared_ptr<ErrorCallback> errorCallback)
{
    if (errorCallback == nullptr) {
        MEDIA_ERR_LOG("SetErrorCallback: Unregistering error callback");
    }
    errorCallback_ = errorCallback;
    return;
}

sptr<ICameraDeviceService> CameraInput::GetCameraDevice()
{
    return deviceObj_;
}

std::shared_ptr<ErrorCallback> CameraInput::GetErrorCallback()
{
    return errorCallback_;
}

void CameraInput::ProcessAutoFocusUpdates(const std::shared_ptr<CameraMetadata> &result)
{
    camera_metadata_item_t item;
    common_metadata_header_t *metadata = result->get();
    int ret = FindCameraMetadataItem(metadata, OHOS_CONTROL_AF_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_DEBUG_LOG("Focus mode: %{public}d", item.data.u8[0]);
    }
    ret = FindCameraMetadataItem(metadata, OHOS_CONTROL_AF_STATE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_INFO_LOG("Focus state: %{public}d", item.data.u8[0]);
        if (focusCallback_ != nullptr) {
            camera_af_state_t focusState = static_cast<camera_af_state_t>(item.data.u8[0]);
            auto itr = mapFromMetadataFocus_.find(focusState);
            if (itr != mapFromMetadataFocus_.end()) {
                focusCallback_->OnFocusState(itr->second);
            }
        }
    }
}

std::string CameraInput::GetCameraSettings()
{
    return MetadataUtils::EncodeToString(cameraObj_->GetMetadata());
}

int32_t CameraInput::SetCameraSettings(std::string setting)
{
    std::shared_ptr<CameraMetadata> metadata = MetadataUtils::DecodeFromString(setting);
    if (metadata == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetCameraSettings Failed to decode metadata setting from string");
        return CAMERA_INVALID_ARG;
    }
    return UpdateSetting(metadata);
}
} // CameraStandard
} // OHOS
