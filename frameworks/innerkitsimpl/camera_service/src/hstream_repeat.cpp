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
#include "hstream_repeat.h"
#include "media_log.h"

#include <iostream>

namespace OHOS {
namespace CameraStandard {
HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer)
{
    if (producer == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeat::HStreamRepeat producer is null");
        return;
    }
    producer_ = producer;
    isVideo_ = false;
}

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, bool isVideo)
{
    if (producer == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeat::HStreamRepeat producer is null");
        return;
    }
    producer_ = producer;
    isVideo_ = isVideo;
}

HStreamRepeat::~HStreamRepeat()
{}

int32_t HStreamRepeat::LinkInput(sptr<Camera::IStreamOperator> &streamOperator,
            std::shared_ptr<CameraMetadata> cameraAbility, int32_t streamId)
{
    if (streamOperator == nullptr || cameraAbility == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeat::LinkInput streamOperator is null");
        return CAMERA_INVALID_ARG;
    }
    streamOperator_ = streamOperator;
    if (isVideo_) {
        videoStreamId_ = streamId;
    } else {
        previewStreamId_ = streamId;
    }
    cameraAbility_ = cameraAbility;
    return CAMERA_OK;
}

void HStreamRepeat::SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfo)
{
    streamInfo->format_ = PIXEL_FMT_YCRCB_420_SP;
    streamInfo->tunneledMode_ = false;
    streamInfo->datasapce_ = CAMERA_PREVIEW_COLOR_SPACE;
    streamInfo->bufferQueue_ = producer_;
    if (isVideo_) {
        streamInfo->streamId_ = videoStreamId_;
        streamInfo->width_ = CAMERA_VIDEO_WIDTH;
        streamInfo->height_ = CAMERA_VIDEO_HEIGHT;
        streamInfo->intent_ = Camera::VIDEO;
        streamInfo->encodeType_ = Camera::ENCODE_TYPE_H264;
    } else {
        streamInfo->streamId_ = previewStreamId_;
        streamInfo->width_ = CAMERA_PREVIEW_WIDTH;
        streamInfo->height_ = CAMERA_PREVIEW_HEIGHT;
        streamInfo->intent_ = Camera::PREVIEW;
    }
}

int32_t HStreamRepeat::Start()
{
    int32_t rc = CAMERA_OK;

    if (isVideo_) {
        rc = StartVideo();
    } else {
        rc = StartPreview();
    }
    return rc;
}

int32_t HStreamRepeat::StartVideo()
{
    Camera::CamRetCode rc = Camera::NO_ERROR;

    videoCaptureId_ = CAMERA_VIDEO_CAPTURE_ID;
    std::shared_ptr<Camera::CaptureInfo> captureInfoVideo = std::make_shared<Camera::CaptureInfo>();
    captureInfoVideo->streamIds_ = {videoStreamId_};
    captureInfoVideo->captureSetting_ = cameraAbility_;
    captureInfoVideo->enableShutterCallback_ = false;
    rc = streamOperator_->Capture(videoCaptureId_, captureInfoVideo, true);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamRepeat::Start CommitStreams Video failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return HdiToServiceError(rc);
}

int32_t HStreamRepeat::StartPreview()
{
    Camera::CamRetCode rc = Camera::NO_ERROR;

    previewCaptureId_ = CAMERA_PREVIEW_CAPTURE_ID;
    std::shared_ptr<Camera::CaptureInfo> captureInfoPreview = std::make_shared<Camera::CaptureInfo>();
    captureInfoPreview->streamIds_ = {previewStreamId_};
    captureInfoPreview->captureSetting_ = cameraAbility_;
    captureInfoPreview->enableShutterCallback_ = false;
    rc = streamOperator_->Capture(previewCaptureId_, captureInfoPreview, true);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamRepeat::StartPreview failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return HdiToServiceError(rc);
}

int32_t HStreamRepeat::StopPreview()
{
    Camera::CamRetCode rc = Camera::NO_ERROR;

    if (previewCaptureId_ != 0) {
        rc = streamOperator_->CancelCapture(previewCaptureId_);
        if (rc != Camera::NO_ERROR) {
            MEDIA_ERR_LOG("HStreamRepeat::StopStreamRepeat failed with error Code:%{public}d", rc);
            return HdiToServiceError(rc);
        }
        previewCaptureId_ = 0;
    }
    return HdiToServiceError(rc);
}

int32_t HStreamRepeat::StopVideo()
{
    Camera::CamRetCode rc = Camera::NO_ERROR;

    if (videoCaptureId_ != 0) {
        rc = streamOperator_->CancelCapture(videoCaptureId_);
        if (rc != NO_ERROR) {
            MEDIA_ERR_LOG("HStreamRepeat::Stop failed  with error Code:%{public}d", rc);
            return HdiToServiceError(rc);
        }
        videoCaptureId_ = 0;
    }
    return rc;
}

int32_t HStreamRepeat::Stop()
{
    int32_t rc = NO_ERROR;

    if (isVideo_) {
        rc = StopVideo();
    } else {
        rc = StopPreview();
    }
    return rc;
}

int32_t HStreamRepeat::SetFps(float Fps)
{
    return CAMERA_OK;
}

int32_t HStreamRepeat::Release()
{
    streamRepeatCallback_ = nullptr;
    videoCaptureId_ = 0;
    previewCaptureId_ = 0;
    return CAMERA_OK;
}

int32_t HStreamRepeat::IsStreamsSupported(Camera::OperationMode mode,
                                          const std::shared_ptr<CameraStandard::CameraMetadata> &modeSetting,
                                          const std::shared_ptr<Camera::StreamInfo> &pInfo)
{
    Camera::StreamSupportType pType;
    Camera::CamRetCode rc = streamOperator_->IsStreamsSupported(mode, modeSetting,
        pInfo, pType);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamRepeat::IsStreamsSupported failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    } else if (pType == Camera::NOT_SUPPORTED) {
        MEDIA_ERR_LOG("HStreamRepeat::IsStreamsSupported Not supported");
        return CAMERA_UNKNOWN_ERROR;
    }

    return CAMERA_OK;
}

bool HStreamRepeat::IsVideo()
{
    return isVideo_;
}

sptr<OHOS::IBufferProducer> HStreamRepeat::GetBufferProducer()
{
    return producer_;
}

int32_t HStreamRepeat::SetCallback(sptr<IStreamRepeatCallback> &callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeat::SetCallback callback is null");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatCallback_ = callback;
    return CAMERA_OK;
}

int32_t HStreamRepeat::OnFrameStarted()
{
    if (streamRepeatCallback_ != nullptr) {
        streamRepeatCallback_->OnFrameStarted();
    }
    return CAMERA_OK;
}

int32_t HStreamRepeat::OnFrameEnded(int32_t frameCount)
{
    if (streamRepeatCallback_ != nullptr) {
        streamRepeatCallback_->OnFrameEnded(frameCount);
    }
    return CAMERA_OK;
}

int32_t HStreamRepeat::OnFrameError(int32_t errorType)
{
    if (streamRepeatCallback_ != nullptr) {
        if (errorType == Camera::BUFFER_LOST) {
           streamRepeatCallback_->OnFrameError(CAMERA_STREAM_BUFFER_LOST);
        } else {
            streamRepeatCallback_->OnFrameError(CAMERA_UNKNOWN_ERROR);
        }
    }
    return CAMERA_OK;
}
} // namespace Standard
} // namespace OHOS
