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

#ifndef CAMERA_LISTENER_PROXY_H
#define CAMERA_LISTENER_PROXY_H

#include "input/i_standard_camera_listener.h"
#include "input/camera_death_recipient.h"
#include "nocopyable.h"

namespace OHOS {
namespace CameraStandard {
class CameraListenerProxy : public IRemoteProxy<IStandardCameraListener>, public NoCopyable {
public:
    explicit CameraListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~CameraListenerProxy();

private:
    static inline BrokerDelegator<CameraListenerProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // CAMERA_LISTENER_PROXY_H
