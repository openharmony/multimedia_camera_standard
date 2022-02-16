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

#include "input/camera_napi.h"
#include "input/camera_manager_napi.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

napi_ref CameraManagerNapi::sConstructor_ = nullptr;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CameraManager"};
}

CameraManagerNapi::CameraManagerNapi() : env_(nullptr), wrapper_(nullptr)
{
}

CameraManagerNapi::~CameraManagerNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

// Constructor callback
napi_value CameraManagerNapi::CameraManagerNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraManagerNapi> obj = std::make_unique<CameraManagerNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->cameraManager_ = CameraManager::GetInstance();
            obj->cameraManager_->SetPermissionCheck(true);
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               CameraManagerNapi::CameraManagerNapiDestructor, nullptr, &(obj->wrapper_));
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

void CameraManagerNapi::CameraManagerNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    CameraManagerNapi *camera = reinterpret_cast<CameraManagerNapi*>(nativeObject);
    if (camera != nullptr) {
        camera->cameraManager_->SetPermissionCheck(false);
        camera->~CameraManagerNapi();
    }
}

napi_value CameraManagerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor camera_mgr_properties[] = {
        // CameraManager
        DECLARE_NAPI_FUNCTION("getCameras", GetCameras),
        DECLARE_NAPI_FUNCTION("createCameraInput", CreateCameraInputInstance)
    };

    status = napi_define_class(env, CAMERA_MANAGER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               CameraManagerNapiConstructor, nullptr,
                               sizeof(camera_mgr_properties) / sizeof(camera_mgr_properties[PARAM0]),
                               camera_mgr_properties, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_MANAGER_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value CameraManagerNapi::CreateCameraManagerInstance(napi_env env)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;

    status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        status = napi_new_instance(env, ctor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        } else {
            MEDIA_ERR_LOG("New instance could not be obtained");
        }
    }
    napi_get_undefined(env, &result);
    return result;
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    CameraManagerNapiAsyncContext &asyncContext)
{
    char buffer[PATH_MAX];
    const int32_t refCount = 1;
    napi_value result;
    size_t length = 0;
    auto context = &asyncContext;
    int32_t numValue;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    context->cameraId = "";
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_string) {
            if (napi_get_value_string_utf8(env, argv[i], buffer, PATH_MAX, &length) == napi_ok) {
                context->cameraId = string(buffer);
            } else {
                MEDIA_ERR_LOG("Could not able to read cameraId argument!");
            }
        } else if (i == PARAM0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &numValue);
            if (CameraNapiUtils::MapCameraPositionEnumFromJs(numValue, context->cameraPosition) == -1) {
                MEDIA_ERR_LOG("Unsupported camera position found");
                NAPI_ASSERT(env, false, "Type mismatch");
            } else {
                MEDIA_INFO_LOG("Camera position : %{public}d", context->cameraPosition);
            }
        } else if (i == PARAM1 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &numValue);
            if (CameraNapiUtils::MapCameraTypeEnumFromJs(numValue, context->cameraType) == -1) {
                MEDIA_ERR_LOG("Unsupported camera type found");
                NAPI_ASSERT(env, false, "Type mismatch");
            } else {
                MEDIA_INFO_LOG("Camera type : %{public}d", context->cameraType);
            }
        } else if (valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            MEDIA_ERR_LOG("Failed to get create camera input arguments!");
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

static napi_value CreateCameraJSArray(napi_env env, napi_status status,
    std::vector<sptr<CameraInfo>> cameraObjList)
{
    napi_value cameraArray = nullptr;
    napi_value camera = nullptr;

    if (cameraObjList.empty()) {
        MEDIA_ERR_LOG("cameraObjList is empty");
        return cameraArray;
    }

    status = napi_create_array(env, &cameraArray);
    if (status == napi_ok) {
        for (size_t i = 0; i < cameraObjList.size(); i++) {
            camera = CameraInfoNapi::CreateCameraObj(env, cameraObjList[i]);
            MEDIA_INFO_LOG("GetCameras CreateCameraObj success");
            if (camera == nullptr || napi_set_element(env, cameraArray, i, camera) != napi_ok) {
                MEDIA_ERR_LOG("Failed to create camera napi wrapper object");
                return nullptr;
            }
        }
    }
    return cameraArray;
}

void GetCamerasAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraManagerNapiAsyncContext*>(data);

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    jsContext->data = CreateCameraJSArray(env, status, context->cameraObjList);
    if (jsContext->data == nullptr) {
        napi_get_undefined(env, &jsContext->data);
        MEDIA_ERR_LOG("Failed to create napi cameraArray");
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraManagerNapi::GetCameras(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraManagerNapiAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetCameras");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraManagerNapiAsyncContext*>(data);
                context->cameraObjList = context->objectInfo->cameraManager_->GetCameras();
                MEDIA_INFO_LOG("GetCameras cameraManager_->GetCameras() : %{public}zu", context->cameraObjList.size());
                context->status = true;
            },
            GetCamerasAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("GetCameras napi_create_async_work failed ");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void CreateCameraInputAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraManagerNapiAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    if (context->status) {
        jsContext->data = CameraInputNapi::CreateCameraInput(env, context->cameraId,
            context->cameraInput);
    } else {
        jsContext->data = nullptr;
    }

    if (jsContext->data == nullptr) {
        napi_get_undefined(env, &jsContext->data);
        MEDIA_ERR_LOG("Failed to create camera input instance");
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraManagerNapi::CreateCameraInputInstance(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc >= ARGS_ONE && argc <= ARGS_THREE), "requires 3 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraManagerNapiAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status != napi_ok || asyncContext->objectInfo == nullptr) {
        MEDIA_ERR_LOG("napi_unwrap( ) failure!");
        return nullptr;
    }

    result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
    CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "CreateCameraInputInstance");
    status = napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto context = static_cast<CameraManagerNapiAsyncContext*>(data);
            context->cameraObjList = CameraManager::GetInstance()->GetCameras();
            sptr<CameraInfo> camInfo = nullptr;
            size_t i;
            for (i = 0; i < context->cameraObjList.size(); i += 1) {
                camInfo = context->cameraObjList[i];
                if (context->cameraId.empty()) {
                    if (camInfo->GetPosition() == context->cameraPosition &&
                        camInfo->GetCameraType() == context->cameraType) {
                        break;
                    }
                } else if (camInfo->GetID() == context->cameraId) {
                    break;
                }
            }
            if (camInfo != nullptr && i < context->cameraObjList.size()) {
                context->cameraInput = context->objectInfo->cameraManager_->CreateCameraInput(camInfo);
                context->status = true;
            } else {
                MEDIA_ERR_LOG("Error: unable to get camera Info!");
                context->status = false;
            }
        },
        CreateCameraInputAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}
} // namespace CameraStandard
} // namespace OHOS
