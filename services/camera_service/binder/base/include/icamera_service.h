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

#ifndef OHOS_CAMERA_ICAMERA_SERVICE_H
#define OHOS_CAMERA_ICAMERA_SERVICE_H

#include "camera_metadata_info.h"
#include "icamera_service_callback.h"
#include "icamera_device_service.h"
#include "icapture_session.h"
#include "iremote_broker.h"
#include "istream_capture.h"
#include "istream_repeat.h"
#include "surface.h"

namespace OHOS {
namespace CameraStandard {
class ICameraService : public IRemoteBroker {
public:
    virtual int32_t CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService>& device) = 0;

    virtual int32_t SetCallback(sptr<ICameraServiceCallback>& callback) = 0;

    virtual int32_t GetCameras(std::vector<std::string> &cameraIds,
        std::vector<std::shared_ptr<CameraMetadata>> &cameraAbilityList) = 0;

    virtual int32_t CreateCaptureSession(sptr<ICaptureSession> &session) = 0;

    virtual int32_t CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer,
                                      sptr<IStreamCapture> &photoOutput) = 0;

    virtual int32_t CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer,
                                        sptr<IStreamRepeat> &previewOutput) = 0;

    virtual int32_t CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t width,
                                              int32_t height, sptr<IStreamRepeat> &previewOutput) = 0;

    virtual int32_t CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer,
                                      sptr<IStreamRepeat> &videoOutput) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ICameraService");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ICAMERA_SERVICE_H
