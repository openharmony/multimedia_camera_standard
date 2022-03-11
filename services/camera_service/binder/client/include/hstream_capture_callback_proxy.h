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

#ifndef OHOS_CAMERA_HSTREAM_CAPTURE_CALLBACK_PROXY_H
#define OHOS_CAMERA_HSTREAM_CAPTURE_CALLBACK_PROXY_H

#include "iremote_proxy.h"
#include "istream_capture_callback.h"

namespace OHOS {
namespace CameraStandard {
class HStreamCaptureCallbackProxy : public IRemoteProxy<IStreamCaptureCallback> {
public:
    explicit HStreamCaptureCallbackProxy(const sptr<IRemoteObject> &impl);

    virtual ~HStreamCaptureCallbackProxy() = default;

    int32_t OnCaptureStarted(int32_t captureId) override;

    int32_t OnCaptureEnded(int32_t captureId, int32_t frameCount) override;

    int32_t OnCaptureError(int32_t captureId, int32_t errorType) override;

    int32_t OnFrameShutter(int32_t captureId, uint64_t timestamp) override;

private:
    static inline BrokerDelegator<HStreamCaptureCallbackProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HSTREAM_CAPTURE_CALLBACK_PROXY_H