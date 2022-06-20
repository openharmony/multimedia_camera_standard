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

#include "hstream_capture.h"

#include "camera_util.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
HStreamCapture::HStreamCapture(sptr<OHOS::IBufferProducer> producer, int32_t format)
    : HStreamCommon(StreamType::CAPTURE, producer, format)
{}

HStreamCapture::~HStreamCapture()
{}

int32_t HStreamCapture::LinkInput(sptr<Camera::IStreamOperator> streamOperator,
                                  std::shared_ptr<Camera::CameraMetadata> cameraAbility, int32_t streamId)
{
    return HStreamCommon::LinkInput(streamOperator, cameraAbility, streamId);
}

void HStreamCapture::SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfo)
{
    if (streamInfo == nullptr) {
        MEDIA_ERR_LOG("HStreamCapture::SetStreamInfo null");
        return;
    }
    HStreamCommon::SetStreamInfo(streamInfo);
    streamInfo->intent_ = Camera::STILL_CAPTURE;
    streamInfo->encodeType_ = Camera::ENCODE_TYPE_JPEG;
}

int32_t HStreamCapture::Capture(const std::shared_ptr<Camera::CameraMetadata> &captureSettings)
{
    CAMERA_SYNC_TRACE;

    if (streamOperator_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
    int32_t ret = AllocateCaptureId(curCaptureID_);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("HStreamCapture::Capture Failed to allocate a captureId");
        return ret;
    }
    std::shared_ptr<Camera::CaptureInfo> captureInfoPhoto = std::make_shared<Camera::CaptureInfo>();
    captureInfoPhoto->streamIds_ = {streamId_};
    if (!Camera::GetCameraMetadataItemCount(captureSettings->get())) {
        captureInfoPhoto->captureSetting_ = cameraAbility_;
    } else {
        captureInfoPhoto->captureSetting_ = captureSettings;
    }
    captureInfoPhoto->enableShutterCallback_ = true;

    MEDIA_INFO_LOG("HStreamCapture::Capture Starting photo capture with capture ID: %{public}d", curCaptureID_);
    Camera::CamRetCode rc = streamOperator_->Capture(curCaptureID_, captureInfoPhoto, false);
    if (rc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamCapture::Capture failed with error Code: %{public}d", rc);
        ret = HdiToServiceError(rc);
    }
    ReleaseCaptureId(curCaptureID_);
    curCaptureID_ = 0;
    return ret;
}

int32_t HStreamCapture::CancelCapture()
{
    // Cancel cature is dummy till continuous/burst mode is supported
    return CAMERA_OK;
}

int32_t HStreamCapture::Release()
{
    if (curCaptureID_) {
        ReleaseCaptureId(curCaptureID_);
    }
    streamCaptureCallback_ = nullptr;
    return HStreamCommon::Release();
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
    CAMERA_SYNC_TRACE;
    if (streamCaptureCallback_ != nullptr) {
        streamCaptureCallback_->OnCaptureStarted(captureId);
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureEnded(int32_t captureId, int32_t frameCount)
{
    CAMERA_SYNC_TRACE;
    if (streamCaptureCallback_ != nullptr) {
        streamCaptureCallback_->OnCaptureEnded(captureId, frameCount);
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
    CAMERA_SYNC_TRACE;
    if (streamCaptureCallback_ != nullptr) {
        streamCaptureCallback_->OnFrameShutter(captureId, timestamp);
    }
    return CAMERA_OK;
}

void HStreamCapture::DumpStreamInfo(std::string& dumpString)
{
    dumpString += "capture stream:\n";
    HStreamCommon::DumpStreamInfo(dumpString);
}
} // namespace CameraStandard
} // namespace OHOS
