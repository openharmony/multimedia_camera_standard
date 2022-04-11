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

#include "input/camera_input_napi.h"
#include <uv.h>

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CameraNapi"};
}

napi_ref CameraInputNapi::sConstructor_ = nullptr;
std::string CameraInputNapi::sCameraId_ = "invalid";
sptr<CameraInput> CameraInputNapi::sCameraInput_ = nullptr;

void ExposureCallbackListener::OnExposureStateCallbackAsync(ExposureState state) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("ExposureCallbackListener:OnExposureStateCallbackAsync() failed to get event loop");
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("ExposureCallbackListener:OnExposureStateCallbackAsync() failed to allocate work");
        return;
    }
    ExposureCallbackInfo *callbackInfo = new(std::nothrow) ExposureCallbackInfo(state, this);
    if (!callbackInfo) {
        MEDIA_ERR_LOG("ExposureCallbackListener:OnExposureStateCallbackAsync() failed to allocate callback info");
        delete work;
        return;
    }
    work->data = callbackInfo;
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        ExposureCallbackInfo *callbackInfo = reinterpret_cast<ExposureCallbackInfo *>(work->data);
        if (callbackInfo) {
            callbackInfo->listener_->OnExposureStateCallback(callbackInfo->state_);
            delete callbackInfo;
        }
        delete work;
    });
    if (ret) {
        MEDIA_ERR_LOG("ExposureCallbackListener:OnExposureStateCallbackAsync() failed to execute work");
        delete callbackInfo;
        delete work;
    }
}

void ExposureCallbackListener::OnExposureStateCallback(ExposureState state) const
{
    int32_t jsExposureState;
    napi_value result[ARGS_TWO];
    napi_value callback = nullptr;
    napi_value retVal;

    CameraNapiUtils::MapExposureStateEnum(state, jsExposureState);

    napi_get_undefined(env_, &result[PARAM0]);
    napi_create_int32(env_, jsExposureState, &result[PARAM1]);

    napi_get_reference_value(env_, callbackRef_, &callback);
    napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
}

void ExposureCallbackListener::OnExposureState(const ExposureState state)
{
    MEDIA_INFO_LOG("ExposureCallbackListener:OnExposureState() is called!, state: %{public}d", state);
    OnExposureStateCallbackAsync(state);
}

void FocusCallbackListener::OnFocusStateCallbackAsync(FocusState state) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("FocusCallbackListener:OnFocusStateCallbackAsync() failed to get event loop");
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("FocusCallbackListener:OnFocusStateCallbackAsync() failed to allocate work");
        return;
    }
    FocusCallbackInfo *callbackInfo = new(std::nothrow) FocusCallbackInfo(state, this);
    if (!callbackInfo) {
        MEDIA_ERR_LOG("FocusCallbackListener:OnFocusStateCallbackAsync() failed to allocate callback info");
        delete work;
        return;
    }
    work->data = callbackInfo;
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        FocusCallbackInfo *callbackInfo = reinterpret_cast<FocusCallbackInfo *>(work->data);
        if (callbackInfo) {
            callbackInfo->listener_->OnFocusStateCallback(callbackInfo->state_);
            delete callbackInfo;
        }
        delete work;
    });
    if (ret) {
        MEDIA_ERR_LOG("FocusCallbackListener:OnFocusStateCallbackAsync() failed to execute work");
        delete callbackInfo;
        delete work;
    }
}

void FocusCallbackListener::OnFocusStateCallback(FocusState state) const
{
    int32_t jsFocusState;
    napi_value result[ARGS_TWO];
    napi_value callback = nullptr;
    napi_value retVal;

    CameraNapiUtils::MapFocusStateEnum(state, jsFocusState);

    napi_get_undefined(env_, &result[PARAM0]);
    napi_create_int32(env_, jsFocusState, &result[PARAM1]);

    napi_get_reference_value(env_, callbackRef_, &callback);
    napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
}

void FocusCallbackListener::OnFocusState(FocusState state)
{
    MEDIA_INFO_LOG("FocusCallbackListener:OnFocusState() is called!, state: %{public}d", state);
    OnFocusStateCallbackAsync(state);
}

void ErrorCallbackListener::OnErrorCallbackAsync(const int32_t errorType, const int32_t errorMsg) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("ErrorCallbackListener:OnErrorCallbackAsync() failed to get event loop");
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("ErrorCallbackListener:OnErrorCallbackAsync() failed to allocate work");
        return;
    }
    ErrorCallbackInfo *callbackInfo = new(std::nothrow) ErrorCallbackInfo(errorType, errorMsg, this);
    if (!callbackInfo) {
        MEDIA_ERR_LOG("ErrorCallbackListener:OnErrorCallbackAsync() failed to allocate callback info");
        delete work;
        return;
    }
    work->data = callbackInfo;
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        ErrorCallbackInfo *callbackInfo = reinterpret_cast<ErrorCallbackInfo *>(work->data);
        if (callbackInfo) {
            callbackInfo->listener_->OnErrorCallback(callbackInfo->errorType_, callbackInfo->errorMsg_);
            delete callbackInfo;
        }
        delete work;
    });
    if (ret) {
        MEDIA_ERR_LOG("ErrorCallbackListener:OnErrorCallbackAsync() failed to execute work");
        delete callbackInfo;
        delete work;
    }
}

void ErrorCallbackListener::OnErrorCallback(const int32_t errorType, const int32_t errorMsg) const
{
    int32_t jsErrorCodeUnknown = -1;
    napi_value result[ARGS_TWO];
    napi_value callback = nullptr;
    napi_value retVal;
    napi_value propValue;
    napi_create_object(env_, &result[PARAM1]);

    napi_get_undefined(env_, &result[PARAM0]);
    napi_create_int32(env_, jsErrorCodeUnknown, &propValue);

    napi_set_named_property(env_, result[PARAM1], "code", propValue);
    napi_get_reference_value(env_, callbackRef_, &callback);
    napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
}

void ErrorCallbackListener::OnError(const int32_t errorType, const int32_t errorMsg) const
{
    MEDIA_INFO_LOG("ErrorCallbackListener:OnError() is called!, errorType: %{public}d", errorType);
    OnErrorCallbackAsync(errorType, errorMsg);
}

CameraInputNapi::CameraInputNapi() : env_(nullptr), wrapper_(nullptr)
{
}

CameraInputNapi::~CameraInputNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void CameraInputNapi::CameraInputNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    CameraInputNapi *cameraObj = reinterpret_cast<CameraInputNapi*>(nativeObject);
    if (cameraObj != nullptr) {
        cameraObj->~CameraInputNapi();
    }
}

napi_value CameraInputNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor camera_input_props[] = {
        DECLARE_NAPI_FUNCTION("getCameraId", GetCameraId),
        DECLARE_NAPI_FUNCTION("hasFlash", HasFlash),
        DECLARE_NAPI_FUNCTION("isFlashModeSupported", IsFlashModeSupported),
        DECLARE_NAPI_FUNCTION("getFlashMode", GetFlashMode),
        DECLARE_NAPI_FUNCTION("setFlashMode", SetFlashMode),

        DECLARE_NAPI_FUNCTION("isExposureModeSupported", IsExposureModeSupported),
        DECLARE_NAPI_FUNCTION("getExposureMode", GetExposureMode),
        DECLARE_NAPI_FUNCTION("setExposureMode", SetExposureMode),

        DECLARE_NAPI_FUNCTION("isFocusModeSupported", IsFocusModeSupported),
        DECLARE_NAPI_FUNCTION("getFocusMode", GetFocusMode),
        DECLARE_NAPI_FUNCTION("setFocusMode", SetFocusMode),

        DECLARE_NAPI_FUNCTION("getSupportedSizes", GetSupportedSizes),

        DECLARE_NAPI_FUNCTION("getSupportedPhotoFormats", GetSupportedPhotoFormats),
        DECLARE_NAPI_FUNCTION("getSupportedVideoFormats", GetSupportedVideoFormats),
        DECLARE_NAPI_FUNCTION("getSupportedPreviewFormats", GetSupportedPreviewFormats),

        DECLARE_NAPI_FUNCTION("getZoomRatioRange", GetZoomRatioRange),
        DECLARE_NAPI_FUNCTION("getZoomRatio", GetZoomRatio),
        DECLARE_NAPI_FUNCTION("setZoomRatio", SetZoomRatio),

        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On)
    };

    status = napi_define_class(env, CAMERA_INPUT_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               CameraInputNapiConstructor, nullptr,
                               sizeof(camera_input_props) / sizeof(camera_input_props[PARAM0]),
                               camera_input_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_INPUT_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

// Constructor callback
napi_value CameraInputNapi::CameraInputNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraInputNapi> obj = std::make_unique<CameraInputNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->cameraInput_ = sCameraInput_;
            obj->cameraId_ = sCameraId_;
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               CameraInputNapi::CameraInputNapiDestructor, nullptr, &(obj->wrapper_));
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

napi_value CameraInputNapi::CreateCameraInput(napi_env env, std::string cameraId,
    sptr<CameraInput> cameraInput)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sCameraId_ = cameraId;
        sCameraInput_ = cameraInput;
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create Camera input instance");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

sptr<CameraInput> CameraInputNapi::GetCameraInput()
{
    return cameraInput_;
}

void GetCameraIdAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    if (context->status) {
        status = napi_create_string_utf8(env, context->cameraId.c_str(), NAPI_AUTO_LENGTH, &jsContext->data);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("GetCameraIdAsyncCallbackComplete:napi_create_string_utf8() failed");
            CameraNapiUtils::CreateNapiErrorObject(env,
                "GetCameraIdAsyncCallbackComplete:napi_create_string_utf8() failed", jsContext);
        }
    } else {
        CameraNapiUtils::CreateNapiErrorObject(env, context->errorMsg.c_str(), jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}


void FetchOptionsParam(napi_env env, napi_value arg, const CameraInputAsyncContext &context, bool &err)
{
    CameraInputAsyncContext *asyncContext = const_cast<CameraInputAsyncContext *>(&context);
    if (asyncContext == nullptr) {
        MEDIA_INFO_LOG("FetchOptionsParam:asyncContext is null");
        return;
    }

    if (asyncContext->enumType.compare("ZoomRatio") == 0) {
        double zoom;
        napi_get_value_double(env, arg, &zoom);
        MEDIA_INFO_LOG("Camera ZoomRatio : %{public}f", zoom);
        asyncContext->zoomRatio = zoom;
        return;
    }

    int32_t value;
    napi_get_value_int32(env, arg, &value);

    if (asyncContext->enumType.compare("CameraFormat") == 0) {
        if (CameraNapiUtils::MapCameraFormatEnumFromJs(value, asyncContext->cameraFormat) == -1) {
            err = true;
            return;
        }
    } else if (asyncContext->enumType.compare("FlashMode") == 0) {
        MEDIA_INFO_LOG("Camera flashMode : %{public}d", value);
        asyncContext->flashMode = value;
    } else if (asyncContext->enumType.compare("ExposureMode") == 0) {
        asyncContext->exposureMode = value;
    } else if (asyncContext->enumType.compare("FocusMode") == 0) {
        int32_t retVal = CameraNapiUtils::MapFocusModeEnumFromJs(value, asyncContext->focusMode);
        if (retVal == -1) {
            err = true;
            return;
        } else if (retVal == 1) {
            asyncContext->focusModeLocked = true;
        }
    } else {
        err = true;
    }
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    CameraInputAsyncContext &asyncContext)
{
    string str = "";
    vector<string> strArr;
    string order = "";
    bool err = false;
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_number) {
            FetchOptionsParam(env, argv[PARAM0], asyncContext, err);
            if (err) {
                MEDIA_ERR_LOG("fetch options retrieval failed");
                NAPI_ASSERT(env, false, "type mismatch");
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

void CommonCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    if (context == nullptr) {
        MEDIA_ERR_LOG("Async context is null");
        return;
    }
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();

    if (!context->status) {
        CameraNapiUtils::CreateNapiErrorObject(env, context->errorMsg.c_str(), jsContext);
    } else {
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
        if (context->bRetBool) {
            napi_get_boolean(env, context->isSupported, &jsContext->data);
        } else {
            napi_get_undefined(env, &jsContext->data);
        }
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetCameraId(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetCameraId");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->status = true;
                    context->cameraId = context->objectInfo->cameraId_;
                    if (context->cameraId.empty()) {
                        context->status = false;
                        context->errorMsg = "GetCameraId( ) Failed";
                    }
                }
            },
            GetCameraIdAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetCameraId");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

bool CameraInputNapi::IsFlashSupported(sptr<CameraInput> cameraInput, int flash)
{
    std::vector<camera_flash_mode_enum_t> list = cameraInput->GetSupportedFlashModes();
    camera_flash_mode_enum_t flashMode = static_cast<camera_flash_mode_enum_t>(flash);
    for (size_t i = 0; i < list.size(); ++i) {
        if (flashMode == list[i]) {
            return true;
        }
    }
    MEDIA_ERR_LOG("FlashMode : %{public}d, does not supported", flash);
    return false;
}

napi_value CameraInputNapi::HasFlash(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "HasFlash");
        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = true;
                    std::vector<camera_flash_mode_enum_t> list;
                    list = context->objectInfo->cameraInput_->GetSupportedFlashModes();
                    if (list.empty()) {
                        context->status = false;
                        context->errorMsg = "GetSupportedFlashModes( ) Failed";
                    } else {
                        context->status = true;
                        context->isSupported = !(list.empty());
                    }
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for HasFlash");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::IsFlashModeSupported(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "FlashMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "IsFlashModeSupported");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = true;
                    context->isSupported = IsFlashSupported(context->objectInfo->cameraInput_, context->flashMode);
                    context->status = true;
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for IsFlashModeSupported");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::SetFlashMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "FlashMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "SetFlashMode");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    sptr<CameraInput> cameraInput = context->objectInfo->cameraInput_;
                    if (IsFlashSupported(cameraInput, context->flashMode)) {
                        cameraInput->LockForControl();
                        cameraInput->SetFlashMode(static_cast<camera_flash_mode_enum_t>(context->flashMode));
                        cameraInput->SetExposureMode(OHOS_CAMERA_AE_MODE_ON_ALWAYS_FLASH);
                        cameraInput->UnlockForControl();
                        context->status = true;
                    } else {
                        MEDIA_ERR_LOG("Flash mode is not supported");
                        context->errorMsg = "Flash mode is not supported";
                    }
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for SetFlashMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetFlashModeAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    status = napi_create_int32(env, context->flashMode, &jsContext->data);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("GetFlashModeAsyncCallbackComplete:napi_create_int32() failed");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "GetFlashModeAsyncCallbackComplete:napi_create_int32() failed", jsContext);
    }
    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetFlashMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");
    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetFlashMode");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->flashMode = context->objectInfo->cameraInput_->GetFlashMode();
                    context->status = true;
                }
            },
            GetFlashModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetFlashMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::IsExposureModeSupported(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "ExposureMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "IsExposureModeSupported");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->bRetBool = true;
                context->status = true;
                context->isSupported = true;
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for IsExposureModeSupported");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetExposureModeAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    status = napi_create_int32(env, context->exposureMode, &jsContext->data);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("GetExposureModeAsyncCallbackComplete:napi_create_int32() failed");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "GetExposureModeAsyncCallbackComplete:napi_create_int32() failed", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetExposureMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetExposureMode");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->exposureMode = context->objectInfo->cameraInput_->GetExposureMode();
                    context->status = true;
                }
            },
            GetExposureModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetExposureMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::SetExposureMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "ExposureMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "SetExposureMode");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->objectInfo->cameraInput_->
                                SetExposureMode(static_cast<camera_ae_mode_t>(context->exposureMode));
                    context->status = true;
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for SetExposureMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::IsFocusModeSupported(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "FocusMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "IsFocusModeSupported");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = true;
                    context->status = true;
                    if (context->focusModeLocked) {
                        MEDIA_INFO_LOG("FOCUS_MODE_LOCKED is supported");
                        return;
                    }

                    vector<camera_af_mode_t> vecSupportedFocusModeList;
                    vecSupportedFocusModeList = context->objectInfo->cameraInput_->GetSupportedFocusModes();
                    if (find(vecSupportedFocusModeList.begin(), vecSupportedFocusModeList.end(),
                        context->focusMode) != vecSupportedFocusModeList.end()) {
                        context->isSupported = true;
                    } else {
                        context->isSupported = false;
                    }
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for IsFocusModeSupported");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetSupportedPhotoFormatsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    napi_value photoFormats = nullptr;
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    if (!context->vecSupportedPhotoFormatList.empty()) {
        size_t len = context->vecSupportedPhotoFormatList.size();
        if (napi_create_array(env, &photoFormats) == napi_ok) {
            size_t i;
            size_t j = 0;
            for (i = 0; i < len; i++) {
                int32_t iProp;
                CameraNapiUtils::MapCameraFormatEnum(context->vecSupportedPhotoFormatList[i], iProp);
                napi_value value;
                if (iProp != -1 && napi_create_int32(env, iProp, &value) == napi_ok) {
                    napi_set_element(env, photoFormats, j, value);
                    j++;
                }
            }
            jsContext->data = photoFormats;
        } else {
            MEDIA_ERR_LOG("GetSupportedPhotoFormatsAsyncCallbackComplete:napi_create_array() failed");
            CameraNapiUtils::CreateNapiErrorObject(env,
                "GetSupportedPhotoFormatsAsyncCallbackComplete:napi_create_array() failed", jsContext);
        }
    } else {
        MEDIA_ERR_LOG("No PhotoFormats found!");
        CameraNapiUtils::CreateNapiErrorObject(env, "No PhotoFormats found!", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetSupportedPhotoFormats(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetSupportedPhotoFormats");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->vecSupportedPhotoFormatList =
                        context->objectInfo->cameraInput_->GetSupportedPhotoFormats();
                    context->status = true;
                }
            },
            GetSupportedPhotoFormatsAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetSupportedPhotoFormats");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetSupportedVideoFormatsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    napi_value videoFormats = nullptr;
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    if (!context->vecSupportedVideoFormatList.empty()) {
        size_t len = context->vecSupportedVideoFormatList.size();
        if (napi_create_array(env, &videoFormats) == napi_ok) {
            size_t i;
            size_t j = 0;
            for (i = 0; i < len; i++) {
                int32_t iProp;
                CameraNapiUtils::MapCameraFormatEnum(context->vecSupportedVideoFormatList[i], iProp);
                napi_value value;
                if (iProp != -1 && napi_create_int32(env, iProp, &value) == napi_ok) {
                    napi_set_element(env, videoFormats, j, value);
                    j++;
                }
            }
            jsContext->data = videoFormats;
        } else {
            MEDIA_ERR_LOG("GetSupportedVideoFormatsAsyncCallbackComplete:napi_create_array() failed");
            CameraNapiUtils::CreateNapiErrorObject(env,
                "GetSupportedVideoFormatsAsyncCallbackComplete:napi_create_array() failed", jsContext);
        }
    } else {
        MEDIA_ERR_LOG("No VideoFormats found!");
        CameraNapiUtils::CreateNapiErrorObject(env, "No VideoFormats found!", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetSupportedVideoFormats(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetSupportedVideoFormats");
        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->vecSupportedVideoFormatList =
                        context->objectInfo->cameraInput_->GetSupportedVideoFormats();
                    context->status = true;
                }
            },
            GetSupportedVideoFormatsAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetSupportedVideoFormats");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetSupportedPreviewFormatsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    napi_value  previewFormats = nullptr;
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    if (!context->vecSupportedPreviewFormatList.empty()) {
        size_t len = context->vecSupportedPreviewFormatList.size();
        if (napi_create_array(env, &previewFormats) == napi_ok) {
            size_t i;
            size_t j = 0;
            for (i = 0; i < len; i++) {
                int32_t iProp;
                CameraNapiUtils::MapCameraFormatEnum(context->vecSupportedPreviewFormatList[i], iProp);
                napi_value value;
                if (iProp != -1 && napi_create_int32(env, iProp, &value) == napi_ok) {
                    napi_set_element(env, previewFormats, j, value);
                    j++;
                }
            }
            jsContext->data = previewFormats;
        } else {
            MEDIA_ERR_LOG("GetSupportedPreviewFormatsAsyncCallbackComplete:napi_create_array() failed");
            CameraNapiUtils::CreateNapiErrorObject(env,
                "GetSupportedPreviewFormatsAsyncCallbackComplete:napi_create_array() failed", jsContext);
        }
    } else {
        MEDIA_ERR_LOG("No previewFormats found!");
        CameraNapiUtils::CreateNapiErrorObject(env, "No previewFormats found!", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetSupportedPreviewFormats(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetSupportedPreviewFormats");
        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->vecSupportedPreviewFormatList =
                        context->objectInfo->cameraInput_->GetSupportedPreviewFormats();
                    context->status = true;
                }
            },
            GetSupportedPreviewFormatsAsyncCallbackComplete,
            static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetSupportedPreviewFormats");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetFocusModeAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    int32_t jsFocusMode;
    CameraNapiUtils::MapFocusModeEnum(context->focusMode, jsFocusMode);
    status = napi_create_int32(env, jsFocusMode, &jsContext->data);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("GetFocusModeAsyncCallbackComplete:napi_create_int32() failed");
            CameraNapiUtils::CreateNapiErrorObject(env,
                "GetFocusModeAsyncCallbackComplete:napi_create_int32() failed", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetFocusMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetFocusMode");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->focusMode = context->objectInfo->cameraInput_->GetFocusMode();
                    context->status = true;
                }
            },
            GetFocusModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetFocusMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::SetFocusMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "FocusMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "SetFocusMode");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    sptr<CameraInput> cameraInput = context->objectInfo->cameraInput_;
                    context->status = true;
                    if (context->focusModeLocked) {
                        MEDIA_INFO_LOG("Focus mode set is FOCUS_MODE_LOCKED");
                        return;
                    }

                    vector<camera_af_mode_t> vecSupportedFocusModeList;
                    vecSupportedFocusModeList = context->objectInfo->cameraInput_->GetSupportedFocusModes();
                    if (find(vecSupportedFocusModeList.begin(), vecSupportedFocusModeList.end(),
                        context->focusMode) != vecSupportedFocusModeList.end()) {
                        cameraInput->LockForControl();
                        context->objectInfo->cameraInput_->SetFocusMode(context->focusMode);
                        cameraInput->UnlockForControl();
                    } else {
                        MEDIA_ERR_LOG("Focus mode is not supported");
                        context->status = false;
                        context->errorMsg = "Focus mode is not supported";
                    }
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for SetFocusMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetSupportedSizesAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    napi_value cameraSizeArray = nullptr;
    napi_value cameraSize = nullptr;

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    if (!context->vecSupportedSizeList.empty()) {
        size_t len = context->vecSupportedSizeList.size();
        if (napi_create_array(env, &cameraSizeArray) == napi_ok) {
            size_t i;
            for (i = 0; i < len; i++) {
                cameraSize = CameraSizeNapi::CreateCameraSize(env, context->vecSupportedSizeList[i]);
                if (cameraSize == nullptr || napi_set_element(env, cameraSizeArray, i, cameraSize) != napi_ok) {
                    HiLog::Error(LABEL, "Failed to get CameraSize napi object");
                    CameraNapiUtils::CreateNapiErrorObject(env,
                        "Failed to get CameraSize napi object", jsContext);
                    break;
                }
            }
            if (i == len) {
                jsContext->data = cameraSizeArray;
            }
        } else {
            CameraNapiUtils::CreateNapiErrorObject(env, "GetSupportedSizes() failed", jsContext);
        }
    } else {
        HiLog::Error(LABEL, "No supported size found!");
        CameraNapiUtils::CreateNapiErrorObject(env, "No supported size found!", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetSupportedSizes(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "CameraFormat";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetSupportedSizes");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->vecSupportedSizeList =
                        context->objectInfo->cameraInput_->getSupportedSizes(context->cameraFormat);
                    context->status = true;
                }
            },
            GetSupportedSizesAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetSupportedSizes");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetZoomRatioRangeAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    napi_value zoomRatioRange = nullptr;

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    if (!context->vecZoomRatioList.empty() && napi_create_array(env, &zoomRatioRange) == napi_ok) {
        int32_t j = 0;
        for (size_t i = 0; i < context->vecZoomRatioList.size(); i++) {
            int32_t  zoomRatio = context->vecZoomRatioList[i];
            napi_value value;
            if (napi_create_double(env, zoomRatio, &value) == napi_ok) {
                napi_set_element(env, zoomRatioRange, j, value);
                j++;
            }
        }
        jsContext->data = zoomRatioRange;
    } else {
        MEDIA_ERR_LOG("vecSupportedZoomRatioList is empty or failed to create array!");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "vecSupportedZoomRatioList is empty or failed to create array!", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetZoomRatioRange(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetSupportedZoomRatioRange");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->vecZoomRatioList = context->objectInfo->cameraInput_->GetSupportedZoomRatioRange();
                    context->status = true;
                }
            },
            GetZoomRatioRangeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetZoomRatioRange");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetZoomRatioAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    status = napi_create_double(env, context->zoomRatio, &jsContext->data);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("GetZoomRatioAsyncCallbackComplete:napi_create_double() failed");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "GetZoomRatioAsyncCallbackComplete:napi_create_double() failed", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraInputNapi::GetZoomRatio(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetZoomRatio");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->zoomRatio = context->objectInfo->cameraInput_->GetZoomRatio();
                    context->status = true;
                }
            },
            GetZoomRatioAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for GetZoomRatio");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::SetZoomRatio(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "ZoomRatio";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "SetZoomRatio");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    sptr<CameraInput> cameraInput = context->objectInfo->cameraInput_;
                    cameraInput->LockForControl();
                    cameraInput->SetZoomRatio(context->zoomRatio);
                    cameraInput->UnlockForControl();
                    context->status = true;
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for SetZoomRatio");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraInputNapi::Release(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Release");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraInputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->objectInfo->cameraInput_->Release();
                    context->status = true;
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraInputNapi::Release");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void CameraInputNapi::RegisterCallback(napi_env env, const string &eventType, napi_ref callbackRef)
{
    if (eventType.empty()) {
        MEDIA_ERR_LOG("Failed to Register Callback callback name is empty!");
        return;
    }

    if (eventType.compare("exposureStateChange") == 0) {
        // Set callback for exposureStateChange
        shared_ptr<ExposureCallbackListener> exposureCallback = make_shared<ExposureCallbackListener>(env, callbackRef);
        cameraInput_->SetExposureCallback(exposureCallback);
        exposureCallback_ = exposureCallback;
    } else if (eventType.compare("focusStateChange") == 0) {
        // Set callback for focusStateChange
        shared_ptr<FocusCallbackListener> focusCallback = make_shared<FocusCallbackListener>(env, callbackRef);
        cameraInput_->SetFocusCallback(focusCallback);
        focusCallback_ = focusCallback;
    } else if (eventType.compare("error") == 0) {
        // Set callback for error
        shared_ptr<ErrorCallbackListener> errorCallback = make_shared<ErrorCallbackListener>(env, callbackRef);
        cameraInput_->SetErrorCallback(errorCallback);
        errorCallback_ = errorCallback;
    } else {
        MEDIA_ERR_LOG("Incorrect callback event type provided for camera input!");
    }
}

napi_value CameraInputNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    size_t argCount = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    size_t res = 0;
    char buffer[SIZE];
    std::string eventType;
    const int32_t refCount = 1;
    CameraInputNapi *obj = nullptr;
    napi_status status;

    napi_get_undefined(env, &undefinedResult);

    CAMERA_NAPI_GET_JS_ARGS(env, info, argCount, argv, thisVar);
    NAPI_ASSERT(env, argCount == ARGS_TWO, "requires 2 parameters");

    if (thisVar == nullptr || argv[PARAM0] == nullptr || argv[PARAM1] == nullptr) {
        MEDIA_ERR_LOG("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status == napi_ok && obj != nullptr) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string
            || napi_typeof(env, argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
            return undefinedResult;
        }

        napi_get_value_string_utf8(env, argv[PARAM0], buffer, SIZE, &res);
        eventType = std::string(buffer);

        napi_ref callbackRef;
        napi_create_reference(env, argv[PARAM1], refCount, &callbackRef);

        obj->RegisterCallback(env, eventType, callbackRef);
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS
