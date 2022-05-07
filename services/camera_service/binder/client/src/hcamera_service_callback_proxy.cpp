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

#include "hcamera_service_callback_proxy.h"
#include "camera_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HCameraServiceCallbackProxy::HCameraServiceCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ICameraServiceCallback>(impl) { }

int32_t HCameraServiceCallbackProxy::OnCameraStatusChanged(const std::string& cameraId, const CameraStatus status)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnCameraStatusChanged Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteString(cameraId)) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnCameraStatusChanged Write CameraId failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(status)) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnCameraStatusChanged Write status failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_CALLBACK_STATUS_CHANGED, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnCameraStatusChanged failed, error: %{public}d", error);
    }
    return error;
}

int32_t HCameraServiceCallbackProxy::OnFlashlightStatusChanged(const std::string& cameraId, const FlashStatus status)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = ERR_NONE;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnFlashlightStatus Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteString(cameraId)) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnFlashlightStatus Write CameraId failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(status)) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnFlashlightStatus Write status failed");
        return IPC_PROXY_ERR;
    }
    error = Remote()->SendRequest(CAMERA_CALLBACK_FLASHLIGHT_STATUS_CHANGED, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceCallbackProxy OnFlashlightStatus failed, error: %{public}d", error);
    }
    return error;
}
} // namespace CameraStandard
} // namespace OHOS

