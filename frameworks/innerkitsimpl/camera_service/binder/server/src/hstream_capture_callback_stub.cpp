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

#include "hstream_capture_callback_stub.h"
#include "media_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
int HStreamCaptureCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = ERR_NONE;

    switch (code) {
        case CAMERA_STREAM_CAPTURE_ON_CAPTURE_STARTED:
            errCode = OnCaptureStarted();
            break;
        case CAMERA_STREAM_CAPTURE_ON_CAPTURE_ENDED:
            errCode = OnCaptureEnded();
            break;
        case CAMERA_STREAM_CAPTURE_ON_CAPTURE_ERROR:
            errCode = HStreamCaptureCallbackStub::HandleOnCaptureError(data);
            break;
        case CAMERA_STREAM_CAPTURE_ON_FRAME_SHUTTER:
            errCode = HStreamCaptureCallbackStub::HandleOnFrameShutter(data);
            break;
        default:
            MEDIA_ERR_LOG("HStreamCaptureCallbackStub request code %{public}d not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HStreamCaptureCallbackStub::HandleOnCaptureError(MessageParcel& data)
{
    int32_t errorType = data.ReadInt32();

    return OnCaptureError(errorType);
}

int HStreamCaptureCallbackStub::HandleOnFrameShutter(MessageParcel& data)
{
    uint64_t timestamp = data.ReadUint64();

    return OnFrameShutter(timestamp);
}
} // namespace CameraStandard
} // namespace OHOS