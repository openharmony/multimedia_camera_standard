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

#include "hstream_capture_proxy.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HStreamCaptureProxy::HStreamCaptureProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStreamCapture>(impl) { }

int32_t HStreamCaptureProxy::Capture(const std::shared_ptr<CameraMetadata> &captureSettings)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool bRet = MetadataUtils::EncodeCameraMetadata(captureSettings, data);
    if (!bRet) {
        MEDIA_ERR_LOG("HStreamCaptureProxy::Capture EncodeCameraMetadata failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_START, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureProxy Capture failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamCaptureProxy::CancelCapture()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_CANCEL, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureProxy CancelCapture failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamCaptureProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_RELEASE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureProxy CancelCapture failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamCaptureProxy::SetCallback(sptr<IStreamCaptureCallback> &callback)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (callback == nullptr) {
        MEDIA_ERR_LOG("HStreamCaptureProxy SetCallback callback is null");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteRemoteObject(callback->AsObject())) {
        MEDIA_ERR_LOG("HStreamCaptureProxy SetCallback write StreamCaptureCallback obj failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_STREAM_CAPTURE_SET_CALLBACK, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamCaptureProxy SetCallback failed, error: %{public}d", error);
    }

    return error;
}
} // namespace CameraStandard
} // namespace OHOS
