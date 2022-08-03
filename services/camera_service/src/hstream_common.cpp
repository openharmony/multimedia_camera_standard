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

#include "hstream_common.h"

#include "camera_util.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
HStreamCommon::HStreamCommon(StreamType streamType, sptr<OHOS::IBufferProducer> producer, int32_t format)
{
    streamId_ = 0;
    curCaptureID_ = 0;
    isReleaseStream_ = false;
    streamOperator_ = nullptr;
    cameraAbility_ = nullptr;
    producer_ = producer;
    width_ = producer->GetDefaultWidth();
    height_ = producer->GetDefaultHeight();
    format_ = format;
    streamType_ = streamType;
}

HStreamCommon::~HStreamCommon()
{}

int32_t HStreamCommon::SetReleaseStream(bool isReleaseStream)
{
    isReleaseStream_ = isReleaseStream;
    return CAMERA_OK;
}

bool HStreamCommon::IsReleaseStream()
{
    return isReleaseStream_;
}

int32_t HStreamCommon::GetStreamId()
{
    return streamId_;
}

StreamType HStreamCommon::GetStreamType()
{
    return streamType_;
}

int32_t HStreamCommon::LinkInput(sptr<IStreamOperator> streamOperator,
                                 std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility, int32_t streamId)
{
    if (streamOperator == nullptr || cameraAbility == nullptr) {
        MEDIA_ERR_LOG("HStreamCommon::LinkInput streamOperator is null");
        return CAMERA_INVALID_ARG;
    }
    if (!IsValidSize(cameraAbility, format_, width_, height_)) {
        return CAMERA_INVALID_SESSION_CFG;
    }
    streamId_ = streamId;
    streamOperator_ = streamOperator;
    cameraAbility_ = cameraAbility;
    return CAMERA_OK;
}

void HStreamCommon::SetStreamInfo(StreamInfo &streamInfo)
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
    MEDIA_INFO_LOG("HStreamCommon::SetStreamInfo pixelFormat is %{public}d", pixelFormat);
    streamInfo.streamId_ = streamId_;
    streamInfo.width_ = width_;
    streamInfo.height_ = height_;
    streamInfo.format_ = pixelFormat;
    streamInfo.minFrameDuration_ = 0;
    streamInfo.tunneledMode_ = true;
    streamInfo.bufferQueue_ = new BufferProducerSequenceable(producer_);
    streamInfo.dataspace_ = CAMERA_COLOR_SPACE;
}

int32_t HStreamCommon::Release()
{
    streamId_ = 0;
    curCaptureID_ = 0;
    streamOperator_ = nullptr;
    cameraAbility_ = nullptr;
    producer_ = nullptr;
    return CAMERA_OK;
}

void HStreamCommon::DumpStreamInfo(std::string& dumpString)
{
    StreamInfo curStreamInfo;
    SetStreamInfo(curStreamInfo);
    dumpString += "release status:[" + std::to_string(isReleaseStream_) + "]:\n";
    dumpString += "stream info: \n";
    dumpString += "    Buffer producer Id:[" + std::to_string(curStreamInfo.bufferQueue_->producer_->GetUniqueId());
    dumpString += "]    stream Id:[" + std::to_string(curStreamInfo.streamId_);
    std::map<int, std::string>::const_iterator iter =
        g_cameraFormat.find(format_);
    if (iter != g_cameraFormat.end()) {
        dumpString += "]    format:[" + iter->second;
    }
    dumpString += "]    width:[" + std::to_string(curStreamInfo.width_);
    dumpString += "]    height:[" + std::to_string(curStreamInfo.height_);
    dumpString += "]    dataspace:[" + std::to_string(curStreamInfo.dataspace_);
    dumpString += "]    StreamType:[" + std::to_string(curStreamInfo.intent_);
    dumpString += "]    TunnelMode:[" + std::to_string(curStreamInfo.tunneledMode_);
    dumpString += "]    Encoding Type:[" + std::to_string(curStreamInfo.encodeType_) + "]:\n";
}
} // namespace CameraStandard
} // namespace OHOS
