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

#include "hstream_repeat_callback_proxy.h"
#include "camera_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HStreamRepeatCallbackProxy::HStreamRepeatCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStreamRepeatCallback>(impl) { }

int32_t HStreamRepeatCallbackProxy::OnFrameStarted()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameStarted Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_STREAM_REPEAT_ON_FRAME_STARTED, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameStarted failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatCallbackProxy::OnFrameEnded(int32_t frameCount)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameEnded Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(frameCount)) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameEnded Write frameCount failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_REPEAT_ON_FRAME_ENDED, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameEnded failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatCallbackProxy::OnFrameError(int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameError Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(errorCode)) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameError Write errorType failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_REPEAT_ON_ERROR, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatCallbackProxy OnFrameError failed, error: %{public}d", error);
    }

    return error;
}
} // namespace CameraStandard
} // namespace OHOS
