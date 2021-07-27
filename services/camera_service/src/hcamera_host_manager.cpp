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
#include "hcamera_host_manager.h"
#include "media_log.h"

#include <iostream>

namespace OHOS {
namespace CameraStandard {
HCameraHostManager::HCameraHostManager()
{
}

HCameraHostManager::~HCameraHostManager()
{}

sptr<Camera::ICameraHost> HCameraHostManager::GetICameraHost() {
    if (cameraHostService_ == nullptr) {
        cameraHostService_ = Camera::ICameraHost::Get("camera_service");
        if (cameraHostService_ == nullptr) {
            MEDIA_ERR_LOG("Failed to get ICameraHost");
        }
    }
    return cameraHostService_;
}

int32_t HCameraHostManager::GetCameras(std::vector<std::string> &cameraIds)
{
    Camera::CamRetCode rc = GetICameraHost()->GetCameraIds(cameraIds);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraHostManager::GetCameras failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::GetCameraAbility(std::string &cameraId,
                                             std::shared_ptr<CameraMetadata> &ability)
{
    Camera::CamRetCode rc = GetICameraHost()->GetCameraAbility(cameraId, ability);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraHostManager::GetCameraAbility failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::OpenCameraDevice(std::string &cameraId,
                                             const sptr<Camera::ICameraDeviceCallback> &callback,
                                             sptr<Camera::ICameraDevice> &pDevice)
{
    Camera::CamRetCode rc = GetICameraHost()->OpenCamera(cameraId, callback, pDevice);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::SetCallback(sptr<Camera::ICameraHostCallback> &callback)
{
    Camera::CamRetCode rc = GetICameraHost()->SetCallback(callback);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraHostManager::CameraHostCallback SetCallback failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::SetFlashlight(std::string cameraId, bool isEnable)
{
    Camera::CamRetCode rc = GetICameraHost()->SetFlashlight(cameraId, isEnable);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCameraHostManager::SetFlashlight failed  with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}
} // namespace CameraStandard
} // namespace OHOS
