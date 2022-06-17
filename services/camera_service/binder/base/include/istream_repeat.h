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

#ifndef OHOS_CAMERA_ISTREAM_REPEAT_H
#define OHOS_CAMERA_ISTREAM_REPEAT_H

#include "istream_common.h"
#include "istream_repeat_callback.h"

namespace OHOS {
namespace CameraStandard {
class IStreamRepeat : public IStreamCommon {
public:
    virtual int32_t Start() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t SetFps(float Fps) = 0;

    virtual int32_t SetCallback(sptr<IStreamRepeatCallback> &callback) = 0;

    virtual int32_t Release() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamRepeat");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_REPEAT_H
