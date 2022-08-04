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

#include "camera_util.h"
#include "metadata_utils.h"
#include "display.h"
#include "display_manager.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
static const int32_t STREAM_ROTATE_90 = 90;
static const int32_t STREAM_ROTATE_180 = 180;
static const int32_t STREAM_ROTATE_270 = 270;
static const int32_t STREAM_ROTATE_360 = 360;

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format)
    : HStreamCommon(StreamType::REPEAT, producer, format)
{
    isVideo_ = false;
}

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format, int32_t width, int32_t height)
    : HStreamRepeat(producer, format)
{
    width_ = width;
    height_ = height;
}

HStreamRepeat::HStreamRepeat(sptr<OHOS::IBufferProducer> producer, int32_t format, bool isVideo)
    : HStreamRepeat(producer, format)
{
    isVideo_ = isVideo;
}

HStreamRepeat::~HStreamRepeat()
{}

int32_t HStreamRepeat::LinkInput(sptr<IStreamOperator> streamOperator,
                                 std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility, int32_t streamId)
{
    int32_t ret = HStreamCommon::LinkInput(streamOperator, cameraAbility, streamId);
    if (ret != CAMERA_OK) {
        return ret;
    }
    if (!isVideo_) {
        SetStreamTransform();
    }
    return CAMERA_OK;
}

void HStreamRepeat::SetStreamInfo(StreamInfo &streamInfo)
{
    HStreamCommon::SetStreamInfo(streamInfo);
    if (isVideo_) {
        streamInfo.intent_ = VIDEO;
        streamInfo.encodeType_ = ENCODE_TYPE_H264;
    } else {
        streamInfo.intent_ = PREVIEW;
        streamInfo.encodeType_ = ENCODE_TYPE_NULL;
    }
}

int32_t HStreamRepeat::Start()
{
    CAMERA_SYNC_TRACE;

    if (streamOperator_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
    if (curCaptureID_ != 0) {
        MEDIA_ERR_LOG("HStreamRepeat::Start, Already started with captureID: %{public}d", curCaptureID_);
        return CAMERA_INVALID_STATE;
    }
    int32_t ret = AllocateCaptureId(curCaptureID_);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("HStreamRepeat::Start Failed to allocate a captureId");
        return ret;
    }
    std::vector<uint8_t> ability;
    OHOS::Camera::MetadataUtils::ConvertMetadataToVec(cameraAbility_, ability);
    CaptureInfo captureInfo;
    captureInfo.streamIds_ = {streamId_};
    captureInfo.captureSetting_ = ability;
    captureInfo.enableShutterCallback_ = false;
    MEDIA_INFO_LOG("HStreamRepeat::Start Starting with capture ID: %{public}d", curCaptureID_);
    CamRetCode rc = (CamRetCode)(streamOperator_->Capture(curCaptureID_, captureInfo, true));
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        ReleaseCaptureId(curCaptureID_);
        curCaptureID_ = 0;
        MEDIA_ERR_LOG("HStreamRepeat::Start Failed with error Code:%{public}d", rc);
        ret = HdiToServiceError(rc);
    }
    return ret;
}

int32_t HStreamRepeat::Stop()
{
    CAMERA_SYNC_TRACE;

    if (streamOperator_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
    if (curCaptureID_ == 0) {
        MEDIA_ERR_LOG("HStreamRepeat::Stop, Stream not started yet");
        return CAMERA_INVALID_STATE;
    }
    int32_t ret = CAMERA_OK;
    CamRetCode rc = (CamRetCode)(streamOperator_->CancelCapture(curCaptureID_));
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamRepeat::Stop Failed with errorCode:%{public}d, curCaptureID_: %{public}d",
                      rc, curCaptureID_);
        ret = HdiToServiceError(rc);
    }
    ReleaseCaptureId(curCaptureID_);
    curCaptureID_ = 0;
    return ret;
}

int32_t HStreamRepeat::SetFps(float Fps)
{
    return CAMERA_OK;
}

int32_t HStreamRepeat::Release()
{
    if (curCaptureID_) {
        ReleaseCaptureId(curCaptureID_);
    }
    streamRepeatCallback_ = nullptr;
    return HStreamCommon::Release();
}

bool HStreamRepeat::IsVideo()
{
    return isVideo_;
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
        if (errorType == BUFFER_LOST) {
            streamRepeatCallback_->OnFrameError(CAMERA_STREAM_BUFFER_LOST);
        } else {
            streamRepeatCallback_->OnFrameError(CAMERA_UNKNOWN_ERROR);
        }
    }
    return CAMERA_OK;
}

void HStreamRepeat::DumpStreamInfo(std::string& dumpString)
{
    dumpString += "repeat stream:\n";
    HStreamCommon::DumpStreamInfo(dumpString);
}

void HStreamRepeat::SetStreamTransform()
{
    camera_metadata_item_t item;
    int ret = OHOS::Camera::FindCameraMetadataItem(cameraAbility_->get(), OHOS_SENSOR_ORIENTATION, &item);
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
