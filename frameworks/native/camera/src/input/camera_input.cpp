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

#include "input/camera_input.h"

#include <cinttypes>
#include <securec.h>
#include <set>
#include "camera_device_ability_items.h"
#include "camera_util.h"
#include "hcamera_device_callback_stub.h"
#include "ipc_skeleton.h"
#include "camera_log.h"
#include "metadata_utils.h"

namespace OHOS {
namespace CameraStandard {
namespace {
    constexpr int32_t DEFAULT_ITEMS = 10;
    constexpr int32_t DEFAULT_DATA_LENGTH = 100;
    constexpr uint32_t UNIT_LENGTH = 3;
}

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
            CAMERA_SYSEVENT_FAULT(CreateMsg("CameraDeviceServiceCallback::OnError() is called!, errorType: %d,"
                                            "errorMsg: %d", errorType, errorMsg));
            camInput_->GetErrorCallback()->OnError(errorType, errorMsg);
        } else {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::ErrorCallback not set!, Discarding callback");
        }
        return CAMERA_OK;
    }

    int32_t OnResult(const uint64_t timestamp, const std::shared_ptr<Camera::CameraMetadata> &result) override
    {
        MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() is called!, cameraId: %{public}s, timestamp: %{public}"
                       PRIu64, camInput_->GetCameraDeviceInfo()->GetID().c_str(), timestamp);
        camera_metadata_item_t item;
        int ret = Camera::FindCameraMetadataItem(result->get(), OHOS_CONTROL_FLASH_STATE, &item);
        if (ret == 0) {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() OHOS_CONTROL_FLASH_STATE is %{public}d",
                           item.data.u8[0]);
            CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("FlashStateChanged! current OHOS_CONTROL_FLASH_STATE is %d",
                                               item.data.u8[0]));
            POWERMGR_SYSEVENT_TORCH_STATE(IPCSkeleton::GetCallingPid(),
                                          IPCSkeleton::GetCallingUid(), item.data.u8[0]);
        }
        ret = Camera::FindCameraMetadataItem(result->get(), OHOS_CONTROL_FLASH_MODE, &item);
        if (ret == 0) {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() OHOS_CONTROL_FLASH_MODE is %{public}d",
                           item.data.u8[0]);
            CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("FlashModeChanged! current OHOS_CONTROL_FLASH_MODE is %d",
                                               item.data.u8[0]));
        }

        camInput_->ProcessAutoExposureUpdates(result);
        camInput_->ProcessAutoFocusUpdates(result);
        return CAMERA_OK;
    }
};

const std::unordered_map<camera_focus_state_t, FocusCallback::FocusState> CameraInput::mapFromMetadataFocus_ = {
    {OHOS_CAMERA_FOCUS_STATE_SCAN, FocusCallback::SCAN},
    {OHOS_CAMERA_FOCUS_STATE_FOCUSED, FocusCallback::FOCUSED},
    {OHOS_CAMERA_FOCUS_STATE_UNFOCUSED, FocusCallback::UNFOCUSED}
};

const std::unordered_map<camera_exposure_state_t,
        ExposureCallback::ExposureState> CameraInput::mapFromMetadataExposure_ = {
    {OHOS_CAMERA_EXPOSURE_STATE_SCAN, ExposureCallback::SCAN},
    {OHOS_CAMERA_EXPOSURE_STATE_CONVERGED, ExposureCallback::CONVERGED}
};

CameraInput::CameraInput(sptr<ICameraDeviceService> &deviceObj,
                         sptr<CameraInfo> &cameraObj) : cameraObj_(cameraObj), deviceObj_(deviceObj)
{
    CameraDeviceSvcCallback_ = new(std::nothrow) CameraDeviceServiceCallback(this);
    if (CameraDeviceSvcCallback_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::CameraInput CameraDeviceServiceCallback alloc failed");
        return;
    }
    deviceObj_->SetCallback(CameraDeviceSvcCallback_);
}

void CameraInput::Release()
{
    int32_t retCode = deviceObj_->Release();
    if (retCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release Camera Input, retCode: %{public}d", retCode);
    }
}

void CameraInput::LockForControl()
{
    mutex_.lock();
    changedMetadata_ = std::make_shared<Camera::CameraMetadata>(DEFAULT_ITEMS, DEFAULT_DATA_LENGTH);
}

int32_t CameraInput::UpdateSetting(std::shared_ptr<Camera::CameraMetadata> changedMetadata)
{
    CAMERA_SYNC_TRACE;
    if (!Camera::GetCameraMetadataItemCount(changedMetadata->get())) {
        MEDIA_INFO_LOG("CameraInput::UpdateSetting No configuration to update");
        return CAMERA_OK;
    }

    int32_t ret = deviceObj_->UpdateSetting(changedMetadata);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("CameraInput::UpdateSetting Failed to update settings");
        return ret;
    }

    size_t length;
    uint32_t count = changedMetadata->get()->item_count;
    uint8_t *data = Camera::GetMetadataData(changedMetadata->get());
    camera_metadata_item_entry_t *itemEntry = Camera::GetMetadataItems(changedMetadata->get());
    std::shared_ptr<Camera::CameraMetadata> baseMetadata = cameraObj_->GetMetadata();
    for (uint32_t i = 0; i < count; i++, itemEntry++) {
        bool status = false;
        camera_metadata_item_t item;
        length = Camera::CalculateCameraMetadataItemDataSize(itemEntry->data_type, itemEntry->count);
        ret = Camera::FindCameraMetadataItem(baseMetadata->get(), itemEntry->item, &item);
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
    mutex_.unlock();
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
    camera_format_t format;
    std::set<camera_format_t> formats;
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(),
                                             OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % UNIT_LENGTH != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return {};
    }
    for (uint32_t index = 0; index < item.count; index += UNIT_LENGTH) {
        format = static_cast<camera_format_t>(item.data.i32[index]);
        if (format == OHOS_CAMERA_FORMAT_JPEG) {
            formats.insert(format);
        }
    }
    return std::vector<camera_format_t>(formats.begin(), formats.end());
}

std::vector<camera_format_t> CameraInput::GetSupportedVideoFormats()
{
    camera_format_t format;
    std::set<camera_format_t> formats;
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(),
                                             OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % UNIT_LENGTH != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return {};
    }
    for (uint32_t index = 0; index < item.count; index += UNIT_LENGTH) {
        format = static_cast<camera_format_t>(item.data.i32[index]);
        if (format != OHOS_CAMERA_FORMAT_JPEG) {
            formats.insert(format);
        }
    }
    return std::vector<camera_format_t>(formats.begin(), formats.end());
}

std::vector<camera_format_t> CameraInput::GetSupportedPreviewFormats()
{
    camera_format_t format;
    std::set<camera_format_t> formats;
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(),
                                             OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % UNIT_LENGTH != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}d", item.count);
        return {};
    }
    for (uint32_t index = 0; index < item.count; index += UNIT_LENGTH) {
        format = static_cast<camera_format_t>(item.data.i32[index]);
        if (format != OHOS_CAMERA_FORMAT_JPEG) {
            formats.insert(format);
        }
    }
    return std::vector<camera_format_t>(formats.begin(), formats.end());
}

std::vector<CameraPicSize> CameraInput::getSupportedSizes(camera_format_t format)
{
    uint32_t widthOffset = 1;
    uint32_t heightOffset = 2;
    camera_metadata_item_t item;

    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    int ret = Camera::FindCameraMetadataItem(metadata->get(),
                                             OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get stream configuration with return code %{public}d", ret);
        return {};
    }
    if (item.count % UNIT_LENGTH != 0) {
        MEDIA_ERR_LOG("Invalid stream configuration count: %{public}u", item.count);
        return {};
    }
    int32_t count = 0;
    for (uint32_t index_ = 0; index_ < item.count; index_ += UNIT_LENGTH) {
        if (item.data.i32[index_] == format) {
            count++;
        }
    }
    if (count == 0) {
        MEDIA_ERR_LOG("Format: %{public}d is not found in stream configuration", format);
        return {};
    }

    std::vector<CameraPicSize> sizes(count);
    CameraPicSize *size = &sizes[0];
    for (uint32_t index = 0; index < item.count; index += UNIT_LENGTH) {
        if (item.data.i32[index] == format) {
            size->width = static_cast<uint32_t>(item.data.i32[index + widthOffset]);
            size->height = static_cast<uint32_t>(item.data.i32[index + heightOffset]);
            size++;
        }
    }
    return sizes;
}

bool CameraInput::IsExposureModeSupported(camera_exposure_mode_enum_t exposureMode)
{
    std::vector<camera_exposure_mode_enum_t> vecSupportedExposureModeList;
    vecSupportedExposureModeList = this->GetSupportedExposureModes();
    if (find(vecSupportedExposureModeList.begin(), vecSupportedExposureModeList.end(),
        exposureMode) != vecSupportedExposureModeList.end()) {
        return true;
    }

    return false;
}

std::vector<camera_exposure_mode_enum_t> CameraInput::GetSupportedExposureModes()
{
    std::vector<camera_exposure_mode_enum_t> supportedExposureModes;
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_EXPOSURE_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedExposureModes Failed with return code %{public}d", ret);
        return supportedExposureModes;
    }
    getVector(item.data.u8, item.count, supportedExposureModes, camera_exposure_mode_enum_t(0));
    return supportedExposureModes;
}

void CameraInput::SetExposureMode(camera_exposure_mode_enum_t exposureMode)
{
    CAMERA_SYNC_TRACE;
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetExposureMode Need to call LockForControl() before setting camera properties");
        return;
    }
    bool status = false;
    uint32_t count = 1;
    uint8_t exposure = exposureMode;
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_EXPOSURE_MODE, &exposure, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_EXPOSURE_MODE, &exposure, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetExposureMode Failed to set exposure mode");
    }

    return;
}

camera_exposure_mode_enum_t CameraInput::GetExposureMode()
{
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetExposureMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_EXPOSURE_MODE_MANUAL;
    }
    return static_cast<camera_exposure_mode_enum_t>(item.data.u8[0]);
}

void CameraInput::SetExposurePoint(Point exposurePoint)
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetExposurePoint Need to call LockForControl() before setting camera properties");
        return;
    }
    bool status = false;
    float exposureArea[2] = {exposurePoint.x, exposurePoint.y};
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AE_REGIONS, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AE_REGIONS, exposureArea,
            sizeof(exposureArea) / sizeof(exposureArea[0]));
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AE_REGIONS, exposureArea,
            sizeof(exposureArea) / sizeof(exposureArea[0]));
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetExposurePoint Failed to set exposure Area");
    }
}


Point CameraInput::GetExposurePoint()
{
    Point exposurePoint = {0, 0};
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_REGIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetExposurePoint Failed with return code %{public}d", ret);
        return exposurePoint;
    }
    exposurePoint.x = item.data.f[0];
    exposurePoint.y = item.data.f[1];

    return exposurePoint;
}


std::vector<int32_t> CameraInput::GetExposureBiasRange()
{
    return cameraObj_->GetExposureBiasRange();
}


void CameraInput::SetExposureBias(int32_t exposureValue)
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetExposureValue Need to call LockForControl() before setting camera properties");
        return;
    }

    bool status = false;
    int32_t ret;
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    int32_t count = 1;
    camera_metadata_item_t item;

    MEDIA_DEBUG_LOG("CameraInput::SetExposureValue exposure compensation: %{public}d", exposureValue);

    std::vector<int32_t> biasRange = cameraObj_->GetExposureBiasRange();
    if (biasRange.empty()) {
        MEDIA_ERR_LOG("CameraInput::SetExposureValue Bias range is empty");
        return;
    }
    if (exposureValue < biasRange[minIndex]) {
        MEDIA_DEBUG_LOG("CameraInput::SetExposureValue bias value:"
                        "%{public}d is lesser than minimum bias: %{public}d",
                        exposureValue, biasRange[minIndex]);
        exposureValue = biasRange[minIndex];
    } else if (exposureValue > biasRange[maxIndex]) {
        MEDIA_DEBUG_LOG("CameraInput::SetExposureValue bias value: %{public}d is greater than maximum bias: %{public}d",
                        exposureValue, biasRange[maxIndex]);
        exposureValue = biasRange[maxIndex];
    }

    if (exposureValue == 0) {
        MEDIA_ERR_LOG("CameraInput::SetExposureValue Invalid exposure compensation value");
        return;
    }

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &exposureValue, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &exposureValue, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetExposureValue Failed to set exposure compensation");
    }
    return;
}

int32_t CameraInput::GetExposureValue()
{
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetExposureValue Failed with return code %{public}d", ret);
        return 0;
    }
    return static_cast<int32_t>(item.data.i32[0]);
}

void CameraInput::SetExposureCallback(std::shared_ptr<ExposureCallback> exposureCallback)
{
    exposureCallback_ = exposureCallback;
}

std::vector<camera_focus_mode_enum_t> CameraInput::GetSupportedFocusModes()
{
    std::vector<camera_focus_mode_enum_t> supportedFocusModes;
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FOCUS_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedFocusModes Failed with return code %{public}d", ret);
        return supportedFocusModes;
    }
    getVector(item.data.u8, item.count, supportedFocusModes, camera_focus_mode_enum_t(0));
    return supportedFocusModes;
}

void CameraInput::SetFocusCallback(std::shared_ptr<FocusCallback> focusCallback)
{
    focusCallback_ = focusCallback;
    return;
}

bool CameraInput::IsFocusModeSupported(camera_focus_mode_enum_t focusMode)
{
    std::vector<camera_focus_mode_enum_t> vecSupportedFocusModeList;
    vecSupportedFocusModeList = this->GetSupportedFocusModes();
    if (find(vecSupportedFocusModeList.begin(), vecSupportedFocusModeList.end(),
        focusMode) != vecSupportedFocusModeList.end()) {
        return true;
    }

    return false;
}

int32_t CameraInput::StartFocus(camera_focus_mode_enum_t focusMode)
{
    bool status = false;
    int32_t ret;
    static int32_t triggerId = 0;
    uint32_t count = 1;
    uint8_t trigger = OHOS_CAMERA_AF_TRIGGER_START;
    camera_metadata_item_t item;

    // Todo: recheck this condition
    if (focusMode == OHOS_CAMERA_FOCUS_MODE_MANUAL) {
        return CAM_META_SUCCESS;
    }

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AF_TRIGGER, &item);
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
    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AF_TRIGGER_ID, &item);
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

void CameraInput::SetFocusMode(camera_focus_mode_enum_t focusMode)
{
    CAMERA_SYNC_TRACE;
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

#ifdef PRODUCT_M40
    ret = StartFocus(focusMode);
    if (ret != CAM_META_SUCCESS) {
        return;
    }
#endif

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_FOCUS_MODE, &focus, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_FOCUS_MODE, &focus, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetFocusMode Failed to set focus mode");
    }
}

camera_focus_mode_enum_t CameraInput::GetFocusMode()
{
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFocusMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_FOCUS_MODE_MANUAL;
    }
    return static_cast<camera_focus_mode_enum_t>(item.data.u8[0]);
}

void CameraInput::SetFocusPoint(Point focusPoint)
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetFocusPoint Need to call LockForControl() before setting camera properties");
        return;
    }
    bool status = false;
    float FocusArea[2] = {focusPoint.x, focusPoint.y};
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AF_REGIONS, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AF_REGIONS, FocusArea,
            sizeof(FocusArea) / sizeof(FocusArea[0]));
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AF_REGIONS, FocusArea,
            sizeof(FocusArea) / sizeof(FocusArea[0]));
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetFocusPoint Failed to set Focus Area");
    }
}

Point CameraInput::GetFocusPoint()
{
    Point focusPoint = {0, 0};
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AF_REGIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFocusPoint Failed with return code %{public}d", ret);
        return focusPoint;
    }
    focusPoint.x = item.data.f[0];
    focusPoint.y = item.data.f[1];

    return focusPoint;
}

float CameraInput::GetFocalLength()
{
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FOCAL_LENGTH, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFocalLength Failed with return code %{public}d", ret);
        return 0;
    }
    return static_cast<float>(item.data.f[0]);
}

std::vector<float> CameraInput::GetSupportedZoomRatioRange()
{
    return cameraObj_->GetZoomRatioRange();
}

float CameraInput::GetZoomRatio()
{
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
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
        MEDIA_ERR_LOG("CameraInput::SetCropRegion Invalid zoom ratio");
        return CAM_META_FAILURE;
    }

    ret = Camera::FindCameraMetadataItem(cameraObj_->GetMetadata()->get(), OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::SetCropRegion Failed to get sensor active array size with return code %{public}d",
                      ret);
        return ret;
    }
    if (item.count != arrayCount) {
        MEDIA_ERR_LOG("CameraInput::SetCropRegion Invalid sensor active array size count: %{public}u", item.count);
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

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_ZOOM_CROP_REGION, &item);
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
    CAMERA_SYNC_TRACE;
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
        MEDIA_DEBUG_LOG("CameraInput::SetZoomRatio Zoom ratio: %{public}f is lesser than minimum zoom: %{public}f",
                        zoomRatio, zoomRange[minIndex]);
        zoomRatio = zoomRange[minIndex];
    } else if (zoomRatio > zoomRange[maxIndex]) {
        MEDIA_DEBUG_LOG("CameraInput::SetZoomRatio Zoom ratio: %{public}f is greater than maximum zoom: %{public}f",
                        zoomRatio, zoomRange[maxIndex]);
        zoomRatio = zoomRange[maxIndex];
    }

    if (zoomRatio == 0) {
        MEDIA_ERR_LOG("CameraInput::SetZoomRatio Invalid zoom ratio");
        return;
    }

#ifdef PRODUCT_M40
    ret = SetCropRegion(zoomRatio);
    if (ret != CAM_META_SUCCESS) {
        return;
    }
#endif

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
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
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FLASH_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedFlashModes Failed with return code %{public}d", ret);
        return supportedFlashModes;
    }
    getVector(item.data.u8, item.count, supportedFlashModes, camera_flash_mode_enum_t(0));
    return supportedFlashModes;
}

camera_flash_mode_enum_t CameraInput::GetFlashMode()
{
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FLASH_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFlashMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_FLASH_MODE_CLOSE;
    }
    return static_cast<camera_flash_mode_enum_t>(item.data.u8[0]);
}

void CameraInput::SetFlashMode(camera_flash_mode_enum_t flashMode)
{
    CAMERA_SYNC_TRACE;
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetFlashMode Need to call LockForControl() before setting camera properties");
        return;
    }

    bool status = false;
    uint32_t count = 1;
    uint8_t flash = flashMode;
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_FLASH_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_FLASH_MODE, &flash, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_FLASH_MODE, &flash, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CameraInput::SetFlashMode Failed to set flash mode");
        return;
    }

    if (flashMode == OHOS_CAMERA_FLASH_MODE_CLOSE) {
        POWERMGR_SYSEVENT_FLASH_OFF();
    } else {
        POWERMGR_SYSEVENT_FLASH_ON();
    }
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

void CameraInput::ProcessAutoFocusUpdates(const std::shared_ptr<Camera::CameraMetadata> &result)
{
    camera_metadata_item_t item;
    common_metadata_header_t *metadata = result->get();
    int ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_DEBUG_LOG("Focus mode: %{public}d", item.data.u8[0]);
        CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("FocusModeChanged! current OHOS_CONTROL_FOCUS_MODE is %d",
                                           item.data.u8[0]));
    }
    ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_FOCUS_STATE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_INFO_LOG("Focus state: %{public}d", item.data.u8[0]);
        CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("FocusStateChanged! current OHOS_CONTROL_FOCUS_STATE is %d",
                                           item.data.u8[0]));
        if (focusCallback_ != nullptr) {
            camera_focus_state_t focusState = static_cast<camera_focus_state_t>(item.data.u8[0]);
            auto itr = mapFromMetadataFocus_.find(focusState);
            if (itr != mapFromMetadataFocus_.end()) {
                focusCallback_->OnFocusState(itr->second);
            }
        }
    }
}

void CameraInput::ProcessAutoExposureUpdates(const std::shared_ptr<Camera::CameraMetadata> &result)
{
    camera_metadata_item_t item;
    common_metadata_header_t *metadata = result->get();

    int ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_DEBUG_LOG("exposure mode: %{public}d", item.data.u8[0]);
    }

    ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_EXPOSURE_STATE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_INFO_LOG("Exposure state: %{public}d", item.data.u8[0]);
        if (exposureCallback_ != nullptr) {
            camera_exposure_state_t exposureState = static_cast<camera_exposure_state_t>(item.data.u8[0]);
            auto itr = mapFromMetadataExposure_.find(exposureState);
            if (itr != mapFromMetadataExposure_.end()) {
                exposureCallback_->OnExposureState(itr->second);
            }
        }
    }
}

std::string CameraInput::GetCameraSettings()
{
    return Camera::MetadataUtils::EncodeToString(cameraObj_->GetMetadata());
}

int32_t CameraInput::SetCameraSettings(std::string setting)
{
    std::shared_ptr<Camera::CameraMetadata> metadata = Camera::MetadataUtils::DecodeFromString(setting);
    if (metadata == nullptr) {
        MEDIA_ERR_LOG("CameraInput::SetCameraSettings Failed to decode metadata setting from string");
        return CAMERA_INVALID_ARG;
    }
    return UpdateSetting(metadata);
}

sptr<CameraInfo> CameraInput::GetCameraDeviceInfo()
{
    return cameraObj_;
}

void SetVideoStabilizingMode(sptr<CameraInput> device, CameraVideoStabilizationMode VideoStabilizationMode)
{
    if (!device) {
    MEDIA_ERR_LOG("CameraInput::SetVideoStabilizingMode Need call LockForControl() before setting camera properties");
    return;
    }
    uint32_t count = 1;
    uint8_t StabilizationMode = VideoStabilizationMode;

    device->LockForControl();
    MEDIA_DEBUG_LOG("CameraInput::SetVideoStabilizingMode StabilizationMode : %{public}d", StabilizationMode);
    if (!device->changedMetadata_->addEntry(OHOS_CONTROL_VIDEO_STABILIZATION_MODE, &StabilizationMode, count)) {
        MEDIA_DEBUG_LOG("CameraInput::SetVideoStabilizingMode Failed to set video stabilization mode");
    }

    device->UnlockForControl();
}

void SetRecordingFrameRateRange(sptr<CameraInput> device, int32_t minFpsVal, int32_t maxFpsVal)
{
    int32_t frameRateRange[2] = {minFpsVal, maxFpsVal};

    device->LockForControl();

    if (!device->changedMetadata_->addEntry(OHOS_CONTROL_FPS_RANGES,
                                            &frameRateRange,
                                            sizeof(frameRateRange) / sizeof(frameRateRange[0]))) {
        MEDIA_ERR_LOG("SetRecordingFrameRateRange failed");
    }

    device->UnlockForControl();
}
} // CameraStandard
} // OHOS
