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

#include "hstream_capture_callback_proxy.h"
#include "media_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HStreamCaptureCallbackProxy::HStreamCaptureCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStreamCaptureCallback>(impl) { }

int32_t HStreamCaptureCallbackProxy::OnCaptureStarted(int32_t captureId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInt32(captureId)) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureStarted Write captureId failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_ON_CAPTURE_STARTED, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureStarted failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamCaptureCallbackProxy::OnCaptureEnded(int32_t captureId, int32_t frameCount)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInt32(captureId)) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureEnded Write captureId failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(frameCount)) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureEnded Write frameCount failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_ON_CAPTURE_ENDED, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureEnded failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamCaptureCallbackProxy::OnCaptureError(int32_t captureId, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInt32(captureId)) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureError Write captureId failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(errorCode)) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureError Write errorType failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_ON_CAPTURE_ERROR, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnCaptureError failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamCaptureCallbackProxy::OnFrameShutter(int32_t captureId, uint64_t timestamp)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInt32(captureId)) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnFrameShutter Write captureId failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteUint64(timestamp)) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnFrameShutter Write errorType failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_ON_FRAME_SHUTTER, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureCallbackProxy OnFrameShutter failed, error: %{public}d", error);
    }

    return error;
}
} // namespace CameraStandard
} // namespace OHOS