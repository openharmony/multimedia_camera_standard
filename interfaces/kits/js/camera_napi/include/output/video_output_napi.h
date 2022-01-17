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

#ifndef VIDEO_OUTPUT_NAPI_H_
#define VIDEO_OUTPUT_NAPI_H_
#include <securec.h>

#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "output/video_output.h"
#include "hilog/log.h"
#include "camera_napi_utils.h"
#include "input/camera_manager.h"

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
static const std::string CAMERA_VIDEO_OUTPUT_NAPI_CLASS_NAME = "VideoOutput";

class SurfaceListener : public IBufferConsumerListener {
public:
    void OnBufferAvailable() override;
    int32_t SaveData(const char *buffer, int32_t size);
    void SetConsumerSurface(sptr<Surface> captureSurface);

private:
    sptr<Surface> captureSurface_;
    std::string photoPath;
};

class VideoOutputNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateVideoOutput(napi_env env, uint64_t surfaceId);
    static bool IsVideoOutput(napi_env env, napi_value obj);
    VideoOutputNapi();
    ~VideoOutputNapi();
    sptr<CaptureOutput> GetVideoOutput();

private:
    static void VideoOutputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value VideoOutputNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);

    static uint64_t sSurfaceId_;
    static napi_ref sConstructor_;
    static sptr<CaptureOutput> sVideoOutput_;
    static sptr<SurfaceListener> listener;

    std::vector<std::string> callbackList_;
    void RegisterCallback(napi_env env, napi_ref callbackRef);

    napi_env env_;
    napi_ref wrapper_;
    uint64_t surfaceId_;
    sptr<CaptureOutput> videoOutput_;
};

struct VideoOutputAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    VideoOutputNapi* objectInfo;
    bool status;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* VIDEO_OUTPUT_NAPI_H_ */
