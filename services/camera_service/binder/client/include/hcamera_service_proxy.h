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

#ifndef OHOS_CAMERA_HCAMERA_SERVICE_PROXY_H
#define OHOS_CAMERA_HCAMERA_SERVICE_PROXY_H

#include "icamera_service.h"
#include "icamera_service_callback.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace CameraStandard {
class HCameraServiceProxy : public IRemoteProxy<ICameraService> {
public:
    explicit HCameraServiceProxy(const sptr<IRemoteObject> &impl);

    virtual ~HCameraServiceProxy() = default;

    int32_t CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService>& device) override;

    int32_t SetCallback(sptr<ICameraServiceCallback>& callback) override;

    int32_t GetCameras(std::vector<std::string> &cameraIds,
        std::vector<std::shared_ptr<CameraMetadata>> &cameraAbilityList) override;

    int32_t CreateCaptureSession(sptr<ICaptureSession>& session) override;

    int32_t CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, sptr<IStreamCapture>& photoOutput) override;

    int32_t CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer,
                                sptr<IStreamRepeat>& previewOutput) override;

    int32_t CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t width, int32_t height,
                                sptr<IStreamRepeat>& previewOutput) override;

    int32_t CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, sptr<IStreamRepeat>& videoOutput) override;

private:
    static inline BrokerDelegator<HCameraServiceProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HCAMERA_SERVICE_PROXY_H
