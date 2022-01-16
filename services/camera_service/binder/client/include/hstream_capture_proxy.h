/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_HSTREAM_CAPTURE_PROXY_H
#define OHOS_CAMERA_HSTREAM_CAPTURE_PROXY_H

#include "istream_capture.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace CameraStandard {
class HStreamCaptureProxy : public IRemoteProxy<IStreamCapture> {
public:
    explicit HStreamCaptureProxy(const sptr<IRemoteObject> &impl);

    virtual ~HStreamCaptureProxy() = default;

    int32_t Capture(const std::shared_ptr<CameraMetadata> &captureSettings) override;

    int32_t CancelCapture() override;

    int32_t Release() override;

    int32_t SetCallback(sptr<IStreamCaptureCallback> &callback) override;

private:
    static inline BrokerDelegator<HStreamCaptureProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HSTREAM_CAPTURE_PROXY_H
