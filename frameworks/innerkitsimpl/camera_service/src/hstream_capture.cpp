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
#include "hstream_capture.h"
#include "media_log.h"

#include <iostream>

namespace OHOS {
namespace CameraStandard {
HStreamCapture::HStreamCapture(sptr<OHOS::IBufferProducer> producer)
{
    if (producer == nullptr) {
        MEDIA_ERR_LOG("HStreamCapture::HStreamCapture producer is null");
        return;
    }
    producer_ = producer;
}

HStreamCapture::~HStreamCapture()
{}

int32_t HStreamCapture::LinkInput(sptr<Camera::IStreamOperator> &streamOperator,
        std::shared_ptr<CameraMetadata> cameraAbility, int32_t streamId)
{
    if (streamOperator == nullptr || cameraAbility == nullptr) {
        MEDIA_ERR_LOG("HStreamCapture::LinkInput streamOperator is null");
        return CAMERA_INVALID_ARG;
    }
    streamOperator_ = streamOperator;
    photoStreamId_ = streamId;
    cameraAbility_ = cameraAbility;
    return CAMERA_OK;
}

void HStreamCapture::SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfoPhoto)
{
    streamInfoPhoto->streamId_ = photoStreamId_;
    streamInfoPhoto->width_ = CAMERA_PHOTO_WIDTH;
    streamInfoPhoto->height_ = CAMERA_PHOTO_HEIGHT;
    streamInfoPhoto->format_ = PIXEL_FMT_YCRCB_420_SP;
    streamInfoPhoto->datasapce_ = CAMERA_PHOTO_COLOR_SPACE;
    streamInfoPhoto->intent_ = Camera::STILL_CAPTURE;
    streamInfoPhoto->tunneledMode_ = false;
    streamInfoPhoto->bufferQueue_ = producer_;
    streamInfoPhoto->encodeType_ = Camera::ENCODE_TYPE_JPEG;
}

int32_t HStreamCapture::Capture()
{
    Camera::CamRetCode rc = Camera::NO_ERROR;

    photoCaptureId_ = CAMERA_PHOTO_CAPTURE_ID;
    std::shared_ptr<Camera::CaptureInfo> captureInfoPhoto = std::make_shared<Camera::CaptureInfo>();
    captureInfoPhoto->streamIds_ = {photoStreamId_};
    captureInfoPhoto->captureSetting_ = cameraAbility_;
    captureInfoPhoto->enableShutterCallback_ = true;

    rc = streamOperator_->Capture(photoCaptureId_, captureInfoPhoto, false);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamCapture::Capture failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::CancelCapture()
{
    if (photoCaptureId_ != 0) {
        Camera::CamRetCode rc = streamOperator_->CancelCapture(CAMERA_PHOTO_CAPTURE_ID);
        if (rc != Camera::NO_ERROR) {
            MEDIA_ERR_LOG("HStreamCapture::CancelCapture failed with error Code:%{public}d", rc);
            return HdiToServiceError(rc);
        }
        photoCaptureId_ = 0;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::Release()
{
    streamCaptureCallback_ = nullptr;
    photoCaptureId_ = 0;
    return CAMERA_OK;
}

int32_t HStreamCapture::SetCallback(sptr<IStreamCaptureCallback> &callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("HStreamCapture::SetCallback callback is null");
        return CAMERA_INVALID_ARG;
    }
    streamCaptureCallback_ = callback;
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureStarted(int32_t captureId)
{
    if (streamCaptureCallback_ != nullptr) {
        streamCaptureCallback_->OnCaptureStarted(captureId);
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureEnded(int32_t captureId)
{
    if (streamCaptureCallback_ != nullptr) {
        streamCaptureCallback_->OnCaptureEnded(captureId);
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureError(int32_t captureId, int32_t errorCode)
{
    if (streamCaptureCallback_ != nullptr) {
        if (errorCode == Camera::BUFFER_LOST) {
            streamCaptureCallback_->OnCaptureError(captureId, CAMERA_STREAM_BUFFER_LOST);
        } else {
            streamCaptureCallback_->OnCaptureError(captureId, CAMERA_UNKNOWN_ERROR);
        }
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::OnFrameShutter(int32_t captureId, uint64_t timestamp)
{
    if (streamCaptureCallback_ != nullptr) {
        streamCaptureCallback_->OnFrameShutter(captureId, timestamp);
    }
    return CAMERA_OK;
}
} // namespace CameraStandard
} // namespace OHOS
