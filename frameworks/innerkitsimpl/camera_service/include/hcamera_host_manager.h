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

#ifndef OHOS_CAMERA_H_CAMERA_HOST_MANAGER_H
#define OHOS_CAMERA_H_CAMERA_HOST_MANAGER_H

#include "camera_device.h"
#include "camera_metadata_info.h"
#include "icamera_host.h"

#include <refbase.h>
#include <iostream>

namespace OHOS {
namespace CameraStandard {
class HCameraHostManager : public RefBase {
public:
    HCameraHostManager();
    ~HCameraHostManager();

    int32_t Init(void);
    int32_t GetCameras(std::vector<std::string> &cameraIds);
    int32_t GetCameraAbility(std::string &cameraId,
                             std::shared_ptr<CameraMetadata> &ability);
    int32_t OpenCameraDevice(std::string &cameraId,
                             const sptr<Camera::ICameraDeviceCallback> &callback,
                             sptr<Camera::ICameraDevice> &pDevice);
    int32_t SetFlashlight(std::string &cameraId, bool &isEnable);
    int32_t SetCallback(sptr<Camera::ICameraHostCallback> &callback);

private:
    sptr<Camera::ICameraHost> GetICameraHost();
    sptr<Camera::ICameraHost> cameraHostService_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_CAMERA_HOST_MANAGER_H
