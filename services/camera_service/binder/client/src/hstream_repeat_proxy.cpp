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

#include "hstream_repeat_proxy.h"
#include "camera_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HStreamRepeatProxy::HStreamRepeatProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStreamRepeat>(impl) { }

int32_t HStreamRepeatProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Start Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_START_VIDEO_RECORDING, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Start failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Stop Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_STOP_VIDEO_RECORDING, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Stop failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Release Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_STREAM_REPEAT_RELEASE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Stop failed, error: %{public}d", error);
    }
    return error;
}

int32_t HStreamRepeatProxy::SetFps(float fps)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetFps Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteFloat(fps)) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetFps Write Fps failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_REPEAT_SET_FPS, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetFps failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatProxy::SetCallback(sptr<IStreamRepeatCallback> &callback)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (callback == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetCallback callback is null");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetCallback Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteRemoteObject(callback->AsObject())) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetCallback write StreamRepeatCallback obj failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_REPEAT_SET_CALLBACK, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetCallback failed, error: %{public}d", error);
    }

    return error;
}
} // namespace CameraStandard
} // namespace OHOS
