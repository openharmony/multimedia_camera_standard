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

#include "camera_util.h"
#include "hcapture_session.h"
#include "media_log.h"

#include <iostream>

namespace OHOS {
namespace CameraStandard {
HCaptureSession::HCaptureSession(sptr<HCameraHostManager> cameraHostManager,
    sptr<StreamOperatorCallback> streamOperatorCallback)
    : cameraHostManager_(cameraHostManager), streamOperatorCallback_(streamOperatorCallback)
{
    streamRepeatPreview_ = nullptr;
    streamRepeatVideo_ = nullptr;
    streamCapture_ = nullptr;
    previewStreamID_ = CAMERA_PREVIEW_STREAM_ID;
    photoStreamID_ = CAMERA_PHOTO_STREAM_ID;
    videoStreamID_ = CAMERA_VIDEO_STREAM_ID;
    isConfigCommitted_ = true;
}

HCaptureSession::~HCaptureSession()
{}

int32_t HCaptureSession::BeginConfig()
{
    isConfigCommitted_ = false;
    return CAMERA_OK;
}

int32_t HCaptureSession::AddInput(sptr<ICameraDeviceService> cameraDevice)
{
    if (isConfigCommitted_) {
        MEDIA_ERR_LOG("HCaptureSession::AddInput Need to call BeginConfig before adding input");
        return CAMERA_INVALID_STATE;
    }

    if (cameraDevice == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddInput cameraDevice is null");
        return CAMERA_INVALID_ARG;
    }
    cameraDevice_ = static_cast<HCameraDevice*>(cameraDevice.GetRefPtr());
    return CAMERA_OK;
}

int32_t HCaptureSession::AddOutput(sptr<IStreamRepeat> streamRepeat)
{
    if (isConfigCommitted_) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput Need to call BeginConfig before adding output");
        return CAMERA_INVALID_STATE;
    }

    sptr<Surface> captureSurface = Surface::CreateSurfaceAsConsumer();

    if (streamRepeat == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput streamRepeat is null");
        return CAMERA_INVALID_ARG;
    }
    sptr<HStreamRepeat> streamRepeat_ = static_cast<HStreamRepeat *>(streamRepeat.GetRefPtr());
    if (streamRepeat_->IsVideo()) {
        streamRepeatVideo_ = streamRepeat_;
    } else {
        streamRepeatPreview_ = streamRepeat_;
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::AddOutput(sptr<IStreamCapture> streamCapture)
{
    if (isConfigCommitted_) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput Need to call BeginConfig before adding output");
        return CAMERA_INVALID_STATE;
    }

    if (streamCapture == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput streamCapture is null");
        return CAMERA_INVALID_ARG;
    }
    streamCapture_ = static_cast<HStreamCapture*>(streamCapture.GetRefPtr());
    return CAMERA_OK;
}

int32_t HCaptureSession::RemoveInput()
{
    if (isConfigCommitted_) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Need to call BeginConfig before removing input");
        return CAMERA_INVALID_STATE;
    }

    cameraDevice_ = nullptr;
    return CAMERA_OK;
}

int32_t HCaptureSession::RemoveOutput(sptr<IStreamRepeat> streamRepeat)
{
    if (isConfigCommitted_) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Need to call BeginConfig before removing output");
        return CAMERA_INVALID_STATE;
    }

    if (streamRepeat == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput streamRepeat is null");
        return CAMERA_INVALID_ARG;
    }
    sptr<HStreamRepeat> streamRepeat_ = static_cast<HStreamRepeat*>(streamRepeat.GetRefPtr());
    if (streamRepeat_->IsVideo()) {
        streamRepeatVideo_ = nullptr;
    } else {
        streamRepeatPreview_ = nullptr;
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::RemoveOutput(sptr<IStreamCapture> streamCapture)
{
    if (isConfigCommitted_) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Need to call BeginConfig before removing output");
        return CAMERA_INVALID_STATE;
    }

    streamCapture_ = nullptr;
    return CAMERA_OK;
}

int32_t HCaptureSession::CommitConfig()
{
    int32_t rc = 0;
    streamOperator_ = nullptr;
    std::vector<std::shared_ptr<Camera::StreamInfo>> streamInfos;
    std::shared_ptr<Camera::StreamInfo> streamInfo = nullptr;

    if (isConfigCommitted_) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Need to call BeginConfig before committing configuration");
        return CAMERA_INVALID_STATE;
    }

    if (cameraDevice_ == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Camera input is not added!");
        return CAMERA_UNKNOWN_ERROR;
    }

    if (streamRepeatPreview_ == nullptr && streamCapture_ == nullptr && streamRepeatVideo_ == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Camera output(s) not added!");
        return CAMERA_INVALID_OUTPUT_CFG;
    }

    rc = cameraDevice_->Open();
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Failed to open camera, rc: %{public}d", rc);
        return rc;
    }
    streamOperatorCallback_->SetCaptureSession(this);

    rc = cameraDevice_->GetStreamOperator(streamOperatorCallback_, streamOperator_);
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), GetStreamOperator returned %{public}d", rc);
        return rc;
    }
    streamIds_.clear();
    cameraAbility_ = cameraDevice_->GetSettings();
    if (streamRepeatPreview_ != nullptr) {
        rc = streamRepeatPreview_->LinkInput(streamOperator_, cameraAbility_, previewStreamID_);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Failed to link input to PreviewOutput, %{public}d", rc);
            return rc;
        }
        streamInfo = std::make_shared<Camera::StreamInfo>();
        streamRepeatPreview_->SetStreamInfo(streamInfo);
        streamInfos.push_back(streamInfo);
        streamIds_.push_back(previewStreamID_);
    }
    if (streamCapture_ != nullptr) {
        rc = streamCapture_->LinkInput(streamOperator_, cameraAbility_, photoStreamID_);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Failed to link input to PhotoOutput, %{public}d", rc);
            streamIds_.clear();
            return rc;
        }
        streamInfo = std::make_shared<Camera::StreamInfo>();
        streamCapture_->SetStreamInfo(streamInfo);
        streamInfos.push_back(streamInfo);
        streamIds_.push_back(photoStreamID_);
    }
    if (streamRepeatVideo_ != nullptr) {
        rc = streamRepeatVideo_->LinkInput(streamOperator_, cameraAbility_, videoStreamID_);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Failed to link input to VideoOutput, %{public}d", rc);
            streamIds_.clear();
            return rc;
        }
        streamInfo = std::make_shared<Camera::StreamInfo>();
        streamRepeatVideo_->SetStreamInfo(streamInfo);
        streamInfos.push_back(streamInfo);
        streamIds_.push_back(videoStreamID_);
    }
    if (!streamInfos.empty()) {
        rc = streamOperator_->CreateStreams(streamInfos);
        if (rc == Camera::NO_ERROR) {
            rc = streamOperator_->CommitStreams(Camera::NORMAL, cameraAbility_);
            if (rc == Camera::NO_ERROR) {
                isConfigCommitted_ = true;
                rc = CAMERA_OK;
            } else {
                MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Failed to commit config %{public}d", rc);
                streamOperator_->ReleaseStreams(streamIds_);
                streamIds_.clear();
            }
        }
    }
    return rc;
}

int32_t HCaptureSession::Start()
{
    int32_t rc = CAMERA_UNKNOWN_ERROR;
    if (streamRepeatPreview_ != nullptr) {
        rc = streamRepeatPreview_->Start();
    }
    return rc;
}

int32_t HCaptureSession::Stop()
{
    int32_t rc = CAMERA_UNKNOWN_ERROR;
    if (streamRepeatPreview_ != nullptr) {
        rc = streamRepeatPreview_->Stop();
    }
    return rc;
}

int32_t HCaptureSession::Release()
{
    if (streamOperator_ != nullptr) {
        streamOperator_->ReleaseStreams(streamIds_);
        streamOperator_ = nullptr;
    }
    if (streamOperatorCallback_ != nullptr) {
        streamOperatorCallback_->SetCaptureSession(nullptr);
        streamOperatorCallback_ = nullptr;
    }
    if (streamRepeatPreview_ != nullptr) {
        streamRepeatPreview_->Release();
        streamRepeatPreview_ = nullptr;
    }
    if (streamCapture_ != nullptr) {
        streamCapture_->Release();
        streamCapture_ = nullptr;
    }
    if (streamRepeatVideo_ != nullptr) {
        streamRepeatVideo_->Release();
        streamRepeatVideo_ = nullptr;
    }
    if (cameraDevice_ != nullptr) {
        cameraDevice_->Close();
        cameraDevice_ = nullptr;
    }
    return CAMERA_OK;
}

StreamOperatorCallback::StreamOperatorCallback(sptr<HCaptureSession> session)
{
    captureSession_ = session;
}

void StreamOperatorCallback::OnCaptureStarted(int32_t captureId,
                                              const std::vector<int32_t> &streamId)
{
    if (captureSession_ == nullptr) {
        return;
    }
    CaptureType capType = GetCaptureType(captureId);

    switch (capType) {
        case CAPTURE_TYPE_PREVIEW:
            if (captureSession_->streamRepeatPreview_ != nullptr) {
                captureSession_->streamRepeatPreview_->OnFrameStarted();
            }
            break;
        case CAPTURE_TYPE_PHOTO:
            if (captureSession_->streamCapture_ != nullptr) {
                captureSession_->streamCapture_->OnCaptureStarted(captureId);
            }
            break;
        case CAPTURE_TYPE_VIDEO:
            if (captureSession_->streamRepeatVideo_ != nullptr) {
                captureSession_->streamRepeatVideo_->OnFrameStarted();
            }
            break;
        default:
            break;
    }
}

void StreamOperatorCallback::OnCaptureEnded(int32_t captureId,
                                            const std::vector<std::shared_ptr<Camera::CaptureEndedInfo>> &info)
{
    if (captureSession_ == nullptr) {
        return;
    }
    CaptureType capType = GetCaptureType(captureId);

    switch (capType) {
        case CAPTURE_TYPE_PREVIEW:
            if (captureSession_->streamRepeatPreview_ != nullptr) {
                captureSession_->streamRepeatPreview_->OnFrameEnded(info[0]->frameCount_);
            }
            break;
        case CAPTURE_TYPE_PHOTO:
            if (captureSession_->streamCapture_ != nullptr) {
                captureSession_->streamCapture_->OnCaptureEnded(captureId);
            }
            break;
        case CAPTURE_TYPE_VIDEO:
            if (captureSession_->streamRepeatVideo_ != nullptr) {
                captureSession_->streamRepeatVideo_->OnFrameEnded(info[0]->frameCount_);
            }
            break;
        default:
            break;
    }
}

void StreamOperatorCallback::OnCaptureError(int32_t captureId,
                                            const std::vector<std::shared_ptr<Camera::CaptureErrorInfo>> &info)
{
    if (captureSession_ == nullptr) {
        return;
    }
    CaptureType capType = GetCaptureType(captureId);

    switch (capType) {
        case CAPTURE_TYPE_PREVIEW:
            if (captureSession_->streamRepeatPreview_ != nullptr) {
                captureSession_->streamRepeatPreview_->OnFrameError(info[0]->error_);
            }
            break;
        case CAPTURE_TYPE_PHOTO:
            if (captureSession_->streamCapture_ != nullptr) {
                captureSession_->streamCapture_->OnCaptureError(captureId, info[0]->error_);
            }
            break;
        case CAPTURE_TYPE_VIDEO:
            if (captureSession_->streamRepeatVideo_ != nullptr) {
                captureSession_->streamRepeatVideo_->OnFrameError(info[0]->error_);
            }
            break;
        default:
            break;
    }
}

void StreamOperatorCallback::OnFrameShutter(int32_t captureId,
                                            const std::vector<int32_t> &streamId, uint64_t timestamp)
{
    CaptureType capType = GetCaptureType(captureId);
    if (capType == CAPTURE_TYPE_PHOTO && captureSession_ != nullptr
        && captureSession_->streamCapture_ != nullptr) {
        captureSession_->streamCapture_->OnFrameShutter(captureId, timestamp);
    } else {
        MEDIA_INFO_LOG("HCaptureSession::OnFrameShutter(), called for other streams too");
    }
}

void StreamOperatorCallback::SetCaptureSession(sptr<HCaptureSession> captureSession)
{
    captureSession_ = captureSession;
}
} // namespace CameraStandard
} // namespace OHOS
