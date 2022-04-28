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

#include "hcamera_device_callback_stub.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
int HCameraDeviceCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return errCode;
    }
    switch (code) {
        case CAMERA_DEVICE_ON_ERROR:
            errCode = HCameraDeviceCallbackStub::HandleDeviceOnError(data);
            break;
        case CAMERA_DEVICE_ON_RESULT:
            errCode = HCameraDeviceCallbackStub::HandleDeviceOnResult(data);
            break;
        default:
            MEDIA_ERR_LOG("HCameraDeviceCallbackStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }
    return errCode;
}

int HCameraDeviceCallbackStub::HandleDeviceOnError(MessageParcel& data)
{
    int32_t errorType = 0;
    int32_t errorMsg = 0;

    errorType = data.ReadInt32();
    errorMsg = data.ReadInt32();
    return OnError(errorType, errorMsg);
}

int HCameraDeviceCallbackStub::HandleDeviceOnResult(MessageParcel& data)
{
    std::shared_ptr<Camera::CameraMetadata> metadata = nullptr;
    uint64_t timestamp = 0;

    timestamp = data.ReadUint64();
    Camera::MetadataUtils::DecodeCameraMetadata(data, metadata);
    return OnResult(timestamp, metadata);
}
} // namespace CameraStandard
} // namespace OHOS
