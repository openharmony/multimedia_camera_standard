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

#ifndef OHOS_CAMERA_HCAPTURE_SESSION_CALLBACK_PROXY_H
#define OHOS_CAMERA_HCAPTURE_SESSION_CALLBACK_PROXY_H

#include "iremote_proxy.h"
#include "icapture_session_callback.h"

namespace OHOS {
namespace CameraStandard {
class HCaptureSessionCallbackProxy : public IRemoteProxy<ICaptureSessionCallback> {
public:
    explicit HCaptureSessionCallbackProxy(const sptr<IRemoteObject> &impl);
    virtual ~HCaptureSessionCallbackProxy() = default;

    int32_t OnError(int32_t errorCode) override;

private:
    static inline BrokerDelegator<HCaptureSessionCallbackProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HCAPTURE_SESSION_CALLBACK_PROXY_H
