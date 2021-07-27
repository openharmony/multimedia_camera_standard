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

#include "camera_util.h"
#include "hcamera_service.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"

#include <iostream>

namespace OHOS {
namespace CameraStandard {
REGISTER_SYSTEM_ABILITY_BY_ID(HCameraService, CAMERA_SERVICE_ID, true)

HCameraService::HCameraService(int32_t systemAbilityId, bool runOnCreate) : SystemAbility(systemAbilityId, runOnCreate)
{
    cameraHostManager_ = new HCameraHostManager();
    if (cameraHostManager_ == nullptr) {
        MEDIA_ERR_LOG("HCameraService:HCameraHostManager creation is failed");
        return;
    }
}

HCameraService::~HCameraService()
{}

void HCameraService::OnStart()
{
    bool res = Publish(this);
    if (res) {
        MEDIA_INFO_LOG("HCameraService OnStart res=%{public}d", res);
    }
}

void HCameraService::OnDump()
{
    MEDIA_INFO_LOG("HCameraService::OnDump called");
}

void HCameraService::OnStop()
{
    MEDIA_INFO_LOG("HCameraService::OnStop called");
}

int32_t HCameraService::GetCameras(std::vector<std::string> &cameraIds,
    std::vector<std::shared_ptr<CameraMetadata>> &cameraAbilityList)
{
    int32_t ret = cameraHostManager_->GetCameras(cameraIds);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("HCameraService::GetCameras failed");
        return ret;
    }
    std::shared_ptr<CameraMetadata> cameraAbility;
    for (auto id : cameraIds) {
        ret = cameraHostManager_->GetCameraAbility(id, cameraAbility);
            if (ret != CAMERA_OK) {
            MEDIA_ERR_LOG("HCameraService::GetCameraAbility failed");
            return ret;
        }
        cameraAbilityList.emplace_back(cameraAbility);
    }

    return ret;
}

int32_t HCameraService::CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService> &device)
{
    sptr<HCameraDevice> cameraDevice;

    if (cameraDeviceCallback_ == nullptr) {
        cameraDeviceCallback_ = new CameraDeviceCallback();
    }
    cameraDevice = new HCameraDevice(cameraHostManager_, cameraDeviceCallback_, cameraId);
    if (cameraDevice == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateCameraDevice-HCameraDevice allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    device = cameraDevice;
    return CAMERA_OK;
}

int32_t HCameraService::CreateCaptureSession(sptr<ICaptureSession> &session)
{
    sptr<HCaptureSession> captureSession;

    if (streamOperatorCallback_ == nullptr) {
        streamOperatorCallback_ = new StreamOperatorCallback();
    }
    captureSession = new HCaptureSession(cameraHostManager_, streamOperatorCallback_);
    if (captureSession == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateCaptureSession HCaptureSession allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    session = captureSession;
    return CAMERA_OK;
}

int32_t HCameraService::CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, sptr<IStreamCapture> &photoOutput)
{
    sptr<HStreamCapture> streamCapture;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePhotoOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamCapture = new HStreamCapture(producer);
    if (streamCapture == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePhotoOutput HStreamCapture allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    photoOutput = streamCapture;
    return CAMERA_OK;
}

int32_t HCameraService::CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, sptr<IStreamRepeat> &previewOutput)
{
    sptr<HStreamRepeat> streamRepeatPreview;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePreviewOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatPreview = new HStreamRepeat(producer);
    if (streamRepeatPreview == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePreviewOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    previewOutput = streamRepeatPreview;
    return CAMERA_OK;
}

int32_t HCameraService::CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, sptr<IStreamRepeat> &videoOutput)
{
    sptr<HStreamRepeat> streamRepeatVideo;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateVideoOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatVideo = new HStreamRepeat(producer, true);
    if (streamRepeatVideo == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateVideoOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    videoOutput = streamRepeatVideo;
    return CAMERA_OK;
}

CameraHostCallback::CameraHostCallback(sptr<ICameraServiceCallback> &callback)
{
    cameraServiceCallback = callback;
}

void CameraHostCallback::OnCameraStatus(const std::string &cameraId, Camera::CameraStatus status)
{
    CameraStatus svcStatus = CAMERA_STATUS_UNAVAILABLE;

    if (cameraServiceCallback != nullptr) {
        switch (status) {
            case Camera::UN_AVAILABLE:
                svcStatus = CAMERA_STATUS_UNAVAILABLE;
                break;

            case Camera::AVAILABLE:
                svcStatus = CAMERA_STATUS_AVAILABLE;
                break;

            default:
                MEDIA_ERR_LOG("Unknown camera status: %{public}d", status);
                return;
        }
        cameraServiceCallback->OnCameraStatusChanged(cameraId, svcStatus);
    }
}

void CameraHostCallback::OnFlashlightStatus(const std::string &cameraId, Camera::FlashlightStatus status)
{
    FlashStatus flashStaus = FLASH_STATUS_OFF;

    if (cameraServiceCallback != nullptr) {
        switch (status) {
            case Camera::FLASHLIGHT_OFF:
                flashStaus = FLASH_STATUS_OFF;
                break;

            case Camera::FLASHLIGHT_ON:
                flashStaus = FLASH_STATUS_ON;
                break;

            case Camera::FLASHLIGHT_UNAVAILABLE:
                flashStaus = FLASH_STATUS_UNAVAILABLE;
                break;

            default:
                MEDIA_ERR_LOG("Unknown flashlight status: %{public}d", status);
                return;
        }
        cameraServiceCallback->OnFlashlightStatusChanged(cameraId, flashStaus);
    }
}

int32_t HCameraService::SetCallback(sptr<ICameraServiceCallback> &callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("HCameraService::SetCallback callback is null");
        return CAMERA_INVALID_ARG;
    }
    cameraHostCallback_ = new CameraHostCallback(callback);
    if (cameraHostCallback_ == nullptr) {
        MEDIA_ERR_LOG("HCameraService::SetCallback CameraHostCallback allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    int ret = cameraHostManager_->SetCallback(cameraHostCallback_);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("Setting cameraHostCallback failed");
        return ret;
    }
    return CAMERA_OK;
}
} // namespace CameraStandard
} // namespace OHOS
