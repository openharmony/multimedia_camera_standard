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

#include "input/camera_napi.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

napi_ref CameraNapi::sConstructor_ = nullptr;

napi_ref CameraNapi::exposureModeRef_ = nullptr;
napi_ref CameraNapi::focusModeRef_ = nullptr;
napi_ref CameraNapi::flashModeRef_ = nullptr;
napi_ref CameraNapi::cameraFormatRef_ = nullptr;
napi_ref CameraNapi::cameraStatusRef_ = nullptr;
napi_ref CameraNapi::connectionTypeRef_ = nullptr;
napi_ref CameraNapi::cameraPositionRef_ = nullptr;
napi_ref CameraNapi::cameraTypeRef_ = nullptr;
napi_ref CameraNapi::imageRotationRef_ = nullptr;
napi_ref CameraNapi::errorUnknownRef_ = nullptr;
napi_ref CameraNapi::exposureStateRef_ = nullptr;
napi_ref CameraNapi::focusStateRef_ = nullptr;
napi_ref CameraNapi::qualityLevelRef_ = nullptr;

std::unordered_map<std::string, int32_t> mapImageRotation = {
    {"ROTATION_0", 0},
    {"ROTATION_90", 90},
    {"ROTATION_180", 180},
    {"ROTATION_270", 270},
};

std::unordered_map<std::string, int32_t> mapQualityLevel = {
    {"QUALITY_LEVEL_HIGH", 0},
    {"QUALITY_LEVEL_MEDIUM", 1},
    {"QUALITY_LEVEL_LOW", 2},
};

std::unordered_map<std::string, int32_t> mapFocusState = {
    {"FOCUS_STATE_SCAN", 0},
    {"FOCUS_STATE_FOCUSED", 1},
    {"FOCUS_STATE_UNFOCUSED", 2},
};

std::unordered_map<std::string, int32_t> mapExposureState = {
    {"EXPOSURE_STATE_SCAN", 0},
    {"EXPOSURE_STATE_CONVERGED", 1},
};

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CameraNapi"};
}

CameraNapi::CameraNapi() : env_(nullptr), wrapper_(nullptr)
{
}

CameraNapi::~CameraNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

// Constructor callback
napi_value CameraNapi::CameraNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraNapi> obj = std::make_unique<CameraNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               CameraNapi::CameraNapiDestructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                MEDIA_ERR_LOG("CameraNapiConstructor Failure wrapping js to native napi");
            }
        }
    }

    return result;
}

void CameraNapi::CameraNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    CameraNapi *camera = reinterpret_cast<CameraNapi*>(nativeObject);
    if (camera != nullptr) {
        CameraManager::GetInstance()->SetPermissionCheck(false);
        camera->~CameraNapi();
    }
}

napi_value CameraNapi::Init(napi_env env, napi_value exports)
{
    MEDIA_INFO_LOG("CameraNapi::Init()");
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;
    napi_property_descriptor camera_properties[] = {
        DECLARE_NAPI_FUNCTION("getCameraManagerTest", CreateCameraManagerInstance)
    };

    napi_property_descriptor camera_static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getCameraManager", CreateCameraManagerInstance),
        DECLARE_NAPI_STATIC_FUNCTION("createCaptureSession", CreateCameraSessionInstance),
        DECLARE_NAPI_STATIC_FUNCTION("createPreviewOutput", CreatePreviewOutputInstance),
        DECLARE_NAPI_STATIC_FUNCTION("createPhotoOutput", CreatePhotoOutputInstance),
        DECLARE_NAPI_STATIC_FUNCTION("createVideoOutput", CreateVideoOutputInstance),
        DECLARE_NAPI_PROPERTY("FlashMode", CreateFlashModeObject(env)),
        DECLARE_NAPI_PROPERTY("ExposureMode", CreateExposureModeObject(env)),
        DECLARE_NAPI_PROPERTY("ExposureState", CreateExposureStateEnum(env)),
        DECLARE_NAPI_PROPERTY("FocusMode", CreateFocusModeObject(env)),
        DECLARE_NAPI_PROPERTY("FocusState", CreateFocusStateEnum(env)),
        DECLARE_NAPI_PROPERTY("CameraPosition", CreateCameraPositionEnum(env)),
        DECLARE_NAPI_PROPERTY("CameraType", CreateCameraTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("ConnectionType", CreateConnectionTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("CameraFormat", CreateCameraFormatObject(env)),
        DECLARE_NAPI_PROPERTY("CameraStatus", CreateCameraStatusObject(env)),
        DECLARE_NAPI_PROPERTY("ImageRotation", CreateImageRotationEnum(env)),
        DECLARE_NAPI_PROPERTY("QualityLevel", CreateQualityLevelEnum(env)),
        DECLARE_NAPI_PROPERTY("CameraInputErrorCode", CreateErrorUnknownEnum(env)),
        DECLARE_NAPI_PROPERTY("CaptureSessionErrorCode", CreateErrorUnknownEnum(env)),
        DECLARE_NAPI_PROPERTY("PreviewOutputErrorCode", CreateErrorUnknownEnum(env)),
        DECLARE_NAPI_PROPERTY("PhotoOutputErrorCode", CreateErrorUnknownEnum(env)),
        DECLARE_NAPI_PROPERTY("VideoOutputErrorCode", CreateErrorUnknownEnum(env))
    };

    status = napi_define_class(env, CAMERA_LIB_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, CameraNapiConstructor,
                               nullptr, sizeof(camera_properties) / sizeof(camera_properties[PARAM0]),
                               camera_properties, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_LIB_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok && napi_define_properties(env, exports,
                sizeof(camera_static_prop) / sizeof(camera_static_prop[PARAM0]), camera_static_prop) == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_status CameraNapi::AddNamedProperty(napi_env env, napi_value object,
                                         const std::string name, int32_t enumValue)
{
    napi_status status;
    napi_value enumNapiValue;

    status = napi_create_int32(env, enumValue, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), enumNapiValue);
    }

    return status;
}

void CreateCameraManagerAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    MEDIA_INFO_LOG("CreateCameraManagerAsyncCallbackComplete called");
    auto context = static_cast<CameraNapiAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "CameraNapiAsyncContext is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;

    napi_get_undefined(env, &jsContext->error);

    MEDIA_INFO_LOG("CreateCameraManagerInstance created");
    jsContext->data = CameraManagerNapi::CreateCameraManagerInstance(env);

    if (jsContext->data == nullptr) {
        napi_get_undefined(env, &jsContext->data);
        MEDIA_ERR_LOG("Failed to Create camera manager napi instance");
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraNapi::CreateCameraManagerInstance(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("CreateCameraManagerInstance called");
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    MEDIA_INFO_LOG("CreateCameraManagerInstance argc %{public}zu", argc);

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();

    if (argc == ARGS_TWO) {
        CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM1], refCount, asyncContext->callbackRef);
    }
    CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "CreateCameraManagerInstance");
    status = napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {},
        CreateCameraManagerAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("CreateCameraManagerInstance napi_create_async_work failed");
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        MEDIA_INFO_LOG("CreateCameraManagerInstance napi_queue_async_work done");
        asyncContext.release();
    }
    MEDIA_ERR_LOG("CreateCameraManagerInstance return undefined");
    return result;
}

void CreateCameraSessionAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    MEDIA_INFO_LOG("CreateCameraSessionAsyncCallbackComplete called");
    auto context = static_cast<CameraNapiAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    jsContext->data = CameraSessionNapi::CreateCameraSession(env);
    if (jsContext->data == nullptr) {
        napi_get_undefined(env, &jsContext->data);
        MEDIA_ERR_LOG("Failed to create fetch CreateCameraSession result instance");
    }
    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraNapi::CreateCameraSessionInstance(napi_env env, napi_callback_info info)
{
    MEDIA_ERR_LOG("CreateCameraSessionInstance is called");
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);

    auto asyncContext = std::make_unique<CameraNapiAsyncContext>();
    if (argc == ARGS_TWO) {
        CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM1], refCount, asyncContext->callbackRef);
    }

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
    CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "CreateCameraSessionInstance");
    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void* data) {},
        CreateCameraSessionAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

static napi_value ConvertPhotoOutputJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    CameraNapiAsyncContext &asyncContext)
{
    char buffer[PATH_MAX];
    const int32_t refCount = 1;
    napi_value result;
    size_t length = 0;
    auto context = &asyncContext;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_string) {
            if (napi_get_value_string_utf8(env, argv[PARAM0], buffer, PATH_MAX, &length) == napi_ok) {
                MEDIA_INFO_LOG("surfaceId buffer : %{public}s", buffer);
                context->photoSurfaceId.append(buffer);
                MEDIA_INFO_LOG("context->photoSurfaceId after convert : %{public}s", context->photoSurfaceId.c_str());
            } else {
                MEDIA_ERR_LOG("Could not able to read surfaceId argument!");
            }
        } else if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    CameraNapiAsyncContext &asyncContext)
{
    char buffer[PATH_MAX];
    const int32_t refCount = 1;
    napi_value result;
    size_t length = 0;
    auto context = &asyncContext;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_string) {
            if (napi_get_value_string_utf8(env, argv[PARAM0], buffer, PATH_MAX, &length) == napi_ok) {
                MEDIA_INFO_LOG("surfaceId buffer --1  : %{public}s", buffer);
                std::istringstream iss(buffer);
                iss >> context->surfaceId;
                MEDIA_INFO_LOG("context->surfaceId after convert : %{public}" PRIu64, context->surfaceId);
            } else {
                MEDIA_ERR_LOG("Could not able to read surfaceId argument!");
            }
        } else if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

void CreatePreviewOutputAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraNapiAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;

    napi_get_undefined(env, &jsContext->error);

    MEDIA_ERR_LOG("context->surfaceId : %{public}" PRIu64, context->surfaceId);
    jsContext->data = PreviewOutputNapi::CreatePreviewOutput(env, context->surfaceId);

    if (jsContext->data == nullptr) {
        napi_get_undefined(env, &jsContext->data);
        MEDIA_ERR_LOG("Failed to Create preview output napi instance");
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraNapi::CreatePreviewOutputInstance(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("CreatePreviewOutputInstance called");
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraNapiAsyncContext>();
    result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
    CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "CreatePreviewOutput");
    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void* data) {},
        CreatePreviewOutputAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

void CreatePhotoOutputAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraNapiAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    MEDIA_ERR_LOG("context->photoSurfaceId : %{public}s", context->photoSurfaceId.c_str());
    jsContext->data = PhotoOutputNapi::CreatePhotoOutput(env, context->photoSurfaceId);

    if (jsContext->data == nullptr) {
        napi_get_undefined(env, &jsContext->data);
        MEDIA_ERR_LOG("Failed to Create photo output napi instance");
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraNapi::CreatePhotoOutputInstance(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraNapiAsyncContext>();
    result = ConvertPhotoOutputJSArgsToNative(env, argc, argv, *asyncContext);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
    CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "CreatePhotoOutput");
    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void* data) {},
        CreatePhotoOutputAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

void CreateVideoOutputAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraNapiAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    MEDIA_ERR_LOG("context->surfaceId : %{public}" PRIu64, context->surfaceId);
    jsContext->data = VideoOutputNapi::CreateVideoOutput(env, context->surfaceId);
    if (jsContext->data == nullptr) {
        napi_get_undefined(env, &jsContext->data);
        MEDIA_ERR_LOG("Failed to create video vutput instance");
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraNapi::CreateVideoOutputInstance(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraNapiAsyncContext>();
    result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
    CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "CreateVideoOutput");
    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void* data) {},
        CreateVideoOutputAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

napi_value CameraNapi::CreateFlashModeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecFlashMode.size(); i++) {
            propName = vecFlashMode[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop for FlashMode!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &flashModeRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateFlashModeObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateExposureModeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecExposureMode.size(); i++) {
            propName = vecExposureMode[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop for ExposureMode!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &exposureModeRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateExposureModeObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateExposureStateEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (auto itr = mapExposureState.begin(); itr != mapExposureState.end(); ++itr) {
            propName = itr->first;
            status = AddNamedProperty(env, result, propName, itr->second);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add FocusState prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &exposureStateRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateExposureStateEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateFocusModeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecFocusMode.size(); i++) {
            propName = vecFocusMode[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop for FocusMode!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &focusModeRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateFocusModeObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateFocusStateEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (auto itr = mapFocusState.begin(); itr != mapFocusState.end(); ++itr) {
            propName = itr->first;
            status = AddNamedProperty(env, result, propName, itr->second);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add FocusState prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &focusStateRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateFocusStateEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateCameraPositionEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecCameraPositionMode.size(); i++) {
            propName = vecCameraPositionMode[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop for CameraPosition!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &cameraPositionRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateCameraPositionEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateCameraTypeEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecCameraTypeMode.size(); i++) {
            propName = vecCameraTypeMode[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop for CameraType!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &cameraTypeRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateCameraTypeEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateConnectionTypeEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecConnectionTypeMode.size(); i++) {
            propName =  vecConnectionTypeMode[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop for CameraPosition!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &connectionTypeRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateConnectionTypeEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateCameraFormatObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecCameraFormat.size(); i++) {
            propName = vecCameraFormat[i];
            int32_t value = (propName.compare("CAMERA_FORMAT_JPEG") == 0) ? CAM_FORMAT_JPEG : CAM_FORMAT_YCRCb_420_SP;
            status = AddNamedProperty(env, result, propName, value);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &cameraFormatRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateCameraFormatObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateCameraStatusObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = 0; i < vecCameraStatus.size(); i++) {
            propName = vecCameraStatus[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &cameraStatusRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateCameraStatusObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateImageRotationEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (auto itr = mapImageRotation.begin(); itr != mapImageRotation.end(); ++itr) {
            propName = itr->first;
            status = AddNamedProperty(env, result, propName, itr->second);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add ImageRotation prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &imageRotationRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateImageRotationEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateQualityLevelEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (auto itr = mapQualityLevel.begin(); itr != mapQualityLevel.end(); ++itr) {
            propName = itr->first;
            status = AddNamedProperty(env, result, propName, itr->second);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add QualityLevel prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &qualityLevelRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateQualityLevelEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateErrorUnknownEnum(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName = "ERROR_UNKNOWN";

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        status = AddNamedProperty(env, result, propName, -1);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to add ERROR_UNKNOWN prop!");
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &errorUnknownRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_ERR_LOG("CreateErrorUnknownEnum is Failed!");
    napi_get_undefined(env, &result);

    return result;
}
} // namespace CameraStandard
} // namespace OHOS
