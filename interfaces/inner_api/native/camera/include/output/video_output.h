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

#ifndef OHOS_CAMERA_VIDEO_OUTPUT_H
#define OHOS_CAMERA_VIDEO_OUTPUT_H

#include <iostream>
#include <vector>
#include "camera_metadata_info.h"
#include "output/capture_output.h"
#include "istream_repeat.h"
#include "istream_repeat_callback.h"
#include "session/capture_session.h"

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
     * @brief Set the video callback for the video output.
     *
     * @param VideoCallback pointer to be triggered.
     */
    void SetCallback(std::shared_ptr<VideoCallback> callback);

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

    /**
    * @brief Get the supported video frame rate range.
    *
    * @return Returns vector<int32_t> of supported exposure compensation range.
    */
    std::vector<int32_t> GetFrameRateRange();

    /**
     * @brief Set the Video fps range. If fixed frame rate
     * to be set the both min and max framerate should be same.
     *
     * @param min frame rate value of range.
     * @param max frame rate value of range.
     */
    void SetFrameRateRange(int32_t minFrameRate, int32_t maxFrameRate);

private:
    std::shared_ptr<VideoCallback> appCallback_;
    sptr<IStreamRepeatCallback> svcCallback_;
    std::vector<int32_t> videoFramerateRange_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_VIDEO_OUTPUT_H
