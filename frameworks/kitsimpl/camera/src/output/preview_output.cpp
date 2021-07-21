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

#include "output/preview_output.h"
#include "camera_util.h"
#include "media_log.h"

namespace OHOS {
namespace CameraStandard {
PreviewOutput::PreviewOutput(sptr<IStreamRepeat> &streamRepeat)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE::PREVIEW_OUTPUT), streamRepeat_(streamRepeat) {
}

void PreviewOutput::Release()
{
    int32_t errCode = streamRepeat_->Release();
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release PreviewOutput!, errCode: %{public}d", errCode);
    }
    return;
}

void PreviewOutput::SetCallback(std::shared_ptr<PreviewCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("PreviewOutput::SetCallback: Unregistering application callback!");
    }
    appCallback_ = callback;
    return;
}

sptr<IStreamRepeat> PreviewOutput::GetStreamRepeat()
{
    return streamRepeat_;
}
} // CameraStandard
} // OHOS