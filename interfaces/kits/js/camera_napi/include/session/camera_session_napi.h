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

#ifndef CAMERA_SESSION_NAPI_H_
#define CAMERA_SESSION_NAPI_H_

#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "hilog/log.h"
#include "camera_napi_utils.h"

#include "input/camera_manager.h"
#include "input/camera_info.h"
#include "session/capture_session.h"

#include "input/camera_input_napi.h"
#include "output/photo_output_napi.h"
#include "output/preview_output_napi.h"
#include "output/video_output_napi.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

namespace OHOS {
namespace CameraStandard {
static const std::string CAMERA_SESSION_NAPI_CLASS_NAME = "CaptureSession";

class SessionCallbackListener : public SessionCallback {
public:
    SessionCallbackListener(napi_env env, napi_ref ref) : env_(env), callbackRef_(ref) {}
    ~SessionCallbackListener() = default;
    void OnError(int32_t errorCode) override;

private:
    void OnErrorCallback(int32_t errorCode) const;
    void OnErrorCallbackAsync(int32_t errorCode) const;

    napi_env env_;
    napi_ref callbackRef_ = nullptr;
};

struct SessionCallbackInfo {
    int32_t errorCode_;
    const SessionCallbackListener *listener_;
    SessionCallbackInfo(int32_t errorCode, const SessionCallbackListener *listener)
        : errorCode_(errorCode), listener_(listener) {}
};

class CameraSessionNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateCameraSession(napi_env env);
    CameraSessionNapi();
    ~CameraSessionNapi();

private:
    static void CameraSessionNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value CameraSessionNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value BeginConfig(napi_env env, napi_callback_info info);
    static napi_value CommitConfig(napi_env env, napi_callback_info info);

    static napi_value AddInput(napi_env env, napi_callback_info info);
    static napi_value RemoveInput(napi_env env, napi_callback_info info);

    static napi_value AddOutput(napi_env env, napi_callback_info info);
    static napi_value RemoveOutput(napi_env env, napi_callback_info info);

    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);

    napi_env env_;
    napi_ref wrapper_;
    sptr<CaptureSession> cameraSession_;

    static thread_local napi_ref sConstructor_;
    static sptr<CaptureSession> sCameraSession_;
};

struct CameraSessionAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    napi_value object;
    CameraSessionNapi* objectInfo;
    sptr<CaptureInput> cameraInput;
    sptr<CaptureOutput> cameraOutput;
    bool status;
    std::string errorMsg;
    bool bRetBool;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_SESSION_NAPI_H_ */
