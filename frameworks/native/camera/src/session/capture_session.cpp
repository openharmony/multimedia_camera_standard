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

#include "session/capture_session.h"
#include "camera_util.h"
#include "hcapture_session_callback_stub.h"
#include "input/camera_input.h"
#include "camera_log.h"
#include "output/photo_output.h"
#include "output/preview_output.h"
#include "output/video_output.h"

using namespace std;

namespace OHOS {
namespace CameraStandard {
class CaptureSessionCallback : public HCaptureSessionCallbackStub {
public:
    sptr<CaptureSession> captureSession_ = nullptr;
    CaptureSessionCallback() : captureSession_(nullptr) {
    }

    explicit CaptureSessionCallback(const sptr<CaptureSession> &captureSession) : captureSession_(captureSession) {
    }

    ~CaptureSessionCallback()
    {
        captureSession_ = nullptr;
    }

    int32_t OnError(int32_t errorCode) override
    {
        MEDIA_INFO_LOG("CaptureSessionCallback::OnError() is called!, errorCode: %{public}d",
                       errorCode);
        if (captureSession_ != nullptr && captureSession_->GetApplicationCallback() != nullptr) {
            CAMERA_SYSEVENT_FAULT(CreateMsg("Session OnError! errorCode:%d", errorCode));
            captureSession_->GetApplicationCallback()->OnError(errorCode);
        } else {
            MEDIA_INFO_LOG("CaptureSessionCallback::ApplicationCallback not set!, Discarding callback");
        }
        return CAMERA_OK;
    }
};

CaptureSession::CaptureSession(sptr<ICaptureSession> &captureSession)
{
    captureSession_ = captureSession;
    inputDevice_ = nullptr;
}

int32_t CaptureSession::BeginConfig()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->BeginConfig();
}

int32_t CaptureSession::CommitConfig()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->CommitConfig();
}

int32_t CaptureSession::AddInput(sptr<CaptureInput> &input)
{
    CAMERA_SYNC_TRACE;
    if (input == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::AddInput input is null");
        return CAMERA_INVALID_ARG;
    }
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddInput"));
    inputDevice_ = input;
    return captureSession_->AddInput(((sptr<CameraInput> &)input)->GetCameraDevice());
}

int32_t CaptureSession::AddOutput(sptr<CaptureOutput> &output)
{
    CAMERA_SYNC_TRACE;
    if (output == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::AddOutput output is null");
        return CAMERA_INVALID_ARG;
    }
    if (output->GetType() == CAPTURE_OUTPUT_TYPE::PHOTO_OUTPUT) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddPhotoOutput"));
        ((sptr<PhotoOutput> &)output)->SetSession(this);
        return captureSession_->AddOutput(((sptr<PhotoOutput> &)output)->GetStreamCapture());
    } else if (output->GetType() == CAPTURE_OUTPUT_TYPE::PREVIEW_OUTPUT) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddPreviewOutput"));
        return captureSession_->AddOutput(((sptr<PreviewOutput> &)output)->GetStreamRepeat());
    } else {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddVideoOutput"));
        return captureSession_->AddOutput(((sptr<VideoOutput> &)output)->GetStreamRepeat());
    }
}

int32_t CaptureSession::RemoveInput(sptr<CaptureInput> &input)
{
    CAMERA_SYNC_TRACE;
    if (input == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::RemoveInput input is null");
        return CAMERA_INVALID_ARG;
    }
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemoveInput"));
    return captureSession_->RemoveInput(((sptr<CameraInput> &)input)->GetCameraDevice());
}

int32_t CaptureSession::RemoveOutput(sptr<CaptureOutput> &output)
{
    CAMERA_SYNC_TRACE;
    if (output == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::RemoveOutput output is null");
        return CAMERA_INVALID_ARG;
    }
    if (output->GetType() == CAPTURE_OUTPUT_TYPE::PHOTO_OUTPUT) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemovePhotoOutput"));
        return captureSession_->RemoveOutput(((sptr<PhotoOutput> &)output)->GetStreamCapture());
    } else if (output->GetType() == CAPTURE_OUTPUT_TYPE::PREVIEW_OUTPUT) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemovePreviewOutput"));
        return captureSession_->RemoveOutput(((sptr<PreviewOutput> &)output)->GetStreamRepeat());
    } else {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemoveVideoOutput"));
        return captureSession_->RemoveOutput(((sptr<VideoOutput> &)output)->GetStreamRepeat());
    }
}

int32_t CaptureSession::Start()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->Start();
}

int32_t CaptureSession::Stop()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->Stop();
}

void CaptureSession::SetCallback(std::shared_ptr<SessionCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetCallback: Unregistering application callback!");
    }
    int32_t errorCode = CAMERA_OK;

    appCallback_ = callback;
    if (appCallback_ != nullptr) {
        if (captureSessionCallback_ == nullptr) {
            captureSessionCallback_ = new(std::nothrow) CaptureSessionCallback(this);
        }
        errorCode = captureSession_->SetCallback(captureSessionCallback_);
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("CaptureSession::SetCallback: Failed to register callback, errorCode: %{public}d", errorCode);
            captureSessionCallback_ = nullptr;
            appCallback_ = nullptr;
        }
    }
    return;
}

std::shared_ptr<SessionCallback> CaptureSession::GetApplicationCallback()
{
    return appCallback_;
}

void CaptureSession::Release()
{
    CAMERA_SYNC_TRACE;
    int32_t errCode = captureSession_->Release(0);
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to Release capture session!, %{public}d", errCode);
    }
    return;
}
} // CameraStandard
} // OHOS
