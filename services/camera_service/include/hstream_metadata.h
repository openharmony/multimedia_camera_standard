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

#ifndef OHOS_CAMERA_H_STREAM_METADATA_H
#define OHOS_CAMERA_H_STREAM_METADATA_H

#include "camera_metadata_info.h"
#include "display_type.h"
#include "hstream_metadata_stub.h"
#include "hstream_common.h"
#include "istream_operator.h"

#include <refbase.h>
#include <iostream>

namespace OHOS {
namespace CameraStandard {
class HStreamMetadata : public HStreamMetadataStub, public HStreamCommon {
public:
    HStreamMetadata(sptr<OHOS::IBufferProducer> producer, int32_t format);
    ~HStreamMetadata();

    int32_t LinkInput(sptr<Camera::IStreamOperator> streamOperator,
        std::shared_ptr<Camera::CameraMetadata> cameraAbility, int32_t streamId) override;
    void SetStreamInfo(std::shared_ptr<Camera::StreamInfo> streamInfo) override;
    int32_t Release() override;
    int32_t Start() override;
    int32_t Stop() override;
    void DumpStreamInfo(std::string& dumpString) override;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_STREAM_METADATA_H
