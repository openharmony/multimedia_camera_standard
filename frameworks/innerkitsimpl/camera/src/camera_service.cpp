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

#include "camera_service.h"
#include "hi_type.h"
#include "media_log.h"
#include "sdk.h"

using namespace std;

namespace OHOS {
namespace Media {
string g_mainCamera = "main";
bool g_camSdkInit = false;

CameraService::CameraService()
{
    HI_S32 ret = sdk_init();
    if (ret == SDK_INIT_SUCC) {
        g_camSdkInit = true;
        MEDIA_DEBUG_LOG("sdk intialized successfully");
    } else if (ret == SDK_INIT_REENTRY) {
        MEDIA_DEBUG_LOG("sdk already intialized");
    } else {
        MEDIA_ERR_LOG("sdk init failed, ret = %d", ret);
    }
}

CameraService::~CameraService()
{
    if (ability_) {
        delete ability_;
        ability_ = nullptr;
    }

    if (device_) {
        device_->UnInitialize();
        delete device_;
        device_ = nullptr;
    }

    if (g_camSdkInit) {
        sdk_exit();
        g_camSdkInit = false;
        MEDIA_DEBUG_LOG("sdk exited successfully");
    }
}

CameraService *CameraService::GetInstance()
{
    static CameraService instance;
    return &instance;
}

void CameraService::Initialize(CameraServiceCallback &callback)
{
    MEDIA_DEBUG_LOG("Camera service initializing.");
    cameraServiceCb_ = &callback;
    InitCameraDevices();

    list<string> cameraList = {g_mainCamera};
    cameraServiceCb_->OnCameraServiceInitialized(cameraList);
}

const CameraAbility *CameraService::GetCameraAbility(std::string &cameraId)
{
    if ((cameraId == g_mainCamera) && (device_ != nullptr)) {
        return ability_;
    }
    return nullptr;
}

void CameraService::CreateCamera(string cameraId)
{
    if (cameraId != g_mainCamera) {
        MEDIA_ERR_LOG("This camera does not exist. (cameraId=%s)", cameraId.c_str());
        return;
    }
    cameraServiceCb_->OnCameraStatusChange(cameraId, CameraServiceCallback::CAMERA_STATUS_CREATED, *device_);
}

int32_t CameraService::InitCameraDevices()
{
    /* Only one camera device now, support more cameras in future */
    device_ = new (nothrow) CameraDevice;
    if (device_ == nullptr) {
        MEDIA_FATAL_LOG("New object failed.");
        return MEDIA_ERR;
    }
    ability_ = new (nothrow) CameraAbility;
    if (ability_ == nullptr) {
        delete device_;
        device_ = nullptr;
        MEDIA_FATAL_LOG("New object failed.");
        return MEDIA_ERR;
    }

    if (device_->Initialize(*ability_) != MEDIA_OK) {
        delete device_;
        delete ability_;
        device_ = nullptr;
        ability_ = nullptr;
        return MEDIA_ERR;
    }
    return MEDIA_OK;
}
} // namespace Media
} // namespace OHOS