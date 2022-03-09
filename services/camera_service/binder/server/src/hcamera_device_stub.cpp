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

#include "hcamera_device_stub.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
int HCameraDeviceStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return errCode;
    }
    switch (code) {
        case CAMERA_DEVICE_OPEN: {
            errCode = Open();
            break;
        }
        case CAMERA_DEVICE_CLOSE:
            errCode = Close();
            break;
        case CAMERA_DEVICE_RELEASE:
            errCode = Release();
            break;
        case CAMERA_DEVICE_SET_CALLBACK:
            errCode = HCameraDeviceStub::HandleSetCallback(data);
            break;
        case CAMERA_DEVICE_UPDATE_SETTNGS:
            errCode = HCameraDeviceStub::HandleUpdateSetting(data);
            break;
        case CAMERA_DEVICE_ENABLED_RESULT:
            errCode = HCameraDeviceStub::HandleEnableResult(data);
            break;
        case CAMERA_DEVICE_GET_ENABLED_RESULT:
            errCode = HCameraDeviceStub::HandleGetEnabledResults(reply);
            break;
        case CAMERA_DEVICE_DISABLED_RESULT:
            errCode = HCameraDeviceStub::HandleDisableResult(data);
            break;
        default:
            MEDIA_ERR_LOG("HCameraDeviceStub request code %{public}d not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HCameraDeviceStub::HandleSetCallback(MessageParcel &data)
{
    auto remoteObject = data.ReadRemoteObject();
    if (remoteObject == nullptr) {
        MEDIA_ERR_LOG("HCameraDeviceStub HandleSetCallback CameraDeviceServiceCallback is null");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto callback = iface_cast<ICameraDeviceServiceCallback>(remoteObject);

    return SetCallback(callback);
}

int HCameraDeviceStub::HandleUpdateSetting(MessageParcel &data)
{
    std::shared_ptr<CameraStandard::CameraMetadata> metadata = nullptr;
    MetadataUtils::DecodeCameraMetadata(data, metadata);

    return UpdateSetting(metadata);
}

int HCameraDeviceStub::HandleGetEnabledResults(MessageParcel &reply)
{
    std::vector<int32_t> results;
    int ret = GetEnabledResults(results);
    if (ret != ERR_NONE) {
        MEDIA_ERR_LOG("CameraDeviceStub::HandleGetEnabledResults GetEnabledResults failed : %{public}d", ret);
        return ret;
    }

    if (!reply.WriteInt32Vector(results)) {
        MEDIA_ERR_LOG("HCameraDeviceStub::HandleGetEnabledResults write results failed");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ret;
}

int HCameraDeviceStub::HandleEnableResult(MessageParcel &data)
{
    std::vector<int32_t> results;
    if (!data.ReadInt32Vector(&results)) {
        MEDIA_ERR_LOG("CameraDeviceStub::HandleEnableResult read results failed");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    int ret = EnableResult(results);
    if (ret != ERR_NONE) {
        MEDIA_ERR_LOG("CameraDeviceStub::HandleEnableResult EnableResult failed : %{public}d", ret);
    }

    return ret;
}

int HCameraDeviceStub::HandleDisableResult(MessageParcel &data)
{
    std::vector<int32_t> results;
    if (!data.ReadInt32Vector(&results)) {
        MEDIA_ERR_LOG("CameraDeviceStub::HandleDisableResult read results failed");
        return IPC_STUB_INVALID_DATA_ERR;
    }

    int ret = DisableResult(results);
    if (ret != ERR_NONE) {
        MEDIA_ERR_LOG("CameraDeviceStub::HandleDisableResult DisableResult failed : %{public}d", ret);
    }

    return ret;
}
} // namespace CameraStandard
} // namespace OHOS
