/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "hcapture_session_callback_proxy.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HCaptureSessionCallbackProxy::HCaptureSessionCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ICaptureSessionCallback>(impl) { }

int32_t HCaptureSessionCallbackProxy::OnError(int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCaptureSessionCallbackProxy OnError Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(errorCode)) {
        MEDIA_ERR_LOG("HCaptureSessionCallbackProxy OnError Write errorCode failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_CAPTURE_SESSION_ON_ERROR, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCaptureSessionCallbackProxy OnError failed, error: %{public}d", error);
    }
    return error;
}
} // namespace CameraStandard
} // namespace OHOS
