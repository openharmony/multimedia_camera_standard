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

#ifndef PHOTO_OUTPUT_NAPI_H_
#define PHOTO_OUTPUT_NAPI_H_

#include <cinttypes>
#include <securec.h>

#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "input/camera_manager.h"
#include "input/camera_info.h"
#include "output/photo_output.h"

#include "hilog/log.h"
#include "camera_napi_utils.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "image_receiver.h"

namespace OHOS {
namespace CameraStandard {
static const char CAMERA_PHOTO_OUTPUT_NAPI_CLASS_NAME[] = "PhotoOutput";

struct CallbackInfo {
    int32_t captureID;
    uint64_t timestamp = 0;
    int32_t frameCount = 0;
    int32_t errorCode;
};

class PhotoOutputCallback : public PhotoCallback {
public:
    PhotoOutputCallback(napi_env env);
    ~PhotoOutputCallback() = default;

    void OnCaptureStarted(const int32_t captureID) const override;
    void OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const override;
    void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const override;
    void OnCaptureError(const int32_t captureId, const int32_t errorCode) const override;
    void SetCallbackRef(const std::string &eventType, const napi_ref &callbackRef);

private:
    void UpdateJSCallback(std::string propName, const CallbackInfo &info) const;

    napi_env env_;
    napi_ref captureStartCallbackRef_ = nullptr;
    napi_ref captureEndCallbackRef_ = nullptr;
    napi_ref frameShutterCallbackRef_ = nullptr;
    napi_ref errorCallbackRef_ = nullptr;
};

class PhotoOutputNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreatePhotoOutput(napi_env env, std::string surfaceId);
    static bool IsPhotoOutput(napi_env env, napi_value obj);
    PhotoOutputNapi();
    ~PhotoOutputNapi();

    sptr<CaptureOutput> GetPhotoOutput();
    std::string GetSurfaceId();

private:
    static void PhotoOutputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value PhotoOutputNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value Capture(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);

    static napi_ref sConstructor_;
    static std::string sSurfaceId_;
    static sptr<CaptureOutput> sPhotoOutput_;

    napi_env env_;
    napi_ref wrapper_;
    std::string surfaceId_;
    sptr<CaptureOutput> photoOutput_;
    std::shared_ptr<PhotoOutputCallback> photoCallback_ = nullptr;
};

struct PhotoOutputAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    std::string surfaceId;
    int32_t quality = -1;
    int32_t mirror = -1;
    double latitude = -1.0;
    double longitude = -1.0;
    int32_t rotation = -1;
    PhotoOutputNapi* objectInfo;
    int32_t status;
    bool hasPhotoSettings = false;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* PHOTO_OUTPUT_NAPI_H_ */
