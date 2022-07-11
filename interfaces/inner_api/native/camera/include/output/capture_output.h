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

#ifndef OHOS_CAMERA_CAPTURE_OUTPUT_H
#define OHOS_CAMERA_CAPTURE_OUTPUT_H

#include <refbase.h>
#include "istream_common.h"

namespace OHOS {
namespace CameraStandard {
enum CaptureOutputType {
    CAPTURE_OUTPUT_TYPE_PREVIEW,
    CAPTURE_OUTPUT_TYPE_PHOTO,
    CAPTURE_OUTPUT_TYPE_VIDEO,
    CAPTURE_OUTPUT_TYPE_METADATA,
    CAPTURE_OUTPUT_TYPE_MAX
};
static const char *g_captureOutputTypeString[CAPTURE_OUTPUT_TYPE_MAX] = {"Preview", "Photo", "Video", "Metadata"};
class CaptureSession;
class CaptureOutput : public RefBase {
public:
    explicit CaptureOutput(CaptureOutputType OutputType, StreamType streamType,
                           sptr<IStreamCommon> stream);
    virtual ~CaptureOutput() {}

    /**
     * @brief Releases the instance of CaptureOutput.
     */
    virtual void Release() = 0;

    CaptureOutputType GetOutputType();
    const char *GetOutputTypeString();
    StreamType GetStreamType();
    sptr<IStreamCommon> GetStream();
    CaptureSession *GetSession();
    void SetSession(CaptureSession *captureSession);

private:
    CaptureOutputType outputType_;
    StreamType streamType_;
    sptr<IStreamCommon> stream_;
    CaptureSession *session_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAPTURE_OUTPUT_H
