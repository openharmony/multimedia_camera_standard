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

#ifndef OHOS_CAMERA_CAMERA_MANAGER_H
#define OHOS_CAMERA_CAMERA_MANAGER_H

#include <iostream>
#include <refbase.h>
#include <vector>
#include "input/camera_input.h"
#include "input/camera_info.h"
#include "hcamera_service_proxy.h"
#include "icamera_device_service.h"
#include "session/capture_session.h"
#include "output/photo_output.h"
#include "output/video_output.h"
#include "output/preview_output.h"
#include "hcamera_service_callback_stub.h"

namespace OHOS {
namespace CameraStandard {
enum CameraDeviceStatus {
    CAMERA_DEVICE_STATUS_UNAVAILABLE = 0,
    CAMERA_DEVICE_STATUS_AVAILABLE
};

enum FlashlightStatus {
    FLASHLIGHT_STATUS_OFF = 0,
    FLASHLIGHT_STATUS_ON,
    FLASHLIGHT_STATUS_UNAVAILABLE
};

class CameraManagerCallback {
public:
    CameraManagerCallback() = default;
    virtual ~CameraManagerCallback() = default;
    virtual void OnCameraStatusChanged(const std::string &cameraID, const CameraDeviceStatus cameraStatus) const = 0;
    virtual void OnFlashlightStatusChanged(const std::string &cameraID, const FlashlightStatus flashStatus) const = 0;
};

class CameraManager : public RefBase {
public:
    static sptr<CameraManager> &GetInstance();
    std::vector<sptr<CameraInfo>> GetCameras();
    sptr<CameraInput> CreateCameraInput(sptr<CameraInfo> &camera);
    sptr<CaptureSession> CreateCaptureSession();
    sptr<PhotoOutput> CreatePhotoOutput(sptr<Surface> &surface);
    sptr<VideoOutput> CreateVideoOutput(sptr<Surface> &surface);
    sptr<PreviewOutput> CreatePreviewOutput(sptr<Surface> surface);
    sptr<PreviewOutput> CreateCustomPreviewOutput(sptr<Surface> surface, int32_t width, int32_t height);
    void SetCallback(std::shared_ptr<CameraManagerCallback> callback);
    std::shared_ptr<CameraManagerCallback> GetApplicationCallback();

private:
    CameraManager();
    void Init();
    void SetCameraServiceCallback(sptr<ICameraServiceCallback>& callback);
    sptr<ICameraDeviceService> CreateCameraDevice(std::string cameraId);
    sptr<ICameraService> serviceProxy_;
    static sptr<CameraManager> cameraManager_;
    sptr<ICameraServiceCallback> cameraSvcCallback_;
    std::shared_ptr<CameraManagerCallback> cameraMngrCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_MANAGER_H
