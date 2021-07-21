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

#ifndef OHOS_CAMERA_VIDEO_OUTPUT_H
#define OHOS_CAMERA_VIDEO_OUTPUT_H

#include <iostream>
#include <vector>
#include "output/capture_output.h"
#include "istream_repeat.h"

namespace OHOS {
namespace CameraStandard {
class VideoCallback {
public:
    VideoCallback() = default;
    virtual ~VideoCallback() = default;
    virtual void onFrameStarted() = 0;
    virtual void onFrameEnded() = 0;
    virtual void onError(int32_t error) = 0;
};

class VideoOutput : public CaptureOutput {
public:
    VideoOutput(sptr<IStreamRepeat> &streamRepeat);
    void Release() override;
    sptr<IStreamRepeat> GetStreamRepeat();
    void SetCallback(std::shared_ptr<VideoCallback> callback);
    std::vector<float> GetSupportedFps();
    float GetFps();
    int32_t SetFps(float fps);
    int32_t Start();
    int32_t Stop();

private:
    sptr<IStreamRepeat> streamRepeat_;
    std::shared_ptr<VideoCallback> appCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_VIDEO_OUTPUT_H
