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

enum class CaptureSessionState {
    SESSION_INIT = 0,
    SESSION_CONFIG_INPROGRESS,
    SESSION_CONFIG_COMMITTED,
};

static const int32_t STREAMID_BEGIN = 1;

class HCaptureSession : public HCaptureSessionStub {
public:
    HCaptureSession(sptr<HCameraHostManager> cameraHostManager, sptr<StreamOperatorCallback> streamOperatorCb);
    ~HCaptureSession();

    int32_t BeginConfig() override;
    int32_t CommitConfig() override;

    int32_t AddInput(sptr<ICameraDeviceService> cameraDevice) override;
    int32_t AddOutput(sptr<IStreamRepeat> streamRepeat) override;
    int32_t AddOutput(sptr<IStreamCapture> streamCapture) override;

    int32_t RemoveInput(sptr<ICameraDeviceService> cameraDevice) override;
    int32_t RemoveOutput(sptr<IStreamCapture> streamCapture) override;
    int32_t RemoveOutput(sptr<IStreamRepeat> streamRepeat) override;

    int32_t Start() override;
    int32_t Stop() override;
    int32_t Release(pid_t pid) override;
    static void DestroyStubObjectForPid(pid_t pid);
    int32_t SetCallback(sptr<ICaptureSessionCallback> &callback) override;

    friend class StreamOperatorCallback;
    static void dumpSessions(std::string& dumpString);
    void dumpSessionInfo(std::string& dumpString);
    static void CameraSessionSummary(std::string& dumpString);

private:
    int32_t ValidateSessionInputs();
    int32_t ValidateSessionOutputs();
    int32_t GetCameraDevice(sptr<HCameraDevice> &device);
    int32_t HandleCaptureOuputsConfig(sptr<HCameraDevice> &device);
    int32_t CreateAndCommitStreams(sptr<HCameraDevice> &device, std::shared_ptr<Camera::CameraMetadata> &deviceSettings,
                                   std::vector<std::shared_ptr<Camera::StreamInfo>> &streamInfos);
    int32_t CheckAndCommitStreams(sptr<HCameraDevice> &device, std::shared_ptr<Camera::CameraMetadata> &deviceSettings,
                                  std::vector<std::shared_ptr<Camera::StreamInfo>> &allStreamInfos,
                                  std::vector<std::shared_ptr<Camera::StreamInfo>> &newStreamInfos);
    int32_t GetCurrentStreamInfos(sptr<HCameraDevice> &device, std::shared_ptr<Camera::CameraMetadata> &deviceSettings,
                                  std::vector<std::shared_ptr<Camera::StreamInfo>> &streamInfos);
    void UpdateSessionConfig(sptr<HCameraDevice> &device);
    void DeleteReleasedStream();
    void RestorePreviousState(sptr<HCameraDevice> &device, bool isCreateReleaseStreams);
    void ReleaseStreams();
    void ClearCaptureSession(pid_t pid);
    std::string GetSessionState();

    std::mutex mutex_;
    CaptureSessionState curState_ = CaptureSessionState::SESSION_INIT;
    CaptureSessionState prevState_ = CaptureSessionState::SESSION_INIT;
    sptr<HCameraDevice> cameraDevice_;
    std::vector<sptr<HStreamRepeat>> streamRepeats_;
    std::vector<sptr<HStreamCapture>> streamCaptures_;
    std::vector<sptr<HCameraDevice>> cameraDevices_;
    std::vector<sptr<HStreamRepeat>> tempStreamRepeats_;
    std::vector<sptr<HStreamCapture>> tempStreamCaptures_;
    std::vector<sptr<HCameraDevice>> tempCameraDevices_;
    std::vector<int32_t> deletedStreamIds_;
    sptr<HCameraHostManager> cameraHostManager_;
    sptr<StreamOperatorCallback> streamOperatorCallback_;
    sptr<ICaptureSessionCallback> sessionCallback_;
    int32_t streamId_ = STREAMID_BEGIN;
    std::map<CaptureSessionState, std::string> sessionState_;
    pid_t pid_;
    int32_t uid_;
};

class StreamOperatorCallback : public Camera::StreamOperatorCallbackStub {
public:
    StreamOperatorCallback() = default;
    explicit StreamOperatorCallback(sptr<HCaptureSession> session);
    virtual ~StreamOperatorCallback() = default;

    void OnCaptureStarted(int32_t captureId, const std::vector<int32_t> &streamId) override;
    void OnCaptureEnded(int32_t captureId,
                        const std::vector<std::shared_ptr<Camera::CaptureEndedInfo>> &info) override;
    void OnCaptureError(int32_t captureId,
                        const std::vector<std::shared_ptr<Camera::CaptureErrorInfo>> &info) override;
    void OnFrameShutter(int32_t captureId,
                        const std::vector<int32_t> &streamId, uint64_t timestamp) override;
    void SetCaptureSession(sptr<HCaptureSession> captureSession);

private:
    sptr<HStreamCapture> GetStreamCaptureByStreamID(std::int32_t streamId);
    sptr<HStreamRepeat> GetStreamRepeatByStreamID(std::int32_t streamId);
    sptr<HCaptureSession> captureSession_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_CAPTURE_SESSION_H
