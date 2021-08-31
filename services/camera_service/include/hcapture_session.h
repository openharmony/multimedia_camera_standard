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

#ifndef OHOS_CAMERA_H_CAPTURE_SESSION_H
#define OHOS_CAMERA_H_CAPTURE_SESSION_H

#include "hcamera_device.h"
#include "hcapture_session_stub.h"
#include "hstream_capture.h"
#include "hstream_repeat.h"
#include "stream_operator_callback_stub.h"

#include <refbase.h>
#include <iostream>

namespace OHOS {
namespace CameraStandard {
class StreamOperatorCallback;

class HCaptureSession : public HCaptureSessionStub {
public:
    HCaptureSession(sptr<HCameraHostManager> cameraHostManager, sptr<StreamOperatorCallback> streamOperatorCallback);
    ~HCaptureSession();

    int32_t BeginConfig() override;
    int32_t CommitConfig() override;

    int32_t AddInput(sptr<ICameraDeviceService> cameraDevice) override;
    int32_t AddOutput(sptr<IStreamRepeat> streamRepeat) override;
    int32_t AddOutput(sptr<IStreamCapture> streamCapture) override;

    int32_t RemoveInput() override;
    int32_t RemoveOutput(sptr<IStreamCapture> streamCapture) override;
    int32_t RemoveOutput(sptr<IStreamRepeat> streamRepeat) override;

    int32_t Start() override;
    int32_t Stop() override;
    int32_t Release() override;

    friend class StreamOperatorCallback;

private:
    sptr<Camera::IStreamOperator> streamOperator_;
    std::vector<std::shared_ptr<Camera::StreamInfo>> streamInfosPreview_;
    sptr<HStreamRepeat> streamRepeatPreview_;
    sptr<HStreamRepeat> streamRepeatVideo_;
    sptr<HStreamCapture> streamCapture_;
    sptr<HCameraDevice> cameraDevice_;
    int32_t previewStreamID_, photoStreamID_, videoStreamID_;
    std::shared_ptr<CameraMetadata> cameraAbility_;
    sptr<HCameraHostManager> cameraHostManager_;
    std::vector<int32_t> streamIds_;
    sptr<StreamOperatorCallback> streamOperatorCallback_;
};

class StreamOperatorCallback : public Camera::StreamOperatorCallbackStub {
public:
    StreamOperatorCallback() = default;
    StreamOperatorCallback(sptr<HCaptureSession> session);
    virtual ~StreamOperatorCallback() = default;

    virtual void OnCaptureStarted(int32_t captureId, const std::vector<int32_t> &streamId) override;
    virtual void OnCaptureEnded(int32_t captureId,
                                const std::vector<std::shared_ptr<Camera::CaptureEndedInfo>> &info) override;
    virtual void OnCaptureError(int32_t captureId,
                                const std::vector<std::shared_ptr<Camera::CaptureErrorInfo>> &info) override;
    virtual void OnFrameShutter(int32_t captureId,
                                const std::vector<int32_t> &streamId, uint64_t timestamp) override;
    void SetCaptureSession(sptr<HCaptureSession> captureSession);

private:
    sptr<HCaptureSession> captureSession_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_CAPTURE_SESSION_H
