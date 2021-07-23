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

#ifndef OHOS_CAMERA_PREVIEW_OUTPUT_H
#define OHOS_CAMERA_PREVIEW_OUTPUT_H

#include "capture_output.h"
#include "istream_repeat.h"
#include "istream_repeat_callback.h"

namespace OHOS {
namespace CameraStandard {
class PreviewCallback {
public:
    PreviewCallback() = default;
    virtual ~PreviewCallback() = default;
    virtual void OnFrameStarted() const = 0;
    virtual void OnFrameEnded(const int32_t frameCount) const = 0;
    virtual void OnError(const int32_t errorCode) const = 0;
};

class PreviewOutput : public CaptureOutput {
public:
    PreviewOutput(sptr<IStreamRepeat> &streamRepeat);
    void SetCallback(std::shared_ptr<PreviewCallback> callback);
    void Release() override;
    sptr<IStreamRepeat> GetStreamRepeat();
    std::shared_ptr<PreviewCallback> GetApplicationCallback();

private:
    sptr<IStreamRepeat> streamRepeat_;
    std::shared_ptr<PreviewCallback> appCallback_;
    sptr<IStreamRepeatCallback> svcCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_PREVIEW_OUTPUT_H