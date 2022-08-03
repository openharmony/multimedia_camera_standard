/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "hstream_metadata.h"

#include "camera_util.h"
#include "camera_log.h"
#include "metadata_utils.h"

namespace OHOS {
namespace CameraStandard {
HStreamMetadata::HStreamMetadata(sptr<OHOS::IBufferProducer> producer, int32_t format)
    : HStreamCommon(StreamType::METADATA, producer, format)
{}

HStreamMetadata::~HStreamMetadata()
{}

int32_t HStreamMetadata::LinkInput(sptr<IStreamOperator> streamOperator,
                                   std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility, int32_t streamId)
{
    if (streamOperator == nullptr || cameraAbility == nullptr) {
        MEDIA_ERR_LOG("HStreamMetadata::LinkInput streamOperator is null");
        return CAMERA_INVALID_ARG;
    }
    streamId_ = streamId;
    streamOperator_ = streamOperator;
    cameraAbility_ = cameraAbility;
    return CAMERA_OK;
}

void HStreamMetadata::SetStreamInfo(StreamInfo &streamInfo)
{
    HStreamCommon::SetStreamInfo(streamInfo);
    streamInfo.intent_ = ANALYZE;
}

int32_t HStreamMetadata::Start()
{
    CAMERA_SYNC_TRACE;

    if (streamOperator_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
    if (curCaptureID_ != 0) {
        MEDIA_ERR_LOG("HStreamMetadata::Start, Already started with captureID: %{public}d", curCaptureID_);
        return CAMERA_INVALID_STATE;
    }
    int32_t ret = AllocateCaptureId(curCaptureID_);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("HStreamMetadata::Start Failed to allocate a captureId");
        return ret;
    }
    std::vector<uint8_t> ability;
    OHOS::Camera::MetadataUtils::ConvertMetadataToVec(cameraAbility_, ability);
    CaptureInfo captureInfo;
    captureInfo.streamIds_ = {streamId_};
    captureInfo.captureSetting_ = ability;
    captureInfo.enableShutterCallback_ = false;
    MEDIA_INFO_LOG("HStreamMetadata::Start Starting with capture ID: %{public}d", curCaptureID_);
    CamRetCode rc = (CamRetCode)(streamOperator_->Capture(curCaptureID_, captureInfo, true));
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        ReleaseCaptureId(curCaptureID_);
        curCaptureID_ = 0;
        MEDIA_ERR_LOG("HStreamMetadata::Start Failed with error Code:%{public}d", rc);
        ret = HdiToServiceError(rc);
    }
    return ret;
}

int32_t HStreamMetadata::Stop()
{
    CAMERA_SYNC_TRACE;

    if (streamOperator_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
    if (curCaptureID_ == 0) {
        MEDIA_ERR_LOG("HStreamMetadata::Stop, Stream not started yet");
        return CAMERA_INVALID_STATE;
    }
    int32_t ret = CAMERA_OK;
    CamRetCode rc = (CamRetCode)(streamOperator_->CancelCapture(curCaptureID_));
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamMetadata::Stop Failed with errorCode:%{public}d, curCaptureID_: %{public}d",
                      rc, curCaptureID_);
        ret = HdiToServiceError(rc);
    }
    ReleaseCaptureId(curCaptureID_);
    curCaptureID_ = 0;
    return ret;
}

int32_t HStreamMetadata::Release()
{
    return HStreamCommon::Release();
}

void HStreamMetadata::DumpStreamInfo(std::string& dumpString)
{
    dumpString += "metadata stream:\n";
    HStreamCommon::DumpStreamInfo(dumpString);
}
} // namespace Standard
} // namespace OHOS
