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

#include "input/camera_info_napi.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

napi_ref CameraInfoNapi::sConstructor_ = nullptr;
sptr<CameraInfo> CameraInfoNapi::sCameraInfo_ = nullptr;

CameraInfoNapi::CameraInfoNapi() : env_(nullptr), wrapper_(nullptr)
{
}

CameraInfoNapi::~CameraInfoNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void CameraInfoNapi::CameraInfoNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    CameraInfoNapi *cameraObj = reinterpret_cast<CameraInfoNapi*>(nativeObject);
    if (cameraObj != nullptr) {
        cameraObj->~CameraInfoNapi();
    }
}

napi_value CameraInfoNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor camera_object_props[] = {
        DECLARE_NAPI_GETTER("cameraId", GetCameraId),
        DECLARE_NAPI_GETTER("cameraPosition", GetCameraPosition),
        DECLARE_NAPI_GETTER("cameraType", GetCameraType),
        DECLARE_NAPI_GETTER("connectionType", GetConnectionType)
    };

    status = napi_define_class(env, CAMERA_OBJECT_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               CameraInfoNapiConstructor, nullptr,
                               sizeof(camera_object_props) / sizeof(camera_object_props[PARAM0]),
                               camera_object_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_OBJECT_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

// Constructor callback
napi_value CameraInfoNapi::CameraInfoNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraInfoNapi> obj = std::make_unique<CameraInfoNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->cameraInfo_ = sCameraInfo_;
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               CameraInfoNapi::CameraInfoNapiDestructor, nullptr, &(obj->wrapper_));
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

napi_value CameraInfoNapi::CreateCameraObj(napi_env env, sptr<CameraInfo> cameraInfo)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sCameraInfo_ = cameraInfo;
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sCameraInfo_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create Camera obj instance");
        }
    }

    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraInfoNapi::GetCameraId(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    CameraInfoNapi* obj = nullptr;
    std::string cameraId = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_ERR_LOG("Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        cameraId = obj->cameraInfo_->GetID();
        status = napi_create_string_utf8(env, cameraId.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        } else {
            MEDIA_ERR_LOG("Failed to get camera id!, errorCode : %{public}d", status);
        }
    }

    return undefinedResult;
}

napi_value CameraInfoNapi::GetCameraPosition(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    CameraInfoNapi* obj = nullptr;
    int32_t jsCameraPosition = CAMERA_POSITION_UNSPECIFIED;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_ERR_LOG("Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        camera_position_enum_t nativeCameraPosition = obj->cameraInfo_->GetPosition();
        CameraNapiUtils::MapCameraPositionEnum(nativeCameraPosition, jsCameraPosition);
        status = napi_create_int32(env, jsCameraPosition, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        } else {
            MEDIA_ERR_LOG("Failed to get cameraPosition!, errorCode : %{public}d", status);
        }
    }

    return undefinedResult;
}

napi_value CameraInfoNapi::GetCameraType(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    CameraInfoNapi* obj = nullptr;
    int32_t jsCameraType = CAMERA_TYPE_UNSPECIFIED;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_ERR_LOG("Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        camera_type_enum_t nativeCameraType = obj->cameraInfo_->GetCameraType();
        CameraNapiUtils::MapCameraTypeEnum(nativeCameraType, jsCameraType);
        if (jsCameraType == -1) {
            MEDIA_ERR_LOG("Camera type is not a recognized camera type in JS");
            return undefinedResult;
        }
        status = napi_create_int32(env, jsCameraType, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        } else {
            MEDIA_ERR_LOG("Failed to get cameraType!, errorCode : %{public}d", status);
        }
    }

    return undefinedResult;
}

napi_value CameraInfoNapi::GetConnectionType(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    CameraInfoNapi* obj = nullptr;
    int32_t jsConnectionType = CAMERA_CONNECTION_BUILT_IN;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_ERR_LOG("Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        camera_connection_type_t nativeConnectionType = obj->cameraInfo_->GetConnectionType();
        CameraNapiUtils::MapCameraConnectionTypeEnum(nativeConnectionType, jsConnectionType);
        status = napi_create_int32(env, jsConnectionType, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        } else {
            MEDIA_ERR_LOG("Failed to get connectionType!, errorCode : %{public}d", status);
        }
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS
