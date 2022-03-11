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
#include "hcamera_listener_stub.h"
#include "input/camera_death_recipient.h"
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

struct CameraStatusInfo {
    sptr<CameraInfo> cameraInfo;
    CameraDeviceStatus cameraStatus;
};

class CameraManagerCallback {
public:
    CameraManagerCallback() = default;
    virtual ~CameraManagerCallback() = default;
    virtual void OnCameraStatusChanged(const CameraStatusInfo &cameraStatusInfo) const = 0;
    virtual void OnFlashlightStatusChanged(const std::string &cameraID, const FlashlightStatus flashStatus) const = 0;
};

class CameraManager : public RefBase {
public:
    static sptr<CameraManager> &GetInstance();
    std::vector<sptr<CameraInfo>> GetCameras();
    sptr<CameraInput> CreateCameraInput(sptr<CameraInfo> &camera);
    sptr<CaptureSession> CreateCaptureSession();
    sptr<PhotoOutput> CreatePhotoOutput(sptr<Surface> &surface);
    sptr<PhotoOutput> CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format);
    sptr<VideoOutput> CreateVideoOutput(sptr<Surface> &surface);
    sptr<VideoOutput> CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format);
    sptr<PreviewOutput> CreatePreviewOutput(sptr<Surface> surface);
    sptr<PreviewOutput> CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format);
    sptr<PreviewOutput> CreateCustomPreviewOutput(sptr<Surface> surface, int32_t width, int32_t height);
    sptr<PreviewOutput> CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                                  int32_t width, int32_t height);
    void SetCallback(std::shared_ptr<CameraManagerCallback> callback);
    std::shared_ptr<CameraManagerCallback> GetApplicationCallback();
    sptr<CameraInfo> GetCameraInfo(std::string cameraId);

    static const std::string surfaceFormat;
    void SetPermissionCheck(bool Enable);

protected:
    CameraManager(sptr<ICameraService> serviceProxy) : serviceProxy_(serviceProxy) {}

private:
    CameraManager();
    void Init();
    void SetCameraServiceCallback(sptr<ICameraServiceCallback>& callback);
    int32_t CreateListenerObject();
    void CameraServerDied(pid_t pid);

    sptr<ICameraDeviceService> CreateCameraDevice(std::string cameraId);
    sptr<ICameraService> serviceProxy_;
    sptr<CameraListenerStub> listenerStub_ = nullptr;
    sptr<CameraDeathRecipient> deathRecipient_ = nullptr;
    static sptr<CameraManager> cameraManager_;
    sptr<ICameraServiceCallback> cameraSvcCallback_;
    std::shared_ptr<CameraManagerCallback> cameraMngrCallback_;
    std::vector<sptr<CameraInfo>> cameraObjList;
    bool permissionCheckEnabled_ = false;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_MANAGER_H
