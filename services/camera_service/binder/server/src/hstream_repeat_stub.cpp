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

#include "hstream_repeat_stub.h"
#include "camera_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
int HStreamRepeatStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return errCode;
    }
    switch (code) {
        case CAMERA_START_VIDEO_RECORDING:
            errCode = Start();
            break;
        case CAMERA_STOP_VIDEO_RECORDING:
            errCode = Stop();
            break;
        case CAMERA_STREAM_REPEAT_SET_FPS: {
            float fps = data.ReadFloat();
            errCode = SetFps(fps);
            break;
        }
        case CAMERA_STREAM_REPEAT_SET_CALLBACK:
            errCode = HStreamRepeatStub::HandleSetCallback(data);
            break;
        case CAMERA_STREAM_REPEAT_RELEASE:
            errCode = Release();
            break;
        default:
            MEDIA_ERR_LOG("HStreamRepeatStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HStreamRepeatStub::HandleSetCallback(MessageParcel &data)
{
    auto remoteObject = data.ReadRemoteObject();
    if (remoteObject == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeatStub HandleSetCallback StreamRepeatCallback is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto callback = iface_cast<IStreamRepeatCallback>(remoteObject);

    return SetCallback(callback);
}
} // namespace CameraStandard
} // namespace OHOS
