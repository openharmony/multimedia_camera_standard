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

#include "hcamera_service_callback_stub.h"
#include "media_log.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
int HCameraServiceCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return errCode;
    }
    switch (code) {
        case CAMERA_CALLBACK_STATUS_CHANGED:
            errCode = HCameraServiceCallbackStub::HandleOnCameraStatusChanged(data);
            break;

        case CAMERA_CALLBACK_FLASHLIGHT_STATUS_CHANGED:
            errCode = HCameraServiceCallbackStub::HandleOnFlashlightStatusChanged(data);
            break;

        default:
            MEDIA_ERR_LOG("HCameraServiceCallbackStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HCameraServiceCallbackStub::HandleOnCameraStatusChanged(MessageParcel& data)
{
    std::string cameraId = data.ReadString();
    int32_t status = data.ReadInt32();

    return OnCameraStatusChanged(cameraId, (CameraStatus)status);
}

int HCameraServiceCallbackStub::HandleOnFlashlightStatusChanged(MessageParcel& data)
{
    std::string cameraId = data.ReadString();
    int32_t status = data.ReadInt32();

    return OnFlashlightStatusChanged(cameraId, (FlashStatus)status);
}
} // namespace CameraStandard
} // namespace OHOS
