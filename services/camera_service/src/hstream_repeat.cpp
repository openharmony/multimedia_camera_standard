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
    producer_ = producer;
    isVideo_ = false;
    videoStreamId_ = 0;
    videoCaptureId_ = 0;
    previewStreamId_ = 0;
    previewCaptureId_ = 0;
    customPreviewWidth_ = 0;
    customPreviewHeight_ = 0;
    curCaptureID_ = 0;
}

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t width, int32_t height)
{
    producer_ = producer;
    isVideo_ = false;
    videoStreamId_ = 0;
    videoCaptureId_ = 0;
    previewStreamId_ = 0;
    previewCaptureId_ = 0;
    customPreviewWidth_ = width;
    customPreviewHeight_ = height;
    curCaptureID_ = 0;
}

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, bool isVideo)
{
    producer_ = producer;
    isVideo_ = isVideo;
    videoStreamId_ = 0;
    videoCaptureId_ = 0;
    previewStreamId_ = 0;
    previewCaptureId_ = 0;
    customPreviewWidth_ = 0;
    customPreviewHeight_ = 0;
    curCaptureID_ = 0;
}

HStreamRepeat::~HStreamRepeat()
{}

int32_t HStreamRepeat::LinkInput(sptr<Camera::IStreamOperator> &streamOperator,
                                 std::shared_ptr<CameraMetadata> cameraAbility, int32_t streamId)
{
    int32_t previewWidth = 0;
    int32_t previewHeight = 0;

    if (streamOperator == nullptr || cameraAbility == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeat::LinkInput streamOperator is null");
        return CAMERA_INVALID_ARG;
    }
    if (isVideo_) {
        if (!IsValidSize(producer_->GetDefaultWidth(), producer_->GetDefaultHeight(), validVideoSizes_)) {
            return CAMERA_INVALID_OUTPUT_CFG;
        }
        videoStreamId_ = streamId;
        videoCaptureId_ = VIDEO_CAPTURE_ID_START;
    } else {
        previewWidth = (customPreviewWidth_ == 0) ? producer_->GetDefaultWidth() : customPreviewWidth_;
        previewHeight =  (customPreviewHeight_ == 0) ? producer_->GetDefaultHeight() : customPreviewHeight_;
        if (!IsValidSize(previewWidth, previewHeight, validPreviewSizes_)) {
            return CAMERA_INVALID_OUTPUT_CFG;
        }
        previewStreamId_ = streamId;
        previewCaptureId_ = PREVIEW_CAPTURE_ID_START;
    }
    streamOperator_ = streamOperator;
    cameraAbility_ = cameraAbility;
    return CAMERA_OK;
}

void HStreamRepeat::SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfo)
{
    streamInfo->format_ = PIXEL_FMT_YCRCB_420_SP;
    streamInfo->tunneledMode_ = true;
    streamInfo->datasapce_ = CAMERA_PREVIEW_COLOR_SPACE;
    streamInfo->bufferQueue_ = producer_;
    streamInfo->width_ = (customPreviewWidth_ == 0) ? producer_->GetDefaultWidth() : customPreviewWidth_;
    streamInfo->height_ = (customPreviewHeight_ == 0) ? producer_->GetDefaultHeight() : customPreviewHeight_;
    if (isVideo_) {
        streamInfo->streamId_ = videoStreamId_;
        streamInfo->intent_ = Camera::VIDEO;
        streamInfo->encodeType_ = Camera::ENCODE_TYPE_H264;
    } else {
        streamInfo->streamId_ = previewStreamId_;
        streamInfo->intent_ = Camera::PREVIEW;
    }
}

int32_t HStreamRepeat::Start()
{
    int32_t rc = CAMERA_OK;

    if (curCaptureID_ != 0) {
        MEDIA_ERR_LOG("HStreamRepeat::Start, Already started with captureID: %{public}d", curCaptureID_);
        return CAMERA_INVALID_STATE;
    }
    if (isVideo_) {
        rc = StartVideo();
    } else {
        rc = StartPreview();
    }
    return rc;
}

bool HStreamRepeat::IsvalidCaptureID()
{
    int32_t startValue = 0;
    int32_t endValue = 0;
    int32_t captureID = 0;

    if (isVideo_) {
        captureID = videoCaptureId_;
        startValue = VIDEO_CAPTURE_ID_START;
        endValue = VIDEO_CAPTURE_ID_END;
    } else {
        captureID = previewCaptureId_;
        startValue = PREVIEW_CAPTURE_ID_START;
        endValue = PREVIEW_CAPTURE_ID_END;
    }
    return (captureID >= startValue && captureID <= endValue);
}

int32_t HStreamRepeat::StartVideo()
{
    Camera::CamRetCode rc = Camera::NO_ERROR;

    if (!IsvalidCaptureID()) {
        MEDIA_ERR_LOG("HStreamRepeat::StartVideo Failed to Start Video videoCaptureId_:%{public}d", videoCaptureId_);
        return CAMERA_CAPTURE_LIMIT_EXCEED;
    }
    curCaptureID_ = videoCaptureId_;
    videoCaptureId_++;
    std::shared_ptr<Camera::CaptureInfo> captureInfoVideo = std::make_shared<Camera::CaptureInfo>();
    captureInfoVideo->streamIds_ = {videoStreamId_};
    captureInfoVideo->captureSetting_ = cameraAbility_;
    captureInfoVideo->enableShutterCallback_ = false;
    MEDIA_INFO_LOG("HStreamCapture::StartVideo() Starting video with capture ID: %{public}d", curCaptureID_);
    rc = streamOperator_->Capture(curCaptureID_, captureInfoVideo, true);
    if (rc != Camera::NO_ERROR) {
        curCaptureID_ = 0;
        MEDIA_ERR_LOG("HStreamRepeat::Start CommitStreams Video failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return HdiToServiceError(rc);
}

int32_t HStreamRepeat::StartPreview()
{
    Camera::CamRetCode rc = Camera::NO_ERROR;

    if (!IsvalidCaptureID()) {
        MEDIA_ERR_LOG("HStreamRepeat::StartVideo Failed to Start Preview previewCaptureId_:%{public}d",
                      previewCaptureId_);
        return CAMERA_CAPTURE_LIMIT_EXCEED;
    }
    curCaptureID_ = previewCaptureId_;
    previewCaptureId_++;
    std::shared_ptr<Camera::CaptureInfo> captureInfoPreview = std::make_shared<Camera::CaptureInfo>();
    captureInfoPreview->streamIds_ = {previewStreamId_};
    captureInfoPreview->captureSetting_ = cameraAbility_;
    captureInfoPreview->enableShutterCallback_ = false;
    MEDIA_INFO_LOG("HStreamCapture::StartPreview() Starting preview with capture ID: %{public}d", curCaptureID_);
    rc = streamOperator_->Capture(curCaptureID_, captureInfoPreview, true);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamRepeat::StartPreview failed with error Code:%{public}d", rc);
        curCaptureID_ = 0;
        return HdiToServiceError(rc);
    }
    return HdiToServiceError(rc);
}

int32_t HStreamRepeat::Stop()
{
    int32_t rc = NO_ERROR;
    Camera::CamRetCode hdiCode = Camera::NO_ERROR;

    if (curCaptureID_ == 0) {
        MEDIA_ERR_LOG("HStreamRepeat::Stop, Stream not started yet");
        return CAMERA_INVALID_STATE;
    }
    hdiCode = streamOperator_->CancelCapture(curCaptureID_);
    if (rc != NO_ERROR) {
        MEDIA_ERR_LOG("HStreamRepeat::Stop failed  with errorCode:%{public}d, curCaptureID_: %{public}d",
                      rc, curCaptureID_);
        return HdiToServiceError(hdiCode);
    }
    curCaptureID_ = 0;
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
    curCaptureID_ = 0;
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
