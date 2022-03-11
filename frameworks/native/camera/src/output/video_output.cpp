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

#include "output/video_output.h"
#include "camera_util.h"
#include "hstream_repeat_callback_stub.h"
#include "media_log.h"

namespace OHOS {
namespace CameraStandard {
VideoOutput::VideoOutput(sptr<IStreamRepeat> &streamRepeat)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE::VIDEO_OUTPUT), streamRepeat_(streamRepeat) {
}

class HStreamRepeatCallbackImpl : public HStreamRepeatCallbackStub {
public:
    sptr<VideoOutput> videoOutput_ = nullptr;
    HStreamRepeatCallbackImpl() : videoOutput_(nullptr) {
    }

    explicit HStreamRepeatCallbackImpl(const sptr<VideoOutput>& videoOutput) : videoOutput_(videoOutput) {
    }

    ~HStreamRepeatCallbackImpl()
    {
        videoOutput_ = nullptr;
    }

    int32_t OnFrameStarted() override
    {
        if (videoOutput_ != nullptr && videoOutput_->GetApplicationCallback() != nullptr) {
            videoOutput_->GetApplicationCallback()->OnFrameStarted();
        } else {
            MEDIA_INFO_LOG("Discarding HStreamRepeatCallbackImpl::OnFrameStarted callback in video");
        }
        return CAMERA_OK;
    }

    int32_t OnFrameEnded(const int32_t frameCount) override
    {
        if (videoOutput_ != nullptr && videoOutput_->GetApplicationCallback() != nullptr) {
            videoOutput_->GetApplicationCallback()->OnFrameEnded(frameCount);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamRepeatCallbackImpl::OnFrameEnded callback in video");
        }
        return CAMERA_OK;
    }

    int32_t OnFrameError(const int32_t errorCode) override
    {
        if (videoOutput_ != nullptr && videoOutput_->GetApplicationCallback() != nullptr) {
            videoOutput_->GetApplicationCallback()->OnError(errorCode);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamRepeatCallbackImpl::OnFrameError callback in video");
        }
        return CAMERA_OK;
    }
};

void VideoOutput::SetCallback(std::shared_ptr<VideoCallback> callback)
{
    int32_t errorCode = CAMERA_OK;

    appCallback_ = callback;
    if (appCallback_ != nullptr) {
        if (svcCallback_ == nullptr) {
            svcCallback_ = new HStreamRepeatCallbackImpl(this);
        }
        errorCode = streamRepeat_->SetCallback(svcCallback_);
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("VideoOutput::SetCallback: Failed to register callback, errorCode: %{public}d", errorCode);
            svcCallback_ = nullptr;
            appCallback_ = nullptr;
        }
    }
    return;
}

std::vector<float> VideoOutput::GetSupportedFps()
{
    return {};
}

float VideoOutput::GetFps()
{
    return 0;
}

int32_t VideoOutput::SetFps(float fps)
{
    return CAMERA_OK;
}

int32_t VideoOutput::Start()
{
    return streamRepeat_->Start();
}

int32_t VideoOutput::Stop()
{
    return streamRepeat_->Stop();
}

int32_t VideoOutput::Resume()
{
    return streamRepeat_->Start();
}

int32_t VideoOutput::Pause()
{
    return streamRepeat_->Stop();
}

void VideoOutput::Release()
{
    int32_t errCode = streamRepeat_->Release();
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release VideoOutput!, errCode: %{public}d", errCode);
    }
    return;
}

sptr<IStreamRepeat> VideoOutput::GetStreamRepeat()
{
    return streamRepeat_;
}

std::shared_ptr<VideoCallback> VideoOutput::GetApplicationCallback()
{
    return appCallback_;
}
} // CameraStandard
} // OHOS
