/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_ICAPTURE_SESSION_H
#define OHOS_CAMERA_ICAPTURE_SESSION_H

#include "icamera_device_service.h"
#include "icapture_session_callback.h"
#include "iremote_broker.h"
#include "istream_repeat.h"
#include "istream_capture.h"

namespace OHOS {
namespace CameraStandard {
class ICaptureSession : public IRemoteBroker {
public:
    virtual int32_t BeginConfig() = 0;

    virtual int32_t AddInput(sptr<ICameraDeviceService> cameraDevice) = 0;

    virtual int32_t AddOutput(sptr<IStreamRepeat> streamRepeat) = 0;

    virtual int32_t AddOutput(sptr<IStreamCapture> streamCapture) = 0;

    virtual int32_t RemoveInput(sptr<ICameraDeviceService> cameraDevice) = 0;

    virtual int32_t RemoveOutput(sptr<IStreamCapture> streamCapture) = 0;

    virtual int32_t RemoveOutput(sptr<IStreamRepeat> streamRepeat) = 0;

    virtual int32_t CommitConfig() = 0;

    virtual int32_t Start() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t Release(pid_t pid) = 0;

    virtual int32_t SetCallback(sptr<ICaptureSessionCallback> &callback) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ICaptureSession");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ICAPTURE_SESSION_H
