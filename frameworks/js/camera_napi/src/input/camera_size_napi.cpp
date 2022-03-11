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


#include "input/camera_size_napi.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

napi_ref CameraSizeNapi::sConstructor_ = nullptr;
CameraPicSize *CameraSizeNapi::sCameraPicSize_ = nullptr;

CameraSizeNapi::CameraSizeNapi() : env_(nullptr), wrapper_(nullptr)
{
    CameraPicSize_ = nullptr;
}

CameraSizeNapi::~CameraSizeNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void CameraSizeNapi::CameraSizeNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    CameraSizeNapi *cameraSizeNapi = reinterpret_cast<CameraSizeNapi*>(nativeObject);
    if (cameraSizeNapi != nullptr) {
        cameraSizeNapi->~CameraSizeNapi();
    }
}

napi_value CameraSizeNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor camera_size_props[] = {
        DECLARE_NAPI_GETTER("width", GetCameraSizeWidth),
        DECLARE_NAPI_GETTER("height", GetCameraSizeHeight)
    };

    status = napi_define_class(env, CAMERA_SIZE_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               CameraSizeNapiConstructor, nullptr,
                               sizeof(camera_size_props) / sizeof(camera_size_props[PARAM0]),
                               camera_size_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_SIZE_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

// Constructor callback
napi_value CameraSizeNapi::CameraSizeNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraSizeNapi> obj = std::make_unique<CameraSizeNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->cameraPicSize_ = sCameraPicSize_;
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               CameraSizeNapi::CameraSizeNapiDestructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                MEDIA_ERR_LOG("Failure wrapping js to native napi");
            }
        }
    }

    return result;
}

napi_value CameraSizeNapi::CreateCameraSize(napi_env env, CameraPicSize &cameraPicSize)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sCameraPicSize_ = &cameraPicSize;
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create Camera obj instance");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value CameraSizeNapi::GetCameraSizeWidth(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    CameraSizeNapi* obj = nullptr;
    uint32_t cameraSizeWidth = 0;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_ERR_LOG("Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        cameraSizeWidth = obj->cameraPicSize_->width;
        status = napi_create_uint32(env, cameraSizeWidth, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        } else {
            MEDIA_ERR_LOG("Failed to get CameraSizeWidth!, errorCode : %{public}d", status);
        }
    }

    return undefinedResult;
}

napi_value CameraSizeNapi::GetCameraSizeHeight(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    CameraSizeNapi* obj = nullptr;
    uint32_t cameraSizeHeight = 0;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_ERR_LOG("Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        cameraSizeHeight = obj->cameraPicSize_->height;
        status = napi_create_uint32(env, cameraSizeHeight, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        } else {
            MEDIA_ERR_LOG("Failed to get CameraSizeHeight!, errorCode : %{public}d", status);
        }
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS
