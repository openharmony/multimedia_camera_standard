/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "permission_status_change_cb.h"
#include "camera_util.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
void PermissionStatusChangeCb::PermStateChangeCallback(PermStateChangeInfo& result)
{
    MEDIA_DEBUG_LOG("%{public}s changed.", result.permissionName.c_str());
    if ((result.PermStateChangeType == 0) && (curCaptureSession != nullptr))
    {
        curCaptureSession->ReleaseInner();
    }
}
} // namespace CameraStandard
} // namespace OHOS