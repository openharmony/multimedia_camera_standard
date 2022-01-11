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

#include <uv.h>

#include "input/camera_info_napi.h"
#include "input/camera_manager_callback_napi.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CameraNapi"};
    const std::string CAMERA_STATUS_CALLBACK_NAME = "cameraStatus";
}

CameraManagerCallbackNapi::CameraManagerCallbackNapi(napi_env env): env_(env)
{}

CameraManagerCallbackNapi::~CameraManagerCallbackNapi()
{
}

void CameraManagerCallbackNapi::SaveCallbackReference(const std::string &callbackName, napi_value args)
{
    napi_ref callback = nullptr;
    const int32_t refCount = 1;
    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CAMERA_NAPI_ASSERT_EQUAL(status == napi_ok && callback != nullptr,
        "CameraManagerCallbackNapi: creating reference for callback fail");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    if (callbackName == CAMERA_STATUS_CALLBACK_NAME) {
    CameraStatusInfoCallback_ = cb;
    } else {
        MEDIA_ERR_LOG("CameraManagerCallbackNapi: Unknown callback type: %{public}s", callbackName.c_str());
    }
}
void CameraManagerCallbackNapi::OnCameraStatusChanged(const CameraStatusInfo &cameraStatusInfo) const
{
    MEDIA_DEBUG_LOG("CameraManagerCallbackNapi:OnCameraStatusChanged is called");
    MEDIA_DEBUG_LOG("CameraManagerCallbackNapi: CameraDeviceStatus: %{public}d", cameraStatusInfo.cameraStatus);
    sptr<CameraInfo> cameraInfo = cameraStatusInfo.cameraInfo;

    std::unique_ptr<CameraManagerJsCallback> cb = std::make_unique<CameraManagerJsCallback>();
    CAMERA_NAPI_ASSERT_EQUAL(cb != nullptr, "No memory");
    cb->callback = CameraStatusInfoCallback_;
    cb->callbackName = CAMERA_STATUS_CALLBACK_NAME;
    cb->cameraStatusInfo_ = cameraStatusInfo;
    OnJsCallbackCameraStatusInfo(cb);
    return;
}
void CameraManagerCallbackNapi::OnFlashlightStatusChanged(const std::string &cameraID,
    const FlashlightStatus flashStatus) const
{
}

static void SetValueInt32(const napi_env& env, const std::string& fieldStr, const int intValue, const napi_value& result)
{
    napi_value value = nullptr;
    napi_create_int32(env, intValue, &value);
    napi_set_named_property(env, result, fieldStr.c_str(), value);
}

static void NativeCameraStatusInfoToJsObj(const napi_env& env, napi_value& jsObj,
    const CameraStatusInfo &cameraStatusInfo)
{
    napi_create_object(env, &jsObj);

    napi_value cameraStatusInfoNapi = nullptr;
    cameraStatusInfoNapi = CameraInfoNapi::CreateCameraObj(env, cameraStatusInfo.cameraInfo);
    napi_set_named_property(env, jsObj, "camera", cameraStatusInfoNapi);
    SetValueInt32(env, "status", static_cast<int32_t>(cameraStatusInfo.cameraStatus), jsObj);
}

void CameraManagerCallbackNapi::OnJsCallbackCameraStatusInfo(const std::unique_ptr<CameraManagerJsCallback> &jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_ERR_LOG("CameraManagerCallbackNapi: OnJsCallbackCameraStatusInfos: No memory");
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb.get());

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CameraManagerJsCallback *event = reinterpret_cast<CameraManagerJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_DEBUG_LOG("CameraManagerCallbackNapi: JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CAMERA_NAPI_CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CAMERA_NAPI_CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr,
                                            "%{public}s get reference value fail",
                                            request.c_str());

            // Call back function
            napi_value args[1] = { nullptr };
            NativeCameraStatusInfoToJsObj(env, args[0], event->cameraStatusInfo_);
            CAMERA_NAPI_CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                "%{public}s fail to create camerastatus callback", request.c_str());

            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CAMERA_NAPI_CHECK_AND_BREAK_LOG(nstatus == napi_ok,
                                            "%{public}s fail to call camerastatusinfo callback",
                                            request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to execute libuv work queue");
        delete work;
    }
}
} // namespace CameraStandard
} // namespace OHOS
