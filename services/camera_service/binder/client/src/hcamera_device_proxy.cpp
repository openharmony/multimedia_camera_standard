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

#include "hcamera_device_proxy.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"


namespace OHOS {
namespace CameraStandard {
HCameraDeviceProxy::HCameraDeviceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ICameraDeviceService>(impl) { }

int32_t HCameraDeviceProxy::Open()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy Open Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_DEVICE_OPEN, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy Open failed, error: %{public}d", error);
    }
    return error;
}

int32_t HCameraDeviceProxy::Close()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy Close Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_DEVICE_CLOSE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy Close failed, error: %{public}d", error);
    }

    return error;
}

int32_t HCameraDeviceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy Release Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_DEVICE_RELEASE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy Release failed, error: %{public}d", error);
    }

    return error;
}

int32_t HCameraDeviceProxy::SetCallback(sptr<ICameraDeviceServiceCallback>& callback)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (callback == nullptr) {
        MEDIA_ERR_LOG("HCameraDeviceProxy SetCallback callback is null");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy SetCallback Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteRemoteObject(callback->AsObject())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy SetCallback write CameraDeviceServiceCallback obj failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_DEVICE_SET_CALLBACK, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy SetCallback failed, error: %{public}d", error);
    }

    return error;
}

int32_t HCameraDeviceProxy::UpdateSetting(const std::shared_ptr<Camera::CameraMetadata> &settings)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy UpdateSetting Write interface token failed");
        return IPC_PROXY_ERR;
    }
    bool bRet = Camera::MetadataUtils::EncodeCameraMetadata(settings, data);
    if (!bRet) {
        MEDIA_ERR_LOG("HCameraDeviceProxy UpdateSetting EncodeCameraMetadata failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_DEVICE_UPDATE_SETTNGS, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy UpdateSetting failed, error: %{public}d", error);
    }

    return error;
}

int32_t HCameraDeviceProxy::GetEnabledResults(std::vector<int32_t> &results)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy GetEnabledResults Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_DEVICE_GET_ENABLED_RESULT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy GetEnabledResults failed, error: %{public}d", error);
        return IPC_PROXY_ERR;
    }

    if (!reply.ReadInt32Vector(&results)) {
        MEDIA_ERR_LOG("HCameraDeviceProxy GetEnabledResults read results failed");
        return IPC_PROXY_ERR;
    }

    return error;
}

int32_t HCameraDeviceProxy::EnableResult(std::vector<int32_t> &results)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy EnableResult Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32Vector(results)) {
        MEDIA_ERR_LOG("HCameraDeviceProxy EnableResult write results failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_DEVICE_ENABLED_RESULT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy EnableResult failed, error: %{public}d", error);
    }

    return error;
}

int32_t HCameraDeviceProxy::DisableResult(std::vector<int32_t> &results)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceProxy DisableResult Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32Vector(results)) {
        MEDIA_ERR_LOG("HCameraDeviceProxy DisableResult write results failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_DEVICE_DISABLED_RESULT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceProxy DisableResult failed, error: %{public}d", error);
    }

    return error;
}
} // namespace CameraStandard
} // namespace OHOS
