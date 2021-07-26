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

#ifndef OHOS_CAMERA_HCAMERA_SERVICE_STUB_H
#define OHOS_CAMERA_HCAMERA_SERVICE_STUB_H

#include "icamera_service.h"
#include "iremote_stub.h"

namespace OHOS {
namespace CameraStandard {
class HCameraServiceStub : public IRemoteStub<ICameraService> {
public:
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data,
                                MessageParcel &reply, MessageOption &option) override;

private:
    int HandleGetCameras(MessageParcel& reply);
    int HandleCreateCameraDevice(MessageParcel &data, MessageParcel &reply);
    int HandleSetCallback(MessageParcel &data);
    int HandleCreateCaptureSession(MessageParcel &reply);
    int HandleCreatePhotoOutput(MessageParcel &data, MessageParcel &reply);
    int HandleCreatePreviewOutput(MessageParcel &data, MessageParcel &reply);
    int HandleCreateVideoOutput(MessageParcel &data, MessageParcel &reply);
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HCAMERA_SERVICE_STUB_H
