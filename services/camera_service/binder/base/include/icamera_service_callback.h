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

#ifndef OHOS_CAMERA_ICAMERA_SERVICE_CALLBACK_H
#define OHOS_CAMERA_ICAMERA_SERVICE_CALLBACK_H

#include "iremote_broker.h"

namespace OHOS {
namespace CameraStandard {
enum CameraStatus {
    CAMERA_STATUS_UNAVAILABLE = 0,
    CAMERA_STATUS_AVAILABLE
};

enum FlashStatus {
    FLASH_STATUS_OFF = 0,
    FLASH_STATUS_ON,
    FLASH_STATUS_UNAVAILABLE
};

class ICameraServiceCallback : public IRemoteBroker {
public:
    virtual int32_t OnCameraStatusChanged(const std::string cameraId, const CameraStatus status) = 0;
    virtual int32_t OnFlashlightStatusChanged(const std::string cameraId, const FlashStatus status) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ICameraServiceCallback");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ICAMERA_SERVICE_CALLBACK_H