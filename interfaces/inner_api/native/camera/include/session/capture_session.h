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

#ifndef OHOS_CAMERA_CAPTURE_SESSION_H
#define OHOS_CAMERA_CAPTURE_SESSION_H

#include <iostream>
#include <vector>
#include "input/capture_input.h"
#include "output/capture_output.h"
#include "icapture_session.h"
#include "icapture_session_callback.h"

namespace OHOS {
namespace CameraStandard {
class SessionCallback {
public:
    SessionCallback() = default;
    virtual ~SessionCallback() = default;
    virtual void OnError(int32_t errorCode) = 0;
};

class CaptureSession : public RefBase {
public:
    CaptureSession(sptr<ICaptureSession> &captureSession);
    ~CaptureSession() {};
    int32_t BeginConfig();
    int32_t CommitConfig();
    int32_t AddInput(sptr<CaptureInput> &input);
    int32_t AddOutput(sptr<CaptureOutput> &output);
    int32_t RemoveInput(sptr<CaptureInput> &input);
    int32_t RemoveOutput(sptr<CaptureOutput> &output);
    int32_t Start();
    int32_t Stop();
    void SetCallback(std::shared_ptr<SessionCallback> callback);
    std::shared_ptr<SessionCallback> GetApplicationCallback();
    void Release();

private:
    sptr<ICaptureSession> captureSession_;
    std::shared_ptr<SessionCallback> appCallback_;
    sptr<ICaptureSessionCallback> captureSessionCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAPTURE_SESSION_H
