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

#ifndef OHOS_CAMERA_H_CAMERA_SERVICE_H
#define OHOS_CAMERA_H_CAMERA_SERVICE_H

#include "hcamera_device.h"
#include "hcamera_host_manager.h"
#include "hcamera_service_stub.h"
#include "hcapture_session.h"
#include "hstream_capture.h"
#include "hstream_metadata.h"
#include "hstream_repeat.h"
#include "iremote_stub.h"
#include "system_ability.h"

#include <iostream>

namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Camera::V1_0;
class   HCameraService : public SystemAbility, public HCameraServiceStub, public HCameraHostManager::StatusCallback {
    DECLARE_SYSTEM_ABILITY(HCameraService);

public:
    DISALLOW_COPY_AND_MOVE(HCameraService);

    explicit HCameraService(int32_t systemAbilityId, bool runOnCreate = true);
    ~HCameraService() override;
    int32_t GetCameras(std::vector<std::string> &cameraIds,
        std::vector<std::shared_ptr<OHOS::Camera::CameraMetadata>> &cameraAbilityList) override;
    int32_t CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService> &device) override;
    int32_t CreateCaptureSession(sptr<ICaptureSession> &session) override;
    int32_t CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                              sptr<IStreamCapture> &photoOutput) override;
    int32_t CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                sptr<IStreamRepeat> &previewOutput) override;
    int32_t CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format, int32_t width,
                                      int32_t height, sptr<IStreamRepeat> &previewOutput) override;
    int32_t CreateMetadataOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                 sptr<IStreamMetadata> &metadataOutput) override;
    int32_t CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                              sptr<IStreamRepeat> &videoOutput) override;
    int32_t SetCallback(sptr<ICameraServiceCallback> &callback) override;
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    int32_t Dump(int fd, const std::vector<std::u16string>& args) override;

    // HCameraHostManager::StatusCallback
    void OnCameraStatus(const std::string& cameraId, CameraStatus status) override;
    void OnFlashlightStatus(const std::string& cameraId, FlashStatus status) override;

protected:
    HCameraService(sptr<HCameraHostManager> cameraHostManager) : cameraHostManager_(cameraHostManager) {}

private:
    void CameraSummary(std::vector<std::string> cameraIds,
        std::string& dumpString);
    void CameraDumpAbility(common_metadata_header_t *metadataEntry,
    std::string& dumpString);
    void CameraDumpStreaminfo(common_metadata_header_t *metadataEntry,
    std::string& dumpString);
    void CameraDumpZoom(common_metadata_header_t *metadataEntry,
    std::string& dumpString);
    void CameraDumpFlash(common_metadata_header_t *metadataEntry,
    std::string& dumpString);
    void CameraDumpAF(common_metadata_header_t *metadataEntry,
    std::string& dumpString);
    void CameraDumpAE(common_metadata_header_t *metadataEntry,
    std::string& dumpString);
    void CameraDumpSensorInfo(common_metadata_header_t *metadataEntry,
    std::string& dumpString);
    void CameraDumpVideoStabilization(common_metadata_header_t *metadataEntry,
        std::string& dumpString);
    void CameraDumpVideoFrameRateRange(common_metadata_header_t *metadataEntry,
        std::string& dumpString);

    std::mutex mutex_;
    sptr<HCameraHostManager> cameraHostManager_;
    sptr<StreamOperatorCallback> streamOperatorCallback_;
    sptr<ICameraServiceCallback> cameraServiceCallback_;
    std::map<std::string, sptr<HCameraDevice>> devices_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_CAMERA_SERVICE_H
