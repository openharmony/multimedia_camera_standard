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

#include "hcamera_host_manager.h"

#include "camera_host_callback_stub.h"
#include "camera_util.h"
#include "hdf_io_service_if.h"
#include "iservmgr_hdi.h"
#include "media_log.h"

namespace OHOS {
namespace CameraStandard {
struct HCameraHostManager::CameraDeviceInfo {
    std::string cameraId;
    std::shared_ptr<Camera::CameraMetadata> ability;
    std::mutex mutex;

    explicit CameraDeviceInfo(const std::string& cameraId, sptr<Camera::ICameraDevice> device = nullptr)
        : cameraId(cameraId), ability(nullptr)
    {
    }

    ~CameraDeviceInfo() = default;
};

class HCameraHostManager::CameraHostInfo : public Camera::CameraHostCallbackStub {
public:
    explicit CameraHostInfo(HCameraHostManager* cameraHostManager, std::string name);
    ~CameraHostInfo();
    bool Init();
    bool IsCameraSupported(const std::string& cameraId);
    const std::string& GetName();
    int32_t GetCameras(std::vector<std::string>& cameraIds);
    int32_t GetCameraAbility(std::string& cameraId, std::shared_ptr<Camera::CameraMetadata>& ability);
    int32_t OpenCamera(std::string& cameraId, const sptr<Camera::ICameraDeviceCallback>& callback,
                       sptr<Camera::ICameraDevice>& pDevice);
    int32_t SetFlashlight(const std::string& cameraId, bool isEnable);

    // CameraHostCallbackStub
    void OnCameraStatus(const std::string& cameraId, Camera::CameraStatus status) override;
    void OnFlashlightStatus(const std::string& cameraId, Camera::FlashlightStatus status) override;
    void OnCameraEvent(const std::string &cameraId, Camera::CameraEvent event) override;

private:
    std::shared_ptr<CameraDeviceInfo> FindCameraDeviceInfo(const std::string& cameraId);
    void AddDevice(const std::string& cameraId);
    void RemoveDevice(const std::string& cameraId);

    HCameraHostManager* cameraHostManager_;
    std::string name_;
    sptr<Camera::ICameraHost> cameraHostProxy_;

    std::mutex mutex_;
    std::vector<std::string> cameraIds_;
    std::vector<std::shared_ptr<CameraDeviceInfo>> devices_;
};

HCameraHostManager::CameraHostInfo::CameraHostInfo(HCameraHostManager* cameraHostManager, std::string name)
    : cameraHostManager_(cameraHostManager), name_(std::move(name)), cameraHostProxy_(nullptr)
{
}

HCameraHostManager::CameraHostInfo::~CameraHostInfo()
{
    MEDIA_INFO_LOG("CameraHostInfo ~CameraHostInfo");
}

bool HCameraHostManager::CameraHostInfo::Init()
{
    if (cameraHostProxy_ != nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::Init, no camera host proxy");
        return true;
    }
    cameraHostProxy_ = Camera::ICameraHost::Get(name_.c_str());
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("Failed to get ICameraHost");
        return false;
    }
    cameraHostProxy_->SetCallback(this);
    std::lock_guard<std::mutex> lock(mutex_);
    Camera::CamRetCode ret = cameraHostProxy_->GetCameraIds(cameraIds_);
    if (ret != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("Init, GetCameraIds failed, ret = %{public}d", ret);
        return false;
    }
    for (const auto& cameraId : cameraIds_) {
        devices_.push_back(std::make_shared<HCameraHostManager::CameraDeviceInfo>(cameraId));
    }
    return true;
}

bool HCameraHostManager::CameraHostInfo::IsCameraSupported(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(cameraIds_.begin(), cameraIds_.end(),
                       [&cameraId](const auto& camId) { return camId == cameraId; });
}

const std::string& HCameraHostManager::CameraHostInfo::GetName()
{
    return name_;
}

int32_t HCameraHostManager::CameraHostInfo::GetCameras(std::vector<std::string>& cameraIds)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cameraIds.insert(cameraIds.end(), cameraIds_.begin(), cameraIds_.end());
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::GetCameraAbility(std::string& cameraId,
    std::shared_ptr<Camera::CameraMetadata>& ability)
{
    auto deviceInfo = FindCameraDeviceInfo(cameraId);
    if (deviceInfo == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility deviceInfo is null");
        return CAMERA_UNKNOWN_ERROR;
    }

    if (deviceInfo->ability) {
        ability = deviceInfo->ability;
    } else {
        std::lock_guard<std::mutex> lock(deviceInfo->mutex);
        if (cameraHostProxy_ == nullptr) {
            MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility cameraHostProxy_ is null");
            return CAMERA_UNKNOWN_ERROR;
        }
        if (!deviceInfo->ability) {
            Camera::CamRetCode rc = cameraHostProxy_->GetCameraAbility(cameraId, ability);
            if (rc != Camera::NO_ERROR) {
                MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility failed with error Code:%{public}d", rc);
                return HdiToServiceError(rc);
            }
            deviceInfo->ability = ability;
        }
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::OpenCamera(std::string& cameraId,
    const sptr<Camera::ICameraDeviceCallback>& callback,
    sptr<Camera::ICameraDevice>& pDevice)
{
    MEDIA_INFO_LOG("CameraHostInfo::OpenCamera %{public}s", cameraId.c_str());
    auto deviceInfo = FindCameraDeviceInfo(cameraId);
    if (deviceInfo == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility deviceInfo is null");
        return CAMERA_UNKNOWN_ERROR;
    }

    std::lock_guard<std::mutex> lock(deviceInfo->mutex);
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::OpenCamera cameraHostProxy_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }
    Camera::CamRetCode rc = cameraHostProxy_->OpenCamera(cameraId, callback, pDevice);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("CameraHostInfo::OpenCamera failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::SetFlashlight(const std::string& cameraId, bool isEnable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::SetFlashlight cameraHostProxy_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }
    Camera::CamRetCode rc = cameraHostProxy_->SetFlashlight(cameraId, isEnable);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("CameraHostInfo::SetFlashlight failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

void HCameraHostManager::CameraHostInfo::OnCameraStatus(const std::string& cameraId, Camera::CameraStatus status)
{
    if ((cameraHostManager_ == nullptr) || (cameraHostManager_->statusCallback_ == nullptr)) {
        MEDIA_WARNING_LOG("CameraHostInfo::OnCameraStatus for %{public}s with status %{public}d "
                          "failed due to no callback",
                          cameraId.c_str(), status);
        return;
    }
    CameraStatus svcStatus = CAMERA_STATUS_UNAVAILABLE;
    switch (status) {
        case Camera::UN_AVAILABLE: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraStatus, camera %{public}s unavailable", cameraId.c_str());
            svcStatus = CAMERA_STATUS_UNAVAILABLE;
            break;
        }
        case Camera::AVAILABLE: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraStatus, camera %{public}s available", cameraId.c_str());
            svcStatus = CAMERA_STATUS_AVAILABLE;
            AddDevice(cameraId);
            break;
        }
        default:
            MEDIA_ERR_LOG("Unknown camera status: %{public}d", status);
            return;
    }
    cameraHostManager_->statusCallback_->OnCameraStatus(cameraId, svcStatus);
}

void HCameraHostManager::CameraHostInfo::OnFlashlightStatus(const std::string& cameraId,
    Camera::FlashlightStatus status)
{
    if ((cameraHostManager_ == nullptr) || (cameraHostManager_->statusCallback_ == nullptr)) {
        MEDIA_WARNING_LOG("CameraHostInfo::OnFlashlightStatus for %{public}s with status %{public}d "
                          "failed due to no callback or cameraHostManager_ is null",
                          cameraId.c_str(), status);
        return;
    }
    FlashStatus flashStatus = FLASH_STATUS_OFF;
    switch (status) {
        case Camera::FLASHLIGHT_OFF:
            flashStatus = FLASH_STATUS_OFF;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatus, camera %{public}s flash light is off",
                           cameraId.c_str());
            break;

        case Camera::FLASHLIGHT_ON:
            flashStatus = FLASH_STATUS_ON;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatus, camera %{public}s flash light is on",
                           cameraId.c_str());
            break;

        case Camera::FLASHLIGHT_UNAVAILABLE:
            flashStatus = FLASH_STATUS_UNAVAILABLE;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatus, camera %{public}s flash light is unavailable",
                           cameraId.c_str());
            break;

        default:
            MEDIA_ERR_LOG("Unknown flashlight status: %{public}d for camera %{public}s", status, cameraId.c_str());
            return;
    }
    cameraHostManager_->statusCallback_->OnFlashlightStatus(cameraId, flashStatus);
}

void HCameraHostManager::CameraHostInfo::OnCameraEvent(const std::string &cameraId, Camera::CameraEvent event)
{
    if ((cameraHostManager_ == nullptr) || (cameraHostManager_->statusCallback_ == nullptr)) {
        MEDIA_WARNING_LOG("CameraHostInfo::OnCameraEvent for %{public}s with status %{public}d "
                          "failed due to no callback or cameraHostManager_ is null",
                          cameraId.c_str(), event);
        return;
    }
    CameraStatus svcStatus = CAMERA_STATUS_UNAVAILABLE;
    switch (event) {
        case Camera::CAMERA_EVENT_DEVICE_RMV: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraEvent, camera %{public}s unavailable", cameraId.c_str());
            svcStatus = CAMERA_STATUS_UNAVAILABLE;
            RemoveDevice(cameraId);
            break;
        }
        case Camera::CAMERA_EVENT_DEVICE_ADD: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraEvent camera %{public}s available", cameraId.c_str());
            svcStatus = CAMERA_STATUS_AVAILABLE;
            AddDevice(cameraId);
            break;
        }
        default:
            MEDIA_ERR_LOG("Unknown camera event: %{public}d", event);
            return;
    }
    cameraHostManager_->statusCallback_->OnCameraStatus(cameraId, svcStatus);
}

std::shared_ptr<HCameraHostManager::CameraDeviceInfo> HCameraHostManager::CameraHostInfo::FindCameraDeviceInfo
   (const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& deviceInfo : devices_) {
        if (deviceInfo->cameraId == cameraId) {
            MEDIA_INFO_LOG("CameraHostInfo::FindCameraDeviceInfo succeed for %{public}s", cameraId.c_str());
            return deviceInfo;
        }
    }
    MEDIA_WARNING_LOG("CameraHostInfo::FindCameraDeviceInfo failed for %{public}s", cameraId.c_str());
    return nullptr;
}

void HCameraHostManager::CameraHostInfo::AddDevice(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cameraIds_.push_back(cameraId);
    if (std::none_of(devices_.begin(), devices_.end(),
                     [&cameraId](auto& devInfo) { return devInfo->cameraId == cameraId; })) {
        devices_.push_back(std::make_shared<HCameraHostManager::CameraDeviceInfo>(cameraId));
        MEDIA_INFO_LOG("CameraHostInfo::AddDevice, camera %{public}s added", cameraId.c_str());
    } else {
        MEDIA_WARNING_LOG("CameraHostInfo::AddDevice, camera %{public}s already exists", cameraId.c_str());
    }
}

void HCameraHostManager::CameraHostInfo::RemoveDevice(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cameraIds_.erase(std::remove(cameraIds_.begin(), cameraIds_.end(), cameraId), cameraIds_.end());
    devices_.erase(std::remove_if(devices_.begin(), devices_.end(),
        [&cameraId](const auto& devInfo) { return devInfo->cameraId == cameraId; }),
        devices_.end());
}

HCameraHostManager::HCameraHostManager(StatusCallback* statusCallback)
    : statusCallback_(statusCallback), cameraHostInfos_()
{
}

HCameraHostManager::~HCameraHostManager()
{
    statusCallback_ = nullptr;
}

int32_t HCameraHostManager::Init()
{
    MEDIA_INFO_LOG("HCameraHostManager::Init");
    using namespace OHOS::HDI::ServiceManager::V1_0;
    auto svcMgr = IServiceManager::Get();
    if (svcMgr == nullptr) {
        MEDIA_ERR_LOG("%s: IServiceManager failed!", __func__);
        return CAMERA_UNKNOWN_ERROR;
    }
    auto rt = svcMgr->RegisterServiceStatusListener(this, DEVICE_CLASS_CAMERA);
    if (rt != 0) {
        MEDIA_ERR_LOG("%s: RegisterServiceStatusListener failed!", __func__);
    }
    return rt == 0 ? CAMERA_OK : CAMERA_UNKNOWN_ERROR;
}

void HCameraHostManager::DeInit()
{
    using namespace OHOS::HDI::ServiceManager::V1_0;
    auto svcMgr = IServiceManager::Get();
    if (svcMgr == nullptr) {
        MEDIA_ERR_LOG("%s: IServiceManager failed", __func__);
        return;
    }
    auto rt = svcMgr->UnregisterServiceStatusListener(this);
    if (rt != 0) {
        MEDIA_ERR_LOG("%s: UnregisterServiceStatusListener failed!", __func__);
    }
}

int32_t HCameraHostManager::GetCameras(std::vector<std::string>& cameraIds)
{
    MEDIA_INFO_LOG("HCameraHostManager::GetCameras");
    if (cameraHostInfos_.size() == 0) {
        MEDIA_INFO_LOG("HCameraHostManager::GetCameras host info is empty, start add host");
        AddCameraHost("camera_service");
        AddCameraHost("distributed_camera_service");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    cameraIds.clear();
    for (const auto& cameraHost : cameraHostInfos_) {
        cameraHost->GetCameras(cameraIds);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::GetCameraAbility(std::string &cameraId,
                                             std::shared_ptr<Camera::CameraMetadata> &ability)
{
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with invalid device info.");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->GetCameraAbility(cameraId, ability);
}

int32_t HCameraHostManager::OpenCameraDevice(std::string &cameraId,
                                             const sptr<Camera::ICameraDeviceCallback> &callback,
                                             sptr<Camera::ICameraDevice> &pDevice)
{
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with invalid device info");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->OpenCamera(cameraId, callback, pDevice);
}

int32_t HCameraHostManager::SetFlashlight(const std::string& cameraId, bool isEnable)
{
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with invalid device info");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->SetFlashlight(cameraId, isEnable);
}

void HCameraHostManager::OnReceive(const HDI::ServiceManager::V1_0::ServiceStatus& status)
{
    MEDIA_INFO_LOG("HCameraHostManager::OnReceive for camera host %{public}s", status.serviceName.c_str());
    if (status.deviceClass != DEVICE_CLASS_CAMERA) {
        MEDIA_ERR_LOG("HCameraHostManager::OnReceive invalid device class %{public}d", status.deviceClass);
        return;
    }
    using namespace OHOS::HDI::ServiceManager::V1_0;
    std::lock_guard<std::mutex> lock(mutex_);
    switch (status.status) {
        case SERVIE_STATUS_START:
            AddCameraHost(status.serviceName);
            break;
        case SERVIE_STATUS_STOP:
            RemoveCameraHost(status.serviceName);
            break;
        default:
            MEDIA_ERR_LOG("HCameraHostManager::OnReceive unexpected service status %{public}d", status.status);
    }
}

void HCameraHostManager::AddCameraHost(const std::string& svcName)
{
    MEDIA_INFO_LOG("HCameraHostManager::AddCameraHost camera host %{public}s added", svcName.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::any_of(cameraHostInfos_.begin(), cameraHostInfos_.end(),
                    [&svcName](const auto& camHost) { return camHost->GetName() == svcName; })) {
        MEDIA_INFO_LOG("HCameraHostManager::AddCameraHost camera host  %{public}s already exists", svcName.c_str());
        return;
    }
    sptr<HCameraHostManager::CameraHostInfo> cameraHost = new(std::nothrow) HCameraHostManager::CameraHostInfo
                                                          (this, svcName);
    if (!cameraHost->Init()) {
        MEDIA_ERR_LOG("HCameraHostManager::AddCameraHost failed due to init failure");
        return;
    }
    cameraHostInfos_.push_back(cameraHost);
    std::vector<std::string> cameraIds;
    if (statusCallback_ && cameraHost->GetCameras(cameraIds) == CAMERA_OK) {
        for (const auto& cameraId : cameraIds) {
            statusCallback_->OnCameraStatus(cameraId, CAMERA_STATUS_AVAILABLE);
        }
    }
}

void HCameraHostManager::RemoveCameraHost(const std::string& svcName)
{
    MEDIA_INFO_LOG("HCameraHostManager::RemoveCameraHost camera host %{public}s removed", svcName.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(cameraHostInfos_.begin(), cameraHostInfos_.end(),
                           [&svcName](const auto& camHost) { return camHost->GetName() == svcName; });
    if (it == cameraHostInfos_.end()) {
        MEDIA_WARNING_LOG("HCameraHostManager::RemoveCameraHost camera host %{public}s doesn't exist", svcName.c_str());
        return;
    }
    std::vector<std::string> cameraIds;
    if ((*it)->GetCameras(cameraIds) == CAMERA_OK) {
        for (const auto& cameraId : cameraIds) {
            (*it)->OnCameraStatus(cameraId, Camera::UN_AVAILABLE);
        }
    }
    cameraHostInfos_.erase(it);
}

sptr<HCameraHostManager::CameraHostInfo> HCameraHostManager::FindCameraHostInfo(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& cameraHostInfo : cameraHostInfos_) {
        if (cameraHostInfo->IsCameraSupported(cameraId)) {
            return cameraHostInfo;
        }
    }
    return nullptr;
}
} // namespace CameraStandard
} // namespace OHOS
