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

#include "hstream_capture_stub.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
int HStreamCaptureStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return errCode;
    }
    switch (code) {
        case CAMERA_STREAM_CAPTURE_START:
            errCode = HStreamCaptureStub::HandleCapture(data);
            break;
        case CAMERA_STREAM_CAPTURE_CANCEL:
            errCode = CancelCapture();
            break;
        case CAMERA_STREAM_CAPTURE_SET_CALLBACK:
            errCode = HStreamCaptureStub::HandleSetCallback(data);
            break;
        case CAMERA_STREAM_CAPTURE_RELEASE:
            errCode = Release();
            break;
        default:
            MEDIA_ERR_LOG("HStreamCaptureStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HStreamCaptureStub::HandleCapture(MessageParcel &data)
{
    std::shared_ptr<CameraStandard::CameraMetadata> metadata = nullptr;
    MetadataUtils::DecodeCameraMetadata(data, metadata);

    return Capture(metadata);
}

int HStreamCaptureStub::HandleSetCallback(MessageParcel &data)
{
    auto remoteObject = data.ReadRemoteObject();
    if (remoteObject == nullptr) {
        MEDIA_ERR_LOG("HStreamCaptureStub HandleSetCallback StreamCaptureCallback is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto callback = iface_cast<IStreamCaptureCallback>(remoteObject);

    return SetCallback(callback);
}
} // namespace CameraStandard
} // namespace OHOS
