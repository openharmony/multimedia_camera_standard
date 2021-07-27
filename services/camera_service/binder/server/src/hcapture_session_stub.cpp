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

#include "hcapture_session_stub.h"
#include "media_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
int HCaptureSessionStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = ERR_NONE;

    switch (code) {
        case CAMERA_CAPTURE_SESSION_BEGIN_CONFIG:
            errCode = BeginConfig();
            break;
        case CAMERA_CAPTURE_SESSION_ADD_INPUT:
            errCode = HCaptureSessionStub::HandleAddInput(data);
            break;
        case CAMERA_CAPTURE_SESSION_ADD_OUTPUT_CAPTURE:
            errCode = HCaptureSessionStub::HandleAddCaptureOutput(data);
            break;
        case CAMERA_CAPTURE_SESSION_ADD_OUTPUT_REPEAT:
            errCode = HCaptureSessionStub::HandleAddRepeatOutput(data);
            break;
        case CAMERA_CAPTURE_SESSION_REMOVE_INPUT:
            errCode = RemoveInput();
            break;
        case CAMERA_CAPTURE_SESSION_REMOVE_OUTPUT_CAPTURE:
            errCode = HCaptureSessionStub::HandleRemoveCaptureOutput(data);
            break;
        case CAMERA_CAPTURE_SESSION_REMOVE_OUTPUT_REPEAT:
            errCode = HCaptureSessionStub::HandleRemoveRepeatOutput(data);
            break;
        case CAMERA_CAPTURE_SESSION_COMMIT_CONFIG:
            errCode = CommitConfig();
            break;
        case CAMERA_CAPTURE_SESSION_START:
            errCode = Start();
            break;
        case CAMERA_CAPTURE_SESSION_STOP:
            errCode = Stop();
            break;
        case CAMERA_CAPTURE_SESSION_RELEASE:
            errCode = Release();
            break;
        default:
            MEDIA_ERR_LOG("HCaptureSessionStub request code %{public}d not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HCaptureSessionStub::HandleAddInput(MessageParcel &data)
{
    sptr<IRemoteObject> remoteObj = data.ReadRemoteObject();
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("HCaptureSessionStub HandleAddInput CameraDevice is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    sptr<ICameraDeviceService> cameraDevice = iface_cast<ICameraDeviceService>(remoteObj);

    return AddInput(cameraDevice);
}

int HCaptureSessionStub::HandleAddCaptureOutput(MessageParcel &data)
{
    sptr<IRemoteObject> remoteObj = data.ReadRemoteObject();
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("HCaptureSessionStub HandleAddCaptureOutput StreamCapture is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    sptr<IStreamCapture> streamCapture = iface_cast<IStreamCapture>(remoteObj);

    return AddOutput(streamCapture);
}

int HCaptureSessionStub::HandleAddRepeatOutput(MessageParcel &data)
{
    sptr<IRemoteObject> remoteObj = data.ReadRemoteObject();
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("HCaptureSessionStub HandleAddRepeatOutput streamRepeat is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    sptr<IStreamRepeat> streamRepeat = iface_cast<IStreamRepeat>(remoteObj);

    return AddOutput(streamRepeat);
}

int HCaptureSessionStub::HandleRemoveCaptureOutput(MessageParcel &data)
{
    sptr<IRemoteObject> remoteObj = data.ReadRemoteObject();
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("HCaptureSessionStub HandleRemoveCaptureOutput StreamCapture is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    sptr<IStreamCapture> streamCapture = iface_cast<IStreamCapture>(remoteObj);

    return RemoveOutput(streamCapture);
}

int HCaptureSessionStub::HandleRemoveRepeatOutput(MessageParcel &data)
{
    sptr<IRemoteObject> remoteObj = data.ReadRemoteObject();
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("HCaptureSessionStub HandleRemoveRepeatOutput streamRepeat is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    sptr<IStreamRepeat> streamRepeat = iface_cast<IStreamRepeat>(remoteObj);

    return RemoveOutput(streamRepeat);
}
} // namespace CameraStandard
} // namespace OHOS
