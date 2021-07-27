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

#include "session/capture_session.h"
#include "camera_util.h"
#include "input/camera_input.h"
#include "media_log.h"
#include "output/photo_output.h"
#include "output/preview_output.h"
#include "output/video_output.h"

using namespace std;

namespace OHOS {
namespace CameraStandard {
CaptureSession::CaptureSession(sptr<ICaptureSession> &captureSession)
{
    captureSession_ = captureSession;
}

int32_t CaptureSession::BeginConfig()
{
    return captureSession_->BeginConfig();
}

int32_t CaptureSession::CommitConfig()
{
    return captureSession_->CommitConfig();
}

int32_t CaptureSession::AddInput(sptr<CaptureInput> &input)
{
    return captureSession_->AddInput(((sptr<CameraInput> &)input)->GetCameraDevice());
}

int32_t CaptureSession::AddOutput(sptr<CaptureOutput> &output)
{
    if (output->GetType() == CAPTURE_OUTPUT_TYPE::PHOTO_OUTPUT) {
        return captureSession_->AddOutput(((sptr<PhotoOutput> &)output)->GetStreamCapture());
    } else if (output->GetType() == CAPTURE_OUTPUT_TYPE::PREVIEW_OUTPUT) {
        return captureSession_->AddOutput(((sptr<PreviewOutput> &)output)->GetStreamRepeat());
    } else {
        return captureSession_->AddOutput(((sptr<VideoOutput> &)output)->GetStreamRepeat());
    }
}

int32_t CaptureSession::Start()
{
    return captureSession_->Start();
}

int32_t CaptureSession::Stop()
{
    return captureSession_->Stop();
}

void CaptureSession::SetCallback(std::shared_ptr<SessionCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetCallback: Unregistering application callback!");
    }
    appCallback_ = callback;
    return;
}

void CaptureSession::Release()
{
    int32_t errCode = captureSession_->Release();
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to Release capture session!, %{public}d", errCode);
    }
    return;
}
} // CameraStandard
} // OHOS
