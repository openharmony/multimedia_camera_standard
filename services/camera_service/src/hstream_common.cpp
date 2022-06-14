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

int32_t HStreamCommon::Release()
{
    streamId_ = 0;
    curCaptureID_ = 0;
    streamOperator_ = nullptr;
    cameraAbility_ = nullptr;
    producer_ = nullptr;
    return CAMERA_OK;
}
} // namespace CameraStandard
} // namespace OHOS
