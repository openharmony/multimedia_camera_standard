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

#ifndef OHOS_CAMERA_VIDEO_OUTPUT_H
#define OHOS_CAMERA_VIDEO_OUTPUT_H

#include <iostream>
#include <vector>
#include "output/capture_output.h"
#include "istream_repeat.h"
#include "istream_repeat_callback.h"

namespace OHOS {
namespace CameraStandard {
class VideoCallback {
public:
    VideoCallback() = default;
    virtual ~VideoCallback() = default;

    /**
     * @brief Called when video frame is started rendering.
     */
    virtual void OnFrameStarted() const = 0;

    /**
     * @brief Called when video frame is ended.
     *
     * @param frameCount Indicates number of frames captured.
     */
    virtual void OnFrameEnded(const int32_t frameCount) const = 0;

    /**
     * @brief Called when error occured during video rendering.
     *
     * @param errorCode Indicates a {@link ErrorCode} which will give information for video callback error.
     */
    virtual void OnError(const int32_t errorCode) const = 0;
};

class VideoOutput : public CaptureOutput {
public:
    explicit VideoOutput(sptr<IStreamRepeat> &streamRepeat);

    /**
     * @brief Releases a instance of the VideoOutput.
     */
    void Release() override;

    /**
     * @brief Get repeated stream for the video output.
     *
     * @return Returns the pointer to IStreamRepeat.
     */
    sptr<IStreamRepeat> GetStreamRepeat();

    /**
     * @brief Set the video callback for the video output.
     *
     * @param VideoCallback pointer to be triggered.
     */
    void SetCallback(std::shared_ptr<VideoCallback> callback);

    /**
     * @brief Get supported Fps for the video output.
     *
     * @return Returns the vector<float> of Fps values for the video output.
     */
    std::vector<float> GetSupportedFps();

    /**
     * @brief Get current Fps value.
     *
     * @return Returns a float current Fps for the video output.
     */
    float GetFps();

    /**
     * @brief Set Fps value for the video output.
     *
     * @param video frame rate to be set.
     */
    int32_t SetFps(float fps);

    /**
     * @brief Start the video capture.
     */
    int32_t Start();

    /**
     * @brief Stop the video capture.
     */
    int32_t Stop();

    /**
     * @brief Pause the video capture.
     */
    int32_t Pause();

    /**
     * @brief Resume the paused video capture.
     */
    int32_t Resume();

    /**
     * @brief Get the application callback information.
     *
     * @return VideoCallback pointer set by application.
     */
    std::shared_ptr<VideoCallback> GetApplicationCallback();

private:
    sptr<IStreamRepeat> streamRepeat_;
    std::shared_ptr<VideoCallback> appCallback_;
    sptr<IStreamRepeatCallback> svcCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_VIDEO_OUTPUT_H
