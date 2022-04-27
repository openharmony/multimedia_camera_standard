/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_ISTREAM_CAPTURE_H
#define OHOS_CAMERA_ISTREAM_CAPTURE_H

#include "camera_metadata_info.h"
#include "iremote_broker.h"
#include "istream_capture_callback.h"

namespace OHOS {
namespace CameraStandard {
class IStreamCapture : public IRemoteBroker {
public:
    virtual int32_t Capture(const std::shared_ptr<Camera::CameraMetadata> &captureSettings) = 0;

    virtual int32_t CancelCapture() = 0;

    virtual int32_t SetCallback(sptr<IStreamCaptureCallback> &callback) = 0;

    virtual int32_t Release() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamCapture");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_CAPTURE_H
