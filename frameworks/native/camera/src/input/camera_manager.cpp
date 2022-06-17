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

#include "input/camera_manager.h"

#include <cstring>

#include "camera_util.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "camera_log.h"
#include "system_ability_definition.h"

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

int32_t CameraManager::CreateListenerObject()
{
    listenerStub_ = new(std::nothrow) CameraListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, CAMERA_ALLOC_ERROR,
        "failed to new CameraListenerStub object");
    CHECK_AND_RETURN_RET_LOG(serviceProxy_ != nullptr, CAMERA_ALLOC_ERROR,
        "Camera service does not exist.");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, CAMERA_ALLOC_ERROR, "listener object is nullptr..");

    MEDIA_DEBUG_LOG("CreateListenerObject");
    return serviceProxy_->SetListenerObject(object);
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

    int32_t OnCameraStatusChanged(const std::string& cameraId, const CameraStatus status) override
    {
        CameraDeviceStatus deviceStatus;
        CameraStatusInfo cameraStatusInfo;

        CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("OnCameraStatusChanged! for cameraId:%s, current Camera Status:%d",
                                           cameraId.c_str(), status));

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
            if (cameraStatusInfo.cameraInfo) {
                MEDIA_INFO_LOG("OnCameraStatusChanged: cameraId: %{public}s, status: %{public}d",
                               cameraId.c_str(), status);
                camMngr_->GetApplicationCallback()->OnCameraStatusChanged(cameraStatusInfo);
            }
        } else {
            MEDIA_INFO_LOG("CameraManager::Callback not registered!, Ignore the callback");
        }
        return CAMERA_OK;
    }

    int32_t OnFlashlightStatusChanged(const std::string& cameraId, const FlashStatus status) override
    {
        FlashlightStatus flashlightStatus;

        CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("OnFlashlightStatusChanged! for cameraId:%s, current Flash Status:%d",
                                           cameraId.c_str(), status));
        POWERMGR_SYSEVENT_TORCH_STATE(IPCSkeleton::GetCallingPid(),
                                      IPCSkeleton::GetCallingUid(), status);

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
    CAMERA_SYNC_TRACE;
    sptr<ICaptureSession> captureSession = nullptr;
    sptr<CaptureSession> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreateCaptureSession serviceProxy_ is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreateCaptureSession(captureSession);
    if (retCode == CAMERA_OK && captureSession != nullptr) {
        result = new(std::nothrow) CaptureSession(captureSession);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new CaptureSession");
        }
    } else {
        MEDIA_ERR_LOG("Failed to get capture session object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PhotoOutput> CameraManager::CreatePhotoOutput(sptr<Surface> &surface)
{
    CAMERA_SYNC_TRACE;
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
        result = new(std::nothrow) PhotoOutput(streamCapture);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new PhotoOutput ");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(PHOTO, surface->GetDefaultWidth(),
                                            surface->GetDefaultHeight());
        }
    } else {
        MEDIA_ERR_LOG("Failed to get stream capture object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PhotoOutput> CameraManager::CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format)
{
    CAMERA_SYNC_TRACE;
    sptr<IStreamCapture> streamCapture = nullptr;
    sptr<PhotoOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || producer == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreatePhotoOutput serviceProxy_ is null or producer is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreatePhotoOutput(producer, format, streamCapture);
    if (retCode == CAMERA_OK) {
        result = new(std::nothrow) PhotoOutput(streamCapture);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new PhotoOutput");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(PHOTO,
                                            producer->GetDefaultWidth(),
                                            producer->GetDefaultHeight());
        }
    } else {
        MEDIA_ERR_LOG("Failed to get stream capture object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreatePreviewOutput(sptr<Surface> surface)
{
    CAMERA_SYNC_TRACE;
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
        result = new(std::nothrow) PreviewOutput(streamRepeat);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new PreviewOutput");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(PREVIEW,
                                            surface->GetDefaultWidth(),
                                            surface->GetDefaultHeight());
        }
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format)
{
    CAMERA_SYNC_TRACE;
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<PreviewOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || producer == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreatePreviewOutput serviceProxy_ is null or producer is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreatePreviewOutput(producer, format, streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new(std::nothrow) PreviewOutput(streamRepeat);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new PreviewOutput");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(PREVIEW,
                                            producer->GetDefaultWidth(),
                                            producer->GetDefaultHeight());
        }
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreateCustomPreviewOutput(sptr<Surface> surface, int32_t width, int32_t height)
{
    CAMERA_SYNC_TRACE;
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
        result = new(std::nothrow) PreviewOutput(streamRepeat);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new PreviewOutput");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(PREVIEW, width, height);
        }
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<PreviewOutput> CameraManager::CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer,
                                                             int32_t format, int32_t width, int32_t height)
{
    CAMERA_SYNC_TRACE;
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<PreviewOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if ((serviceProxy_ == nullptr) || (producer == nullptr) || (width == 0) || (height == 0)) {
        MEDIA_ERR_LOG("CameraManager::CreatePreviewOutput serviceProxy_ is null or producer is null or invalid size");
        return nullptr;
    }
    retCode = serviceProxy_->CreateCustomPreviewOutput(producer, format, width, height, streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new(std::nothrow) PreviewOutput(streamRepeat);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new PreviewOutput");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(PREVIEW, width, height);
        }
    } else {
        MEDIA_ERR_LOG("PreviewOutput: Failed to get stream repeat object from hcamera service!, %{public}d", retCode);
    }
    return result;
}

sptr<MetadataOutput> CameraManager::CreateMetadataOutput()
{
    CAMERA_SYNC_TRACE;
    sptr<IStreamMetadata> streamMetadata = nullptr;
    sptr<MetadataOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (!serviceProxy_) {
        MEDIA_ERR_LOG("CameraManager::CreateMetadataOutput serviceProxy_ is null");
        return nullptr;
    }
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    if (!surface) {
        MEDIA_ERR_LOG("CameraManager::CreateMetadataOutput Failed to create surface");
        return nullptr;
    }
    int32_t format = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    int32_t width = 1920;
    int32_t height = 1080;
#ifdef RK_CAMERA
    format = OHOS_CAMERA_FORMAT_RGBA_8888;
#endif
    surface->SetDefaultWidthAndHeight(width, height);
    retCode = serviceProxy_->CreateMetadataOutput(surface->GetProducer(), format, streamMetadata);
    if (retCode) {
        MEDIA_ERR_LOG("CameraManager::CreateMetadataOutput Failed to get stream metadata object from hcamera service!, "
                      "%{public}d", retCode);
        return nullptr;
    }
    result = new(std::nothrow) MetadataOutput(surface, streamMetadata);
    if (!result) {
        MEDIA_ERR_LOG("CameraManager::CreateMetadataOutput Failed to allocate MetadataOutput");
        return nullptr;
    }
    sptr<IBufferConsumerListener> listener = new(std::nothrow) MetadataObjectListener(result);
    if (!listener) {
        MEDIA_ERR_LOG("CameraManager::CreateMetadataOutput Failed to allocate metadata object listener");
        return nullptr;
    }
    SurfaceError ret = surface->RegisterConsumerListener(listener);
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_ERR_LOG("CameraManager::CreateMetadataOutput Surface consumer listener registration failed");
        return nullptr;
    }
    POWERMGR_SYSEVENT_CAMERA_CONFIG(METADATA, width, height);
    return result;
}

sptr<VideoOutput> CameraManager::CreateVideoOutput(sptr<Surface> &surface)
{
    CAMERA_SYNC_TRACE;
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
        result = new(std::nothrow) VideoOutput(streamRepeat);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new VideoOutput");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(VIDEO,
                                            surface->GetDefaultWidth(),
                                            surface->GetDefaultHeight());
        }
    } else {
        MEDIA_ERR_LOG("VideoOutpout: Failed to get stream repeat object from hcamera service! %{public}d", retCode);
    }
    return result;
}

sptr<VideoOutput> CameraManager::CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format)
{
    CAMERA_SYNC_TRACE;
    sptr<IStreamRepeat> streamRepeat = nullptr;
    sptr<VideoOutput> result = nullptr;
    int32_t retCode = CAMERA_OK;

    if (serviceProxy_ == nullptr || producer == nullptr) {
        MEDIA_ERR_LOG("CameraManager::CreateVideoOutput serviceProxy_ is null or producer is null");
        return nullptr;
    }
    retCode = serviceProxy_->CreateVideoOutput(producer, format, streamRepeat);
    if (retCode == CAMERA_OK) {
        result = new(std::nothrow) VideoOutput(streamRepeat);
        if (result == nullptr) {
            MEDIA_ERR_LOG("Failed to new VideoOutput");
        } else {
            POWERMGR_SYSEVENT_CAMERA_CONFIG(VIDEO,
                                            producer->GetDefaultWidth(),
                                            producer->GetDefaultHeight());
        }
    } else {
        MEDIA_ERR_LOG("VideoOutpout: Failed to get stream repeat object from hcamera service! %{public}d", retCode);
    }
    return result;
}

void CameraManager::Init()
{
    CAMERA_SYNC_TRACE;
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
        cameraSvcCallback_ = new(std::nothrow) CameraStatusServiceCallback(this);
        if (cameraSvcCallback_) {
            SetCameraServiceCallback(cameraSvcCallback_);
        } else {
            MEDIA_ERR_LOG("CameraManager::init Failed to new CameraStatusServiceCallback.");
        }
    }
    pid_t pid = 0;
    deathRecipient_ = new(std::nothrow) CameraDeathRecipient(pid);
    CHECK_AND_RETURN_LOG(deathRecipient_ != nullptr, "failed to new CameraDeathRecipient.");

    deathRecipient_->SetNotifyCb(std::bind(&CameraManager::CameraServerDied, this, std::placeholders::_1));
    bool result = object->AddDeathRecipient(deathRecipient_);
    if (!result) {
        MEDIA_ERR_LOG("failed to add deathRecipient");
        return;
    }

    int32_t ret = CreateListenerObject();
    CHECK_AND_RETURN_LOG(ret == CAMERA_OK, "failed to new MediaListener.");
}

void CameraManager::CameraServerDied(pid_t pid)
{
    MEDIA_ERR_LOG("camera server has died, pid:%{public}d!", pid);
    if (serviceProxy_ != nullptr) {
        (void)serviceProxy_->AsObject()->RemoveDeathRecipient(deathRecipient_);
        serviceProxy_ = nullptr;
    }
    listenerStub_ = nullptr;
    deathRecipient_ = nullptr;
}

sptr<ICameraDeviceService> CameraManager::CreateCameraDevice(std::string cameraId)
{
    CAMERA_SYNC_TRACE;
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

sptr<CameraManager> &CameraManager::GetInstance()
{
    if (CameraManager::cameraManager_ == nullptr) {
        MEDIA_INFO_LOG("Initializing camera manager for first time!");
        CameraManager::cameraManager_ = new(std::nothrow) CameraManager();
        if (CameraManager::cameraManager_ == nullptr) {
            MEDIA_ERR_LOG("CameraManager::GetInstance failed to new CameraManager");
        }
    }
    return CameraManager::cameraManager_;
}

std::vector<sptr<CameraInfo>> CameraManager::GetCameras()
{
    CAMERA_SYNC_TRACE;

    std::vector<std::string> cameraIds;
    std::vector<std::shared_ptr<Camera::CameraMetadata>> cameraAbilityList;
    int32_t retCode = -1;
    sptr<CameraInfo> cameraObj = nullptr;
    int32_t index = 0;

    if (cameraObjList.size() > 0) {
        cameraObjList.clear();
    }
    if (serviceProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraManager::GetCameras serviceProxy_ is null, returning empty list!");
        return cameraObjList;
    }
    retCode = serviceProxy_->GetCameras(cameraIds, cameraAbilityList);
    if (retCode == CAMERA_OK) {
        for (auto& it : cameraIds) {
            cameraObj = new(std::nothrow) CameraInfo(it, cameraAbilityList[index++]);
            if (cameraObj == nullptr) {
                MEDIA_ERR_LOG("CameraManager::GetCameras new CameraInfo failed for id={public}%s", it.c_str());
                continue;
            }
            CAMERA_SYSEVENT_STATISTIC(CreateMsg("CameraManager GetCameras camera ID:%s, Camera position:%d,"
                                                " Camera Type:%d, Connection Type:%d, Mirror support:%d", it.c_str(),
                                                cameraObj->GetPosition(), cameraObj->GetCameraType(),
                                                cameraObj->GetConnectionType(), cameraObj->IsMirrorSupported()));
            cameraObjList.emplace_back(cameraObj);
        }
    } else {
        MEDIA_ERR_LOG("CameraManager::GetCameras failed!, retCode: %{public}d", retCode);
    }
    return cameraObjList;
}

sptr<CameraInput> CameraManager::CreateCameraInput(sptr<CameraInfo> &camera)
{
    CAMERA_SYNC_TRACE;
    sptr<CameraInput> cameraInput = nullptr;
    sptr<ICameraDeviceService> deviceObj = nullptr;

    if (camera != nullptr) {
        deviceObj = CreateCameraDevice(camera->GetID());
        if (deviceObj != nullptr) {
            cameraInput = new(std::nothrow) CameraInput(deviceObj, camera);
            if (cameraInput == nullptr) {
                MEDIA_ERR_LOG("failed to new CameraInput Returning null in CreateCameraInput");
                return cameraInput;
            }
        } else {
            MEDIA_ERR_LOG("Returning null in CreateCameraInput");
        }
    } else {
        MEDIA_ERR_LOG("CameraManager::CreateCameraInput: Camera object is null");
    }
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CameraManager_CreateCameraInput CameraId:%s", camera->GetID().c_str()));

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
