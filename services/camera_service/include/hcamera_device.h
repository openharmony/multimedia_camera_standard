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

#ifndef OHOS_CAMERA_H_CAMERA_DEVICE_H
#define OHOS_CAMERA_H_CAMERA_DEVICE_H

#include "camera_device_callback_stub.h"
#include "camera_metadata_info.h"
#include "hcamera_device_stub.h"
#include "hcamera_host_manager.h"
#include "icamera_device.h"
#include "icamera_host.h"

#include <iostream>

namespace OHOS {
namespace CameraStandard {
class CameraDeviceCallback;

class HCameraDevice : public HCameraDeviceStub {
public:
    HCameraDevice(sptr<HCameraHostManager> &cameraHostManager, sptr<CameraDeviceCallback> deviceCallback,
                  std::string cameraID);
    ~HCameraDevice();

    int32_t Open() override;
    int32_t Close() override;
    int32_t Release() override;
    int32_t UpdateSetting(const std::shared_ptr<CameraMetadata> &settings) override;
    int32_t GetEnabledResults(std::vector<int32_t> &results) override;
    int32_t EnableResult(std::vector<int32_t> &results) override;
    int32_t DisableResult(std::vector<int32_t> &results) override;
    int32_t GetStreamOperator(sptr<Camera::IStreamOperatorCallback> callback,
            sptr<Camera::IStreamOperator> &streamOperator);
    sptr<Camera::IStreamOperator> GetStreamOperator();
    int32_t SetCallback(sptr<ICameraDeviceServiceCallback> &callback) override;
    int32_t OnError(const Camera::ErrorType type, const int32_t errorMsg);
    int32_t OnResult(const uint64_t timestamp, const std::shared_ptr<CameraStandard::CameraMetadata> &result);
    std::shared_ptr<CameraMetadata> GetSettings();
    std::string GetCameraId();
    bool IsReleaseCameraDevice();
    int32_t SetReleaseCameraDevice(bool isRelease);

private:
    sptr<Camera::ICameraDevice> hdiCameraDevice_;
    sptr<HCameraHostManager> cameraHostManager_;
    std::string cameraID_;
    bool isReleaseCameraDevice_;
    sptr<ICameraDeviceServiceCallback> deviceSvcCallback_;
    sptr<CameraDeviceCallback> deviceHDICallback_;
    std::shared_ptr<CameraMetadata> updateSettings_;
    sptr<Camera::IStreamOperator> streamOperator_;
};

class CameraDeviceCallback : public Camera::CameraDeviceCallbackStub {
public:
    CameraDeviceCallback() = default;
    CameraDeviceCallback(sptr<HCameraDevice> hCameraDevice);
    virtual ~CameraDeviceCallback() = default;
    virtual void OnError(Camera::ErrorType type, int32_t errorMsg) override;
    virtual void OnResult(const uint64_t timestamp,
                          const std::shared_ptr<CameraStandard::CameraMetadata> &result) override;
    void SetHCameraDevice(sptr<HCameraDevice> hcameraDevice);

private:
    sptr<HCameraDevice> hCameraDevice_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_CAMERA_DEVICE_H
