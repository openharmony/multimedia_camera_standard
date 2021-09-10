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

#ifndef OHOS_CAMERA_H_STREAM_REPEAT_H
#define OHOS_CAMERA_H_STREAM_REPEAT_H

#include "camera_metadata_info.h"
#include "display_type.h"
#include "hstream_repeat_stub.h"
#include "istream_operator.h"

#include <refbase.h>
#include <iostream>

namespace OHOS {
namespace CameraStandard {
class HStreamRepeat : public HStreamRepeatStub {
public:
    HStreamRepeat(sptr<OHOS::IBufferProducer> producer);
    HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t width, int32_t height);
    HStreamRepeat(sptr<OHOS::IBufferProducer> producer, bool isVideo);
    ~HStreamRepeat();

    int32_t LinkInput(sptr<Camera::IStreamOperator> &streamOperator,
    std::shared_ptr<CameraMetadata> cameraAbility, int32_t streamId);
    void SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfo);
    int32_t Release() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t IsStreamsSupported(Camera::OperationMode mode,
                               const std::shared_ptr<CameraStandard::CameraMetadata> &modeSetting,
                               const std::shared_ptr<Camera::StreamInfo> &pInfo);
    sptr<OHOS::IBufferProducer> GetBufferProducer();
    int32_t SetFps(float Fps) override;
    int32_t SetCallback(sptr<IStreamRepeatCallback> &callback) override;
    int32_t OnFrameStarted();
    int32_t OnFrameEnded(int32_t frameCount);
    int32_t OnFrameError(int32_t errorType);
    bool IsVideo();

private:
    int32_t StartPreview();
    int32_t StartVideo();
    int32_t StopPreview();
    int32_t StopVideo();
    bool isVideo_;
    int32_t customPreviewWidth_;
    int32_t customPreviewHeight_;
    sptr<Camera::IStreamOperator> streamOperator_;
    int32_t previewStreamId_, previewCaptureId_;
    int32_t videoStreamId_, videoCaptureId_;
    sptr<OHOS::IBufferProducer> producer_;
    sptr<IStreamRepeatCallback> streamRepeatCallback_;
    std::shared_ptr<CameraMetadata> cameraAbility_;
    std::vector<std::pair<int32_t, int32_t>> validPreviewSizes_ = {{640, 480}, {832, 480}};
    std::vector<std::pair<int32_t, int32_t>> validVideoSizes_ = {{1280, 720}};
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_STREAM_REPEAT_H
