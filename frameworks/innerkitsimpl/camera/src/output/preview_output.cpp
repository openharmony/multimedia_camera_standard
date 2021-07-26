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
#include "hstream_repeat_callback_stub.h"
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

class HStreamRepeatCallbackImpl : public HStreamRepeatCallbackStub {
public:
    sptr<PreviewOutput> previewOutput_ = nullptr;
    HStreamRepeatCallbackImpl() : previewOutput_(nullptr) {
    }

    explicit HStreamRepeatCallbackImpl(const sptr<PreviewOutput>& previewOutput) : previewOutput_(previewOutput) {
    }

    ~HStreamRepeatCallbackImpl()
    {
        previewOutput_ = nullptr;
    }

    int32_t OnFrameStarted() override
    {
        if (previewOutput_ != nullptr && previewOutput_->GetApplicationCallback() != nullptr) {
            previewOutput_->GetApplicationCallback()->OnFrameStarted();
        } else {
            MEDIA_INFO_LOG("Discarding HStreamRepeatCallbackImpl::OnFrameStarted callback in preview");
        }
        return CAMERA_OK;
    }

    int32_t OnFrameEnded(int32_t frameCount) override
    {
        if (previewOutput_ != nullptr && previewOutput_->GetApplicationCallback() != nullptr) {
            previewOutput_->GetApplicationCallback()->OnFrameEnded(frameCount);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamRepeatCallbackImpl::OnFrameEnded callback in preview");
        }
        return CAMERA_OK;
    }

    int32_t OnFrameError(int32_t errorCode) override
    {
        if (previewOutput_ != nullptr && previewOutput_->GetApplicationCallback() != nullptr) {
            previewOutput_->GetApplicationCallback()->OnError(errorCode);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamRepeatCallbackImpl::OnFrameError callback in preview");
        }
        return CAMERA_OK;
    }
};

void PreviewOutput::SetCallback(std::shared_ptr<PreviewCallback> callback)
{
    int32_t errorCode = CAMERA_OK;

    appCallback_ = callback;
    if (appCallback_ != nullptr) {
        if (svcCallback_ == nullptr) {
            svcCallback_ = new HStreamRepeatCallbackImpl(this);
        }
        errorCode = streamRepeat_->SetCallback(svcCallback_);
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("PreviewOutput::SetCallback: Failed to register callback, errorCode: %{public}d", errorCode);
            svcCallback_ = nullptr;
            appCallback_ = nullptr;
        }
    }
    return;
}

sptr<IStreamRepeat> PreviewOutput::GetStreamRepeat()
{
    return streamRepeat_;
}

std::shared_ptr<PreviewCallback> PreviewOutput::GetApplicationCallback()
{
    return appCallback_;
}
} // CameraStandard
} // OHOS