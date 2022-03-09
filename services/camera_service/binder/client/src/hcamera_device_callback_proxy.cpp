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

#include "hcamera_device_callback_proxy.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HCameraDeviceCallbackProxy::HCameraDeviceCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ICameraDeviceServiceCallback>(impl) { }

int32_t HCameraDeviceCallbackProxy::OnError(const int32_t errorType, const int32_t errorMsg)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnError Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(errorType)) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnError Write errorType failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(errorMsg)) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnError Write errorMsg failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_DEVICE_ON_ERROR, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnError failed, error: %{public}d", error);
    }
    return error;
}

int32_t HCameraDeviceCallbackProxy::OnResult(const uint64_t timestamp,
                                             const std::shared_ptr<CameraStandard::CameraMetadata> &result)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool bRet = false;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnResult Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteUint64(timestamp)) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnResult Write timestamp failed");
        return IPC_PROXY_ERR;
    }
    bRet = MetadataUtils::EncodeCameraMetadata(result, data);
    if (!bRet) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnResult EncodeCameraMetadata failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_DEVICE_ON_RESULT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraDeviceCallbackProxy OnResult failed, error: %{public}d", error);
    }
    return error;
}
} // namespace CameraStandard
} // namespace OHOS
