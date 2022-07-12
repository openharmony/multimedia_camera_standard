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

#ifndef METADATA_OBJECT_NAPI_H_
#define METADATA_OBJECT_NAPI_H_

#include <cinttypes>
#include <securec.h>

#include "camera_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "input/camera_manager.h"
#include "input/camera_info.h"

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

namespace OHOS {
namespace CameraStandard {
static const char CAMERA_METADATA_OBJECT_NAPI_CLASS_NAME[] = "MetadataObject";

class MetadataObjectNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    MetadataObjectNapi();
    ~MetadataObjectNapi();
    sptr<MetadataObject> GetMetadataObject();
    static napi_value CreateMetaFaceObj(napi_env env, sptr<MetadataObject> metaObj);

private:
    static void MetadataObjectNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value MetadataObjectNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetType(napi_env env, napi_callback_info info);
    static napi_value GetTimestamp(napi_env env, napi_callback_info info);
    static napi_value GetBoundingBox(napi_env env, napi_callback_info info);

    static thread_local napi_ref sConstructor_;

    napi_env env_;
    napi_ref wrapper_;
    sptr<MetadataObject> metadataObject_;
};

class MetadataFaceObjectNapi : public MetadataObjectNapi {
public:
    MetadataFaceObjectNapi();
    ~MetadataFaceObjectNapi();

private:
    napi_env env_;
};

struct MetadataObjectAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    MetadataObjectNapi* objectInfo;
    int32_t status;
    std::string errorMsg;
    MetadataObjectType metadataObjType;
    double metaTimestamp;
    Rect metaFace;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* METADATA_OBJECT_NAPI_H_ */