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

#ifndef CAMERA_SIZE_NAPI_H_
#define CAMERA_SIZE_NAPI_H_

#include "camera_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "hilog/log.h"
#include "camera_napi_utils.h"
#include "input/camera_input.h"

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
static const char CAMERA_SIZE_NAPI_CLASS_NAME[] = "Size";

class CameraSizeNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateCameraSize(napi_env env, CameraPicSize &cameraPicSize);

    CameraSizeNapi();
    ~CameraSizeNapi();

    static CameraPicSize *sCameraPicSize_;

private:
    static void CameraSizeNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value CameraSizeNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetCameraSizeWidth(napi_env env, napi_callback_info info);
    static napi_value GetCameraSizeHeight(napi_env env, napi_callback_info info);

    napi_env env_;
    napi_ref wrapper_;
    CameraPicSize cameraPicSize_;

    static thread_local napi_ref sConstructor_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_SIZE_NAPI_H_ */
