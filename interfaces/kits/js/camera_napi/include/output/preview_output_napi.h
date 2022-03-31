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

#ifndef PREVIEW_OUTPUT_NAPI_H_
#define PREVIEW_OUTPUT_NAPI_H_

#include <securec.h>

#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "output/preview_output.h"
#include "input/camera_manager.h"

#include "hilog/log.h"
#include "camera_napi_utils.h"

#include <cinttypes>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "surface_utils.h"


namespace OHOS {
namespace CameraStandard {
static const char CAMERA_PREVIEW_OUTPUT_NAPI_CLASS_NAME[] = "PreviewOutput";

class PreviewOutputCallback : public PreviewCallback {
public:
    PreviewOutputCallback(napi_env env);
    ~PreviewOutputCallback() = default;

    void OnFrameStarted() const override;
    void OnFrameEnded(const int32_t frameCount) const override;
    void OnError(const int32_t errorCode) const override;
    void SetCallbackRef(const std::string &eventType, const napi_ref &callbackRef);

private:
    void UpdateJSCallback(std::string propName, const int32_t value) const;
    void UpdateJSCallbackAsync(std::string propName, const int32_t value) const;

    napi_env env_;
    napi_ref frameStartCallbackRef_ = nullptr;
    napi_ref frameEndCallbackRef_ = nullptr;
    napi_ref errorCallbackRef_ = nullptr;
};

struct PreviewOutputCallbackInfo {
    std::string eventName_;
    int32_t value_;
    const PreviewOutputCallback *listener_;
    PreviewOutputCallbackInfo(std::string eventName, int32_t value, const PreviewOutputCallback *listener)
        : eventName_(eventName), value_(value), listener_(listener) {}
};

class PreviewOutputNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreatePreviewOutput(napi_env env, uint64_t surfaceId);
    static bool IsPreviewOutput(napi_env env, napi_value obj);
    PreviewOutputNapi();
    ~PreviewOutputNapi();
    sptr<CaptureOutput> GetPreviewOutput();

private:
    static void PreviewOutputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value PreviewOutputNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value JSonFunc(napi_env env, napi_callback_info info);

    napi_env env_;
    napi_ref wrapper_;
    uint64_t surfaceId_;
    sptr<CaptureOutput> previewOutput_;

    static napi_ref sConstructor_;
    static uint64_t sSurfaceId_;
    static sptr<CaptureOutput> sPreviewOutput_;
    std::shared_ptr<PreviewOutputCallback> previewCallback_;
};

struct PreviewOutputAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    PreviewOutputNapi* objectInfo;
    bool status;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* PREVIEW_OUTPUT_NAPI_H_ */
