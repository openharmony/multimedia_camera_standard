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

#include "hstream_repeat.h"

#include <iostream>

#include "camera_util.h"
#include "display.h"
#include "display_manager.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
static const int32_t STREAM_ROTATE_90 = 90;
static const int32_t STREAM_ROTATE_180 = 180;
static const int32_t STREAM_ROTATE_270 = 270;
static const int32_t STREAM_ROTATE_360 = 360;
int32_t HStreamRepeat::videoCaptureId_ = VIDEO_CAPTURE_ID_START;
int32_t HStreamRepeat::previewCaptureId_ = PREVIEW_CAPTURE_ID_START;

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format)
{
    producer_ = producer;
    format_ = format;
    isVideo_ = false;
    streamId_ = 0;
    customPreviewWidth_ = 0;
    customPreviewHeight_ = 0;
    curCaptureID_ = 0;
    isReleaseStream_ = false;
}

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format, int32_t width, int32_t height)
    : HStreamRepeat(producer, format)
{
    customPreviewWidth_ = width;
    customPreviewHeight_ = height;
}

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format, bool isVideo)
    : HStreamRepeat(producer, format)
{
    isVideo_ = isVideo;
}

HStreamRepeat::~HStreamRepeat()
{}

int32_t HStreamRepeat::LinkInput(sptr<Camera::IStreamOperator> streamOperator,
                                 std::shared_ptr<Camera::CameraMetadata> cameraAbility, int32_t streamId)
{
    int32_t previewWidth = 0;
    int32_t previewHeight = 0;

    if (streamOperator == nullptr || cameraAbility == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeat::LinkInput streamOperator is null");
        return CAMERA_INVALID_ARG;
    }
    if (isVideo_) {
        if (!IsValidSize(cameraAbility, format_, producer_->GetDefaultWidth(), producer_->GetDefaultHeight())) {
            return CAMERA_INVALID_SESSION_CFG;
        }
    } else {
        previewWidth = (customPreviewWidth_ == 0) ? producer_->GetDefaultWidth() : customPreviewWidth_;
        previewHeight =  (customPreviewHeight_ == 0) ? producer_->GetDefaultHeight() : customPreviewHeight_;
        if (!IsValidSize(cameraAbility, format_, previewWidth, previewHeight)) {
            return CAMERA_INVALID_SESSION_CFG;
        }
    }
    streamId_ = streamId;
    streamOperator_ = streamOperator;
    cameraAbility_ = cameraAbility;
    if (!isVideo_) {
        SetStreamTransform();
    }
    return CAMERA_OK;
}

void HStreamRepeat::SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfo)
{
    int32_t pixelFormat;
    auto it = g_cameraToPixelFormat.find(format_);
    if (it != g_cameraToPixelFormat.end()) {
        pixelFormat = it->second;
    } else {
#ifdef RK_CAMERA
        pixelFormat = PIXEL_FMT_RGBA_8888;
#else
        pixelFormat = PIXEL_FMT_YCRCB_420_SP;
#endif
    }
    MEDIA_INFO_LOG("HStreamRepeat::SetStreamInfo pixelFormat is %{public}d", pixelFormat);
    if (streamInfo == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeat::SetStreamInfo null");
        return;
    }

    streamInfo->format_ = pixelFormat;
    streamInfo->tunneledMode_ = true;
    streamInfo->datasapce_ = CAMERA_PREVIEW_COLOR_SPACE;
    streamInfo->bufferQueue_ = producer_;
    streamInfo->width_ = (customPreviewWidth_ == 0) ? producer_->GetDefaultWidth() : customPreviewWidth_;
    streamInfo->height_ = (customPreviewHeight_ == 0) ? producer_->GetDefaultHeight() : customPreviewHeight_;
    streamInfo->streamId_ = streamId_;
    if (isVideo_) {
        streamInfo->intent_ = Camera::VIDEO;
        streamInfo->encodeType_ = Camera::ENCODE_TYPE_H264;
    } else {
        streamInfo->intent_ = Camera::PREVIEW;
    }
}

int32_t HStreamRepeat::SetReleaseStream(bool isReleaseStream)
{
    isReleaseStream_ = isReleaseStream;
    return CAMERA_OK;
}

bool HStreamRepeat::IsReleaseStream()
{
    return isReleaseStream_;
}

int32_t HStreamRepeat::Start()
{
    int32_t rc = CAMERA_OK;

    if (streamOperator_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
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
    return ((captureID >= startValue) && (captureID <= endValue));
}

int32_t HStreamRepeat::StartVideo()
{
    CAMERA_SYNC_TRACE;
    Camera::CamRetCode rc = Camera::NO_ERROR;

    if (!IsvalidCaptureID()) {
        MEDIA_ERR_LOG("HStreamRepeat::StartVideo Failed to Start Video videoCaptureId_:%{public}d", videoCaptureId_);
        return CAMERA_CAPTURE_LIMIT_EXCEED;
    }
    curCaptureID_ = videoCaptureId_;
    videoCaptureId_++;
    std::shared_ptr<Camera::CaptureInfo> captureInfoVideo = std::make_shared<Camera::CaptureInfo>();
    captureInfoVideo->streamIds_ = {streamId_};
    captureInfoVideo->captureSetting_ = cameraAbility_;
    captureInfoVideo->enableShutterCallback_ = false;
    MEDIA_INFO_LOG("HStreamRepeat::StartVideo() Starting video with capture ID: %{public}d", curCaptureID_);
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
    CAMERA_SYNC_TRACE;
    Camera::CamRetCode rc = Camera::NO_ERROR;

    if (!IsvalidCaptureID()) {
        MEDIA_ERR_LOG("HStreamRepeat::StartVideo Failed to Start Preview previewCaptureId_:%{public}d",
                      previewCaptureId_);
        return CAMERA_CAPTURE_LIMIT_EXCEED;
    }
    curCaptureID_ = previewCaptureId_;
    previewCaptureId_++;
    std::shared_ptr<Camera::CaptureInfo> captureInfoPreview = std::make_shared<Camera::CaptureInfo>();
    captureInfoPreview->streamIds_ = {streamId_};
    captureInfoPreview->captureSetting_ = cameraAbility_;
    captureInfoPreview->enableShutterCallback_ = false;
    MEDIA_INFO_LOG("HStreamRepeat::StartPreview() Starting preview with capture ID: %{public}d", curCaptureID_);
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
    CAMERA_SYNC_TRACE;
    int32_t rc = NO_ERROR;
    Camera::CamRetCode hdiCode = Camera::NO_ERROR;

    if (streamOperator_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
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
    curCaptureID_ = 0;
    streamOperator_ = nullptr;
    streamId_ = 0;
    cameraAbility_ = nullptr;
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
    CAMERA_SYNC_TRACE;
    if (streamRepeatCallback_ != nullptr) {
        streamRepeatCallback_->OnFrameStarted();
    }
    return CAMERA_OK;
}

int32_t HStreamRepeat::OnFrameEnded(int32_t frameCount)
{
    CAMERA_SYNC_TRACE;
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

int32_t HStreamRepeat::GetStreamId()
{
    return streamId_;
}

void HStreamRepeat::ResetCaptureIds()
{
    videoCaptureId_ = VIDEO_CAPTURE_ID_START;
    previewCaptureId_ = PREVIEW_CAPTURE_ID_START;
}

void HStreamRepeat::dumpRepeatStreamInfo(std::string& dumpString)
{
    std::shared_ptr<Camera::StreamInfo> curStreamInfo;
    curStreamInfo = std::make_shared<Camera::StreamInfo>();
    SetStreamInfo(curStreamInfo);
    dumpString += "repeat stream info: \n";
    dumpString += "    Buffer Producer Id:[" + std::to_string(curStreamInfo->bufferQueue_->GetUniqueId());
    dumpString += "]    stream Id:[" + std::to_string(curStreamInfo->streamId_);
    std::map<int, std::string>::const_iterator iter =
        g_cameraFormat.find(format_);
    if (iter != g_cameraFormat.end()) {
        dumpString += "]    format:[" + iter->second;
    }
    dumpString += "]    width:[" + std::to_string(curStreamInfo->width_);
    dumpString += "]    height:[" + std::to_string(curStreamInfo->height_);
    dumpString += "]    TunnelMode:[" + std::to_string(curStreamInfo->tunneledMode_);
    dumpString += "]    dataspace:[" + std::to_string(curStreamInfo->datasapce_);
    dumpString += "]    Is Video:[" + std::to_string(isVideo_);
    if (isVideo_) {
        dumpString += "]    StreamType:[" + std::to_string(curStreamInfo->intent_);
        dumpString += "]    Encoding Type:[" + std::to_string(curStreamInfo->encodeType_) + "]:\n";
    } else {
        dumpString += "]    StreamType:[" + std::to_string(curStreamInfo->intent_) + "]:\n";
    }
}

void HStreamRepeat::SetStreamTransform()
{
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(cameraAbility_->get(), OHOS_SENSOR_ORIENTATION, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("HStreamRepeat::SetStreamTransform get sensor orientation failed");
        return;
    }
    int32_t sensorOrientation = item.data.i32[0];
    MEDIA_INFO_LOG("HStreamRepeat::SetStreamTransform sensor orientation %{public}d", sensorOrientation);
    auto display = OHOS::Rosen::DisplayManager::GetInstance().GetDefaultDisplay();
    if (display->GetWidth() < display->GetHeight()) {
        ret = SurfaceError::SURFACE_ERROR_OK;
        int32_t streamRotation = STREAM_ROTATE_360 - sensorOrientation;
        switch (streamRotation) {
            case STREAM_ROTATE_90: {
                ret = producer_->SetTransform(ROTATE_90);
                break;
            }
            case STREAM_ROTATE_180: {
                ret = producer_->SetTransform(ROTATE_180);
                break;
            }
            case STREAM_ROTATE_270: {
                ret = producer_->SetTransform(ROTATE_270);
                break;
            }
            default: {
                break;
            }
        }
        MEDIA_INFO_LOG("HStreamRepeat::SetStreamTransform rotate %{public}d", streamRotation);
        if (ret != SurfaceError::SURFACE_ERROR_OK) {
            MEDIA_ERR_LOG("HStreamRepeat::SetStreamTransform failed %{public}d", ret);
        }
    }
}
} // namespace Standard
} // namespace OHOS
