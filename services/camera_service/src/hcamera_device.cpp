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

#include "hcamera_device.h"

#include "camera_util.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
static bool isCameraOpened = false;
HCameraDevice::HCameraDevice(sptr<HCameraHostManager> &cameraHostManager,
                             sptr<CameraDeviceCallback> deviceCallback, std::string cameraID)
{
    cameraHostManager_ = cameraHostManager;
    deviceHDICallback_ = deviceCallback;
    cameraID_ = cameraID;
    streamOperator_ = nullptr;
    isReleaseCameraDevice_ = false;
    if (cameraHostManager == nullptr) {
        MEDIA_ERR_LOG("HCameraDevice::HCameraDevice cameraHostManager is null");
        return;
    }
}

HCameraDevice::~HCameraDevice()
{}

std::string HCameraDevice::GetCameraId()
{
    return cameraID_;
}

int32_t HCameraDevice::SetReleaseCameraDevice(bool isRelease)
{
    isReleaseCameraDevice_ = isRelease;
    return CAMERA_OK;
}

bool HCameraDevice::IsReleaseCameraDevice()
{
    return isReleaseCameraDevice_;
}

std::shared_ptr<Camera::CameraMetadata> HCameraDevice::GetSettings()
{
    int32_t errCode;
    std::shared_ptr<Camera::CameraMetadata> ability = nullptr;
    errCode = cameraHostManager_->GetCameraAbility(cameraID_, ability);
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("HCameraDevice::GetSettings Failed to get Camera Ability: %{public}d", errCode);
        return nullptr;
    }
    return ability;
}

int32_t HCameraDevice::Open()
{
    CAMERA_SYNC_TRACE;
    int32_t errorCode;
    std::lock_guard<std::mutex> lock(deviceLock_);
    if (isCameraOpened) {
        MEDIA_ERR_LOG("HCameraDevice::Open failed, camera is busy");
    }
    MEDIA_INFO_LOG("HCameraDevice::Open Opening camera device: %{public}s", cameraID_.c_str());
    errorCode = cameraHostManager_->OpenCameraDevice(cameraID_, deviceHDICallback_, hdiCameraDevice_);
    if (errorCode == CAMERA_OK) {
        isCameraOpened = true;
        if (updateSettings_ != nullptr) {
            Camera::CamRetCode rc = hdiCameraDevice_->UpdateSettings(updateSettings_);
            if (rc != Camera::NO_ERROR) {
                MEDIA_ERR_LOG("HCameraDevice::Open Update setting failed with error Code: %{public}d", rc);
                return HdiToServiceError(rc);
            }
            updateSettings_ = nullptr;
            MEDIA_DEBUG_LOG("HCameraDevice::Open Updated device settings");
        }
        errorCode = HdiToServiceError(hdiCameraDevice_->SetResultMode(Camera::ON_CHANGED));
    } else {
        MEDIA_ERR_LOG("HCameraDevice::Open Failed to open camera");
    }
    return errorCode;
}

int32_t HCameraDevice::Close()
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(deviceLock_);
    if (hdiCameraDevice_ != nullptr) {
        MEDIA_INFO_LOG("HCameraDevice::Close Closing camera device: %{public}s", cameraID_.c_str());
        hdiCameraDevice_->Close();
    }
    isCameraOpened = false;
    hdiCameraDevice_ = nullptr;
    return CAMERA_OK;
}

int32_t HCameraDevice::Release()
{
    if (hdiCameraDevice_ != nullptr) {
        Close();
    }
    if (deviceHDICallback_ != nullptr) {
        deviceHDICallback_->SetHCameraDevice(nullptr);
        deviceHDICallback_ = nullptr;
    }
    deviceSvcCallback_ = nullptr;
    return CAMERA_OK;
}

int32_t HCameraDevice::GetEnabledResults(std::vector<int32_t> &results)
{
    Camera::CamRetCode rc = hdiCameraDevice_->GetEnabledResults(results);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraDevice::GetEnabledResults failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraDevice::UpdateSetting(const std::shared_ptr<Camera::CameraMetadata> &settings)
{
    CAMERA_SYNC_TRACE;
    if (settings == nullptr) {
        MEDIA_ERR_LOG("HCameraDevice::UpdateSetting settings is null");
        return CAMERA_INVALID_ARG;
    }

    uint32_t count = Camera::GetCameraMetadataItemCount(settings->get());
    if (!count) {
        MEDIA_DEBUG_LOG("HCameraDevice::UpdateSetting Nothing to update");
        return CAMERA_OK;
    }
    if (updateSettings_) {
        camera_metadata_item_t metadataItem;
        for (uint32_t index = 0; index < count; index++) {
            int ret = Camera::GetCameraMetadataItem(settings->get(), index, &metadataItem);
            if (ret != CAM_META_SUCCESS) {
                MEDIA_ERR_LOG("HCameraDevice::UpdateSetting Failed to get metadata item at index: %{public}d", index);
                return CAMERA_INVALID_ARG;
            }
            bool status = false;
            uint32_t currentIndex;
            ret = Camera::FindCameraMetadataItemIndex(updateSettings_->get(), metadataItem.item, &currentIndex);
            if (ret == CAM_META_ITEM_NOT_FOUND) {
                status = updateSettings_->addEntry(metadataItem.item, metadataItem.data.u8, metadataItem.count);
            } else if (ret == CAM_META_SUCCESS) {
                status = updateSettings_->updateEntry(metadataItem.item, metadataItem.data.u8, metadataItem.count);
            }
            if (!status) {
                MEDIA_ERR_LOG("HCameraDevice::UpdateSetting Failed to update metadata item: %{public}d",
                              metadataItem.item);
                return CAMERA_UNKNOWN_ERROR;
            }
        }
    } else {
        updateSettings_ = settings;
    }
    if (hdiCameraDevice_ != nullptr) {
        Camera::CamRetCode rc = hdiCameraDevice_->UpdateSettings(updateSettings_);
        if (rc != Camera::NO_ERROR) {
            MEDIA_ERR_LOG("HCameraDevice::UpdateSetting failed with error Code: %{public}d", rc);
            return HdiToServiceError(rc);
        }
        updateSettings_ = nullptr;
    }
    MEDIA_DEBUG_LOG("HCameraDevice::UpdateSetting Updated device settings");
    return CAMERA_OK;
}

int32_t HCameraDevice::EnableResult(std::vector<int32_t> &results)
{
    if (results.empty()) {
        MEDIA_ERR_LOG("HCameraDevice::EnableResult results vector empty");
        return CAMERA_INVALID_ARG;
    }

    if (hdiCameraDevice_ == nullptr) {
        MEDIA_ERR_LOG("HCameraDevice::hdiCameraDevice_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }

    Camera::CamRetCode rc = hdiCameraDevice_->EnableResult(results);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraDevice::EnableResult failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }

    return CAMERA_OK;
}

int32_t HCameraDevice::DisableResult(std::vector<int32_t> &results)
{
    if (results.empty()) {
        MEDIA_ERR_LOG("HCameraDevice::DisableResult results vector empty");
        return CAMERA_INVALID_ARG;
    }

    if (hdiCameraDevice_ == nullptr) {
        MEDIA_ERR_LOG("HCameraDevice::hdiCameraDevice_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }

    Camera::CamRetCode rc = hdiCameraDevice_->DisableResult(results);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraDevice::DisableResult failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraDevice::SetCallback(sptr<ICameraDeviceServiceCallback> &callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("HCameraDevice::SetCallback callback is null");
        return CAMERA_INVALID_ARG;
    }
    deviceHDICallback_->SetHCameraDevice(this);
    deviceSvcCallback_ = callback;
    return CAMERA_OK;
}

int32_t HCameraDevice::GetStreamOperator(sptr<Camera::IStreamOperatorCallback> callback,
    sptr<Camera::IStreamOperator> &streamOperator)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("HCameraDevice::GetStreamOperator callback is null");
        return CAMERA_INVALID_ARG;
    }

    if (hdiCameraDevice_ == nullptr) {
        MEDIA_ERR_LOG("HCameraDevice::hdiCameraDevice_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }

    Camera::CamRetCode rc = hdiCameraDevice_->GetStreamOperator(callback, streamOperator);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraDevice::GetStreamOperator failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    streamOperator_ = streamOperator;
    return CAMERA_OK;
}

sptr<Camera::IStreamOperator> HCameraDevice::GetStreamOperator()
{
    return streamOperator_;
}

int32_t HCameraDevice::OnError(const Camera::ErrorType type, const int32_t errorMsg)
{
    if (deviceSvcCallback_ != nullptr) {
        if (type == Camera::REQUEST_TIMEOUT) {
            deviceSvcCallback_->OnError(CAMERA_DEVICE_REQUEST_TIMEOUT, errorMsg);
        } else if (type == Camera::DEVICE_PREEMPT) {
            deviceSvcCallback_->OnError(CAMERA_DEVICE_PREEMPTED, errorMsg);
        } else {
            deviceSvcCallback_->OnError(CAMERA_UNKNOWN_ERROR, errorMsg);
        }
    }
    return CAMERA_OK;
}

int32_t HCameraDevice::OnResult(const uint64_t timestamp,
                                const std::shared_ptr<Camera::CameraMetadata> &result)
{
    if (deviceSvcCallback_ != nullptr) {
        deviceSvcCallback_->OnResult(timestamp, result);
    }
    return CAMERA_OK;
}

CameraDeviceCallback::CameraDeviceCallback(sptr<HCameraDevice> hCameraDevice)
{
    hCameraDevice_ = hCameraDevice;
}

void CameraDeviceCallback::OnError(const Camera::ErrorType type, const int32_t errorMsg)
{
    if (hCameraDevice_ != nullptr) {
        hCameraDevice_->OnError(type, errorMsg);
    }
}

void CameraDeviceCallback::OnResult(const uint64_t timestamp,
                                    const std::shared_ptr<Camera::CameraMetadata> &result)
{
    if (hCameraDevice_ != nullptr) {
        hCameraDevice_->OnResult(timestamp, result);
    }
}

void CameraDeviceCallback::SetHCameraDevice(sptr<HCameraDevice> hCameraDevice)
{
    hCameraDevice_ = hCameraDevice;
}
} // namespace CameraStandard
} // namespace OHOS
