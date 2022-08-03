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

#ifndef OHOS_CAMERA_H_STREAM_REPEAT_H
#define OHOS_CAMERA_H_STREAM_REPEAT_H

#include "camera_metadata_info.h"
#include "display_type.h"
#include "hstream_repeat_stub.h"
#include "hstream_common.h"
#include "v1_0/istream_operator.h"

#include <refbase.h>
#include <iostream>

namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Camera::V1_0;
class HStreamRepeat : public HStreamRepeatStub, public HStreamCommon {
public:
    HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format);
    HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format, int32_t width, int32_t height);
    HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format, bool isVideo);
    ~HStreamRepeat();

    int32_t LinkInput(sptr<IStreamOperator> streamOperator,
        std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility, int32_t streamId) override;
    void SetStreamInfo(StreamInfo &streamInfo) override;
    int32_t Release() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t SetFps(float Fps) override;
    int32_t SetCallback(sptr<IStreamRepeatCallback> &callback) override;
    int32_t OnFrameStarted();
    int32_t OnFrameEnded(int32_t frameCount);
    int32_t OnFrameError(int32_t errorType);
    bool IsVideo();
    void DumpStreamInfo(std::string& dumpString) override;

private:
    void SetStreamTransform();
    bool isVideo_;
    sptr<IStreamRepeatCallback> streamRepeatCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_STREAM_REPEAT_H
