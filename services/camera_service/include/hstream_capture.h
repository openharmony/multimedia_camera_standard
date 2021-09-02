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

#ifndef OHOS_CAMERA_H_STREAM_CAPTURE_H
#define OHOS_CAMERA_H_STREAM_CAPTURE_H

#include "camera_metadata_info.h"
#include "display_type.h"
#include "hstream_capture_stub.h"
#include "istream_operator.h"
#include <refbase.h>
#include <iostream>

namespace OHOS {
namespace CameraStandard {
class HStreamCapture : public HStreamCaptureStub {
public:
    HStreamCapture(sptr<OHOS::IBufferProducer> surface);
    ~HStreamCapture();

    int32_t LinkInput(sptr<Camera::IStreamOperator> &streamOperator,
        std::shared_ptr<CameraMetadata> cameraAbility, int32_t streamId);
    void SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfo);
    int32_t Capture() override;
    int32_t CancelCapture() override;
    int32_t Release() override;
    int32_t SetCallback(sptr<IStreamCaptureCallback> &callback) override;
    int32_t OnCaptureStarted(int32_t captureId);
    int32_t OnCaptureEnded(int32_t captureId);
    int32_t OnCaptureError(int32_t captureId, int32_t errorType);
    int32_t OnFrameShutter(int32_t captureId, uint64_t timestamp);

private:
    sptr<Camera::IStreamOperator> streamOperator_;
    int32_t photoStreamId_, photoCaptureId_;
    sptr<OHOS::IBufferProducer> producer_;
    sptr<IStreamCaptureCallback> streamCaptureCallback_;
    std::shared_ptr<CameraMetadata> cameraAbility_;
    std::vector<std::pair<int32_t, int32_t>> validSizes_ = {{1280, 960}};
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_STREAM_CAPTURE_H
