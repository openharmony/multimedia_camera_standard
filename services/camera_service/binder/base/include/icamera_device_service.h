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

#ifndef OHOS_CAMERA_ICAMERA_DEVICE_SERVICE_H
#define OHOS_CAMERA_ICAMERA_DEVICE_SERVICE_H

#include "camera_metadata_info.h"
#include "icamera_device_service_callback.h"
#include "iremote_broker.h"

namespace OHOS {
namespace CameraStandard {
class ICameraDeviceService : public IRemoteBroker {
public:
    virtual int32_t Open() = 0;

    virtual int32_t Close() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t SetCallback(sptr<ICameraDeviceServiceCallback> &callback) = 0;

    virtual int32_t UpdateSetting(const std::shared_ptr<CameraMetadata> &settings) = 0;

    virtual int32_t GetEnabledResults(std::vector<int32_t> &results) = 0;

    virtual int32_t EnableResult(std::vector<int32_t> &results) = 0;

    virtual int32_t DisableResult(std::vector<int32_t> &results) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ICameraDeviceService");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ICAMERA_DEVICE_SERVICE_H
