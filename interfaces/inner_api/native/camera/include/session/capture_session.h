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
    /**
     * @brief Called when error occured during capture session callback.
     *
     * @param errorCode Indicates a {@link ErrorCode} which will give information for capture session callback error.
     */
    virtual void OnError(int32_t errorCode) = 0;
};

enum VideoStabilizationMode {
    OFF = 0,
    LOW,
    MIDDLE,
    HIGH,
    AUTO
};

class CaptureSession : public RefBase {
public:
    sptr<CaptureInput> inputDevice_;
    explicit CaptureSession(sptr<ICaptureSession> &captureSession);
    ~CaptureSession();

    /**
     * @brief Begin the capture session config.
     */
    int32_t BeginConfig();

    /**
     * @brief Commit the capture session config.
     */
    int32_t CommitConfig();

    /**
     * @brief Add CaptureInput for the capture session.
     *
     * @param CaptureInput to be added to session.
     */
    int32_t AddInput(sptr<CaptureInput> &input);

    /**
     * @brief Add CaptureOutput for the capture session.
     *
     * @param CaptureOutput to be added to session.
     */
    int32_t AddOutput(sptr<CaptureOutput> &output);

    /**
     * @brief Remove CaptureInput for the capture session.
     *
     * @param CaptureInput to be removed from session.
     */
    int32_t RemoveInput(sptr<CaptureInput> &input);

    /**
     * @brief Remove CaptureOutput for the capture session.
     *
     * @param CaptureOutput to be removed from session.
     */
    int32_t RemoveOutput(sptr<CaptureOutput> &output);

    /**
     * @brief Starts session and preview.
     */
    int32_t Start();

    /**
     * @brief Stop session and preview..
     */
    int32_t Stop();

    /**
     * @brief Set the session callback for the capture session.
     *
     * @param SessionCallback pointer to be triggered.
     */
    void SetCallback(std::shared_ptr<SessionCallback> callback);

    /**
     * @brief Get the application callback information.
     *
     * @return Returns the pointer to SessionCallback set by application.
     */
    std::shared_ptr<SessionCallback> GetApplicationCallback();

    /**
     * @brief Releases CaptureSession instance.
     */
    void Release();

    /**
    * @brief Get the supported video sabilization modes.
    *
    * @return Returns vector of CameraVideoStabilizationMode supported stabilization modes.
    */
    std::vector<VideoStabilizationMode> GetSupportedStabilizationMode();

    /**
    * @brief Query whether given stabilization mode supported.
    *
    * @param VideoStabilizationMode stabilization mode to query.
    * @return True is supported false otherwise.
    */
    bool IsVideoStabilizationModeSupported(VideoStabilizationMode stabilizationMode);

    /**
    * @brief Get the current Video Stabilizaion mode.
    *
    * @return Returns current Video Stabilizaion mode.
    */
    VideoStabilizationMode GetActiveVideoStabilizationMode();

    /**
    * @brief Set the Video Stabilizaion mode.
    *
    * @param VideoStabilizationMode stabilization mode to set.
    */
    void SetVideoStabilizationMode(VideoStabilizationMode stabilizationMode);

private:
    sptr<ICaptureSession> captureSession_;
    std::shared_ptr<SessionCallback> appCallback_;
    sptr<ICaptureSessionCallback> captureSessionCallback_;
    static const std::unordered_map<CameraVideoStabilizationMode, VideoStabilizationMode> metaToFwVideoStabModes_;
    static const std::unordered_map<VideoStabilizationMode, CameraVideoStabilizationMode> fwToMetaVideoStabModes_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAPTURE_SESSION_H
