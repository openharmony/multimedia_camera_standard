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

#include <cstring>
#include "camera_util.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"
#include "ipc_skeleton.h"
#include "accesstoken_kit.h"
#include "input/camera_manager.h"

using namespace std;
namespace OHOS {
namespace CameraStandard {
sptr<CameraManager> CameraManager::cameraManager_;

const std::string CameraManager::surfaceFormat = "CAMERA_SURFACE_FORMAT";

CameraManager::CameraManager()
{
    Init();
    cameraObjList = {};
}

class CameraStatusServiceCallback : public HCameraServiceCallbackStub {
public:
    sptr<CameraManager> camMngr_ = nullptr;
    CameraStatusServiceCallback() : camMngr_(nullptr) {
    }

    explicit CameraStatusServiceCallback(const sptr<CameraManager>& cameraManager) : camMngr_(cameraManager) {
    }

    ~CameraStatusServiceCallback()
    {
        camMngr_ = nullptr;
    }

    int32_t OnCameraStatusChanged(const std::string cameraId, const CameraStatus status) override
    {
        CameraDeviceStatus deviceStatus;
        CameraStatusInfo cameraStatusInfo;

        MEDIA_INFO_LOG("OnCameraStatusChanged: cameraId: %{public}s, status: %{public}d", cameraId.c_str(), status);
        if (camMngr_ != nullptr && camMngr_->GetApplicationCallback() != nullptr) {
            switch (status) {
                case CAMERA_STATUS_UNAVAILABLE:
                    deviceStatus = CAMERA_DEVICE_STATUS_UNAVAILABLE;
                    break;

                case CAMERA_STATUS_AVAILABLE:
                    deviceStatus = CAMERA_DEVICE_STATUS_AVAILABLE;
                    break;

                default:
                    MEDIA_ERR_LOG("Unknown camera status: %{public}d", status);
                    return CAMERA_INVALID_ARG;
            }
            cameraStatusInfo.cameraInfo = camMngr_->GetCameraInfo(cameraId);
            cameraStatusInfo.cameraStatus = deviceStatus;
            camMngr_->GetApplicationCallback()->OnCameraStatusChanged(cameraStatusInfo);
        } else {
            MEDIA_INFO_LOG("CameraManager::Callback not registered!, Ignore the callback");
        }
        return CAMERA_OK;
    }

    int32_t OnFlashlightStatusChanged(const std::string cameraId, const FlashStatus status) override
    {
        FlashlightStatus flashlightStatus;

        MEDIA_INFO_LOG("OnFlashlightStatusChanged: cameraId: %{public}s, status: %{public}d", cameraId.c_str(), status);
        if (camMngr_ != nullptr && camMngr_->GetApplicationCallback() != nullptr) {
            switch (status) {
                case FLASH_STATUS_OFF:
                    flashlightStatus = FLASHLIGHT_STATUS_OFF;
                    break;

                case FLASH_STATUS_ON:
                    flashlightStatus = FLASHLIGHT_STATUS_ON;
                    break;

                case FLASH_STATUS_UNAVAILABLE:
                    flashlightStatus = FLASHLIGHT_STATUS_UNAVAILABLE;
                    break;

                default:
                    MEDIA_ERR_LOG("Unknown flashlight status: %{public}d", status);
                    return CAMERA_INVALID_ARG;
            }
            camMngr_->GetApplicationCallback()->OnFlashlightStatusChanged(cameraId, flashlightStatus);
        } else {
            MEDIA_INFO_LOG("CameraManager::Callback not registered!, Ignore the callback");
        }
        return CAMERA_OK;
    }
};

sptr<CaptureSession> CameraManager::CreateCaptureSession()
{
    sptr<ICaptureSession> captureSession = nullptr;
    sptr<CaptureSession> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreateCaptureSession serviceProxy_ is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreateCaptureSession(captureSession);
    if (retCode == CAMERA_OK && captureSession != nullptr) {
        result = new CaptureSession(captureSession);
    } else {
        MEDIA_ERR_LOG("Failed to get capture session object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PhotoOutput> CameraManager::CreatePhotoOutput(sptr<Surface> &surface)
{
    sptr<IStreamCapture> streamCapture = nullptr;
    sptr<PhotoOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || surface == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreatePhotoOutput serviceProxy_ is null or surface is null");
        return nullptr;
    }
    std::string format = surface->GetUserData(surfaceFormat);
    retCode = serviceProxy_->CreatePhotoOutput(surface->GetProducer(), std::stoi(format), streamCapture);
    if (retCode == CAMERA_OK) {
        result = new PhotoOutput(streamCapture);
    } else {
        MEDIA_ERR_LOG("Failed to get stream capture object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PhotoOutput> CameraManager::CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format)
{
    sptr<IStreamCapture> streamCapture = nullptr;
    sptr<PhotoOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || producer == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreatePhotoOutput serviceProxy_ is null or producer is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreatePhotoOutput(producer, format, streamCapture);
    if (retCode == CAMERA_OK) {
        result = new PhotoOutput(streamCapture);
    } else {
        MEDIA_ERR_LOG("Failed to get stream capture object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreatePreviewOutput(sptr<Surface> surface)
{
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<PreviewOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || surface == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreatePreviewOutput serviceProxy_ is null or surface is null");
        return nullptr;
    }
    std::string format = surface->GetUserData(surfaceFormat);
    retCode = serviceProxy_->CreatePreviewOutput(surface->GetProducer(), std::stoi(format), streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new PreviewOutput(streamRepeat);
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format)
{
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<PreviewOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || producer == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreatePreviewOutput serviceProxy_ is null or producer is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreatePreviewOutput(producer, format, streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new PreviewOutput(streamRepeat);
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreateCustomPreviewOutput(sptr<Surface> surface, int32_t width, int32_t height)
{
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<PreviewOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || surface == nullptr || width == 0 || height == 0) {
        MEDIA_ERR_LOG("CameraManager::CreatePreviewOutput serviceProxy_ is null or surface is null or invalid size");
        return nullptr;
    }
    std::string format = surface->GetUserData(surfaceFormat);
    retCode = serviceProxy_->CreateCustomPreviewOutput(surface->GetProducer(), std::stoi(format), width, height,
                                                       streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new PreviewOutput(streamRepeat);
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer,
                                                             int32_t format, int32_t width, int32_t height)
{
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<PreviewOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || producer == nullptr || width == 0 || height == 0) {
        MEDIA_ERR_LOG("CameraManager::CreatePreviewOutput serviceProxy_ is null or producer is null or invalid size");
        return nullptr;
    }
    retCode = serviceProxy_->CreateCustomPreviewOutput(producer, format, width, height, streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new PreviewOutput(streamRepeat);
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<VideoOutput> CameraManager::CreateVideoOutput(sptr<Surface> &surface)
{
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<VideoOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || surface == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreateVideoOutput serviceProxy_ is null or surface is null");
        return nullptr;
    }
    std::string format = surface->GetUserData(surfaceFormat);
    retCode = serviceProxy_->CreateVideoOutput(surface->GetProducer(), std::stoi(format), streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new VideoOutput(streamRepeat);
    } else {
        MEDIA_ERR_LOG("VideoOutpout: Failed to get stream repeat object from hcamera service! %{public}d", retCode);
    }
    return result;
}

sptr<VideoOutput> CameraManager::CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format)
{
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<VideoOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || producer == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreateVideoOutput serviceProxy_ is null or producer is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreateVideoOutput(producer, format, streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new VideoOutput(streamRepeat);
    } else {
        MEDIA_ERR_LOG("VideoOutpout: Failed to get stream repeat object from hcamera service! %{public}d", retCode);
    }
    return result;
}

void CameraManager::Init()
{
    sptr<IRemoteObject> object = nullptr;
    cameraMngrCallback_ = nullptr;

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        MEDIA_ERR_LOG("Failed to get System ability manager");
        return;
    }
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    if (object == nullptr) {
        MEDIA_ERR_LOG("CameraManager::GetSystemAbility() is failed");
        return;
    }
    serviceProxy_ = iface_cast<ICameraService>(object);
    if (serviceProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraManager::init serviceProxy_ is null.");
        return;
    } else {
        sptr<CameraManager> helper = this;
        cameraSvcCallback_ = new CameraStatusServiceCallback(helper);
        SetCameraServiceCallback(cameraSvcCallback_);
    }
}

sptr<ICameraDeviceService> CameraManager::CreateCameraDevice(std::string cameraId)
{
    sptr<ICameraDeviceService> device = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || cameraId.empty()) {
        MEDIA_ERR_LOG("GetCameaDevice() serviceProxy_ is null or CameraID is empty: %{public}s", cameraId.c_str());
        return nullptr;
    }
    retCode = serviceProxy_->CreateCameraDevice(cameraId, device);
    if (retCode != CAMERA_OK) {
        MEDIA_ERR_LOG("ret value from CreateCameraDevice, %{public}d", retCode);
        return nullptr;
    }
    return device;
}

void CameraManager::SetCallback(std::shared_ptr<CameraManagerCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_INFO_LOG("CameraManager::SetCallback(): Application unregistering the callback");
    }
    cameraMngrCallback_ = callback;
}

std::shared_ptr<CameraManagerCallback> CameraManager::GetApplicationCallback()
{
    return cameraMngrCallback_;
}

sptr<CameraInfo> CameraManager::GetCameraInfo(std::string cameraId)
{
    sptr<CameraInfo> cameraObj = nullptr;

    for (size_t i = 0; i < cameraObjList.size(); i++) {
        if (cameraObjList[i]->GetID() == cameraId) {
            cameraObj = cameraObjList[i];
            break;
        }
    }
    return cameraObj;
}

void CameraManager::SetPermissionCheck(bool Enable)
{
    permissionCheckEnabled_ = Enable;
}

sptr<CameraManager> &CameraManager::GetInstance()
{
    if (CameraManager::cameraManager_ == nullptr) {
        MEDIA_INFO_LOG("Initializing camera manager for first time!");
        CameraManager::cameraManager_ = new CameraManager();
    }
    return CameraManager::cameraManager_;
}

std::vector<sptr<CameraInfo>> CameraManager::GetCameras()
{
    std::vector<std::string> cameraIds;
    std::vector<std::shared_ptr<CameraMetadata>> cameraAbilityList;
    int32_t retCode = -1;
    sptr<CameraInfo> cameraObj = nullptr;
    int32_t index = 0;

    if (cameraObjList.size() > 0) {
        cameraObjList.clear();
    }
    if (serviceProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraManager::SetCallback serviceProxy_ is null, returning empty list!");
        return cameraObjList;
    }
    retCode = serviceProxy_->GetCameras(cameraIds, cameraAbilityList);
    if (retCode == CAMERA_OK) {
        for (auto& it : cameraIds) {
            cameraObj = new CameraInfo(it, cameraAbilityList[index++]);
            cameraObjList.emplace_back(cameraObj);
        }
    } else {
        MEDIA_ERR_LOG("CameraManager::GetCameras failed!, retCode: %{public}d", retCode);
    }
    return cameraObjList;
}

sptr<CameraInput> CameraManager::CreateCameraInput(sptr<CameraInfo> &camera)
{
    sptr<CameraInput> cameraInput = nullptr;
    sptr<ICameraDeviceService> deviceObj = nullptr;

    if (permissionCheckEnabled_) {
        MEDIA_DEBUG_LOG("CameraManager::CreateCameraInput: Verifying permission to access the camera");
        Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
        std::string permissionName = "ohos.permission.CAMERA";

        int permission_result = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permissionName);
        if (permission_result != Security::AccessToken::TypePermissionState::PERMISSION_GRANTED) {
            MEDIA_ERR_LOG("CameraManager::CreateCameraInput: Permission to Access Camera Denied!!!!");
            return cameraInput;
        } else {
            MEDIA_DEBUG_LOG("CameraManager::CreateCameraInput: Permission to Access Camera Granted!!!!");
        }
    }

    if (camera != nullptr) {
        deviceObj = CreateCameraDevice(camera->GetID());
        if (deviceObj != nullptr) {
            cameraInput = new CameraInput(deviceObj, camera);
        } else {
            MEDIA_ERR_LOG("Returning null in CreateCameraInput");
        }
    } else {
        MEDIA_ERR_LOG("CameraManager::CreateCameraInput: Camera object is null");
    }
    return cameraInput;
}

void CameraManager::SetCameraServiceCallback(sptr<ICameraServiceCallback>& callback)
{
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraManager::SetCallback serviceProxy_ is null");
        return;
    }
    retCode = serviceProxy_->SetCallback(callback);
    if (retCode != CAMERA_OK) {
        MEDIA_ERR_LOG("CameraManager::Set service Callback failed, retCode: %{public}d", retCode);
    }
    return;
}
} // CameraStandard
} // OHOS
