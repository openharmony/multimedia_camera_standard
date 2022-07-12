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

#ifndef METADATA_OUTPUT_NAPI_H_
#define METADATA_OUTPUT_NAPI_H_

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
static const char CAMERA_METADATA_OUTPUT_NAPI_CLASS_NAME[] = "MetadataOutput";

class MetadataOutputCallback : public MetadataObjectCallback {
public:
    explicit MetadataOutputCallback(napi_env env);
    ~MetadataOutputCallback() = default;

    void OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const override;
    void SetCallbackRef(const std::string &eventType, const napi_ref &callbackRef);

private:
    void OnMetadataObjectsAvailableCallback(const std::vector<sptr<MetadataObject>> metadataObjList) const;
    napi_env env_;
    napi_ref metadataObjectsAvailableCallbackRef_ = nullptr;
};

struct  MetadataOutputCallbackInfo {
    const std::vector<sptr<MetadataObject>> info_;
    const MetadataOutputCallback *listener_;
    MetadataOutputCallbackInfo(std::vector<sptr<MetadataObject>> metadataObjList,
        const MetadataOutputCallback *listener)
        : info_(metadataObjList), listener_(listener) {}
};

class MetadataOutputNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateMetadataOutput(napi_env env);
    MetadataOutputNapi();
    ~MetadataOutputNapi();
    sptr<MetadataOutput> GetMetadataOutput();
    static bool IsMetadataOutput(napi_env env, napi_value obj);

private:
    static void MetadataOutputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value MetadataOutputNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetSupportedMetadataObjectTypes(napi_env env, napi_callback_info info);
    static napi_value SetCapturingMetadataObjectTypes(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);

    static thread_local napi_ref sConstructor_;
    static thread_local sptr<MetadataOutput> sMetadataOutput_;

    napi_env env_;
    napi_ref wrapper_;
    sptr<MetadataOutput> metadataOutput_;
    std::shared_ptr<MetadataOutputCallback> metadataCallback_ = nullptr;
};

struct MetadataOutputAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    MetadataOutputNapi* objectInfo;
    bool bRetBool;
    bool isSupported = false;
    int32_t status;
    std::string errorMsg;
    std::vector<MetadataObjectType> SupportedMetadataObjectTypes;
    std::vector<MetadataObjectType> setSupportedMetadataObjectTypes;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* METADATA_OUTPUT_NAPI_H_ */
