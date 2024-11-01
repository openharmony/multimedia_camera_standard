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

#include "session/camera_session_napi.h"
#include <uv.h>

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CameraNapi"};
}

thread_local napi_ref CameraSessionNapi::sConstructor_ = nullptr;
thread_local sptr<CaptureSession> CameraSessionNapi::sCameraSession_ = nullptr;
thread_local uint32_t CameraSessionNapi::cameraSessionTaskId = CAMERA_SESSION_TASKID;

void SessionCallbackListener::OnErrorCallbackAsync(int32_t errorCode) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("SessionCallbackListener:OnErrorCallbackAsync() failed to get event loop");
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("SessionCallbackListener:OnErrorCallbackAsync() failed to allocate work");
        return;
    }
    std::unique_ptr<SessionCallbackInfo> callbackInfo = std::make_unique<SessionCallbackInfo>(errorCode, this);
    work->data = callbackInfo.get();
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        SessionCallbackInfo *callbackInfo = reinterpret_cast<SessionCallbackInfo *>(work->data);
        if (callbackInfo) {
            callbackInfo->listener_->OnErrorCallback(callbackInfo->errorCode_);
            delete callbackInfo;
        }
        delete work;
    });
    if (ret) {
        MEDIA_ERR_LOG("SessionCallbackListener:OnErrorCallbackAsync() failed to execute work");
        delete work;
    } else {
        callbackInfo.release();
    }
}

void SessionCallbackListener::OnErrorCallback(int32_t errorCode) const
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

void SessionCallbackListener::OnError(int32_t errorCode)
{
    MEDIA_INFO_LOG("SessionCallbackListener:OnError() is called!, errorCode: %{public}d", errorCode);
    OnErrorCallbackAsync(errorCode);
}

CameraSessionNapi::CameraSessionNapi() : env_(nullptr), wrapper_(nullptr)
{
}

CameraSessionNapi::~CameraSessionNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void CameraSessionNapi::CameraSessionNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    CameraSessionNapi *cameraObj = reinterpret_cast<CameraSessionNapi*>(nativeObject);
    if (cameraObj != nullptr) {
        cameraObj->~CameraSessionNapi();
    }
}

napi_value CameraSessionNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor camera_session_props[] = {
        DECLARE_NAPI_FUNCTION("beginConfig", BeginConfig),
        DECLARE_NAPI_FUNCTION("commitConfig", CommitConfig),

        DECLARE_NAPI_FUNCTION("addInput", AddInput),
        DECLARE_NAPI_FUNCTION("removeInput", RemoveInput),

        DECLARE_NAPI_FUNCTION("addOutput", AddOutput),
        DECLARE_NAPI_FUNCTION("removeOutput", RemoveOutput),

        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("isVideoStabilizationModeSupported", IsVideoStabilizationModeSupported),
        DECLARE_NAPI_FUNCTION("getActiveVideoStabilizationMode", GetActiveVideoStabilizationMode),
        DECLARE_NAPI_FUNCTION("setVideoStabilizationMode", SetVideoStabilizationMode),

        DECLARE_NAPI_FUNCTION("on", On)
    };

    status = napi_define_class(env, CAMERA_SESSION_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               CameraSessionNapiConstructor, nullptr,
                               sizeof(camera_session_props) / sizeof(camera_session_props[PARAM0]),
                               camera_session_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_SESSION_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

// Constructor callback
napi_value CameraSessionNapi::CameraSessionNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraSessionNapi> obj = std::make_unique<CameraSessionNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            if (sCameraSession_ == nullptr) {
                MEDIA_ERR_LOG("sCameraSession_ is null");
                return result;
            }
            obj->cameraSession_ = sCameraSession_;
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               CameraSessionNapi::CameraSessionNapiDestructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                MEDIA_ERR_LOG("CameraSessionNapi Failure wrapping js to native napi");
            }
        }
    }

    return result;
}

void FetchOptionsParam(napi_env env, napi_value arg, const CameraSessionAsyncContext &context, bool &err)
{
    CameraSessionAsyncContext *asyncContext = const_cast<CameraSessionAsyncContext *>(&context);
    if (asyncContext == nullptr) {
        MEDIA_INFO_LOG("FetchOptionsParam:asyncContext is null");
        return;
    }

    int32_t value;
    napi_get_value_int32(env, arg, &value);

    if (asyncContext->enumType.compare("VideoStabilizationMode") == 0) {
        if ((value < OFF) || (value > AUTO)) {
            err = true;
            return;
        }
        asyncContext->videoStabilizationMode = static_cast<VideoStabilizationMode>(value);
    } else {
        err = true;
    }
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    CameraSessionAsyncContext &asyncContext)
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

        if (i == PARAM0 && (valueType == napi_number || valueType == napi_object)) {
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

napi_value CameraSessionNapi::CreateCameraSession(napi_env env)
{
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sCameraSession_ = CameraManager::GetInstance()->CreateCaptureSession();
        if (sCameraSession_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create Camera session instance");
            napi_get_undefined(env, &result);
            return result;
        } else {
            MEDIA_INFO_LOG("Camera session instance create success");
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sCameraSession_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create Camera session napi instance");
        }
    }
    MEDIA_ERR_LOG("Failed to create Camera session napi instance last");
    napi_get_undefined(env, &result);
    return result;
}

static void CommonCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraSessionAsyncContext*>(data);
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

    if (!context->funcName.empty() && context->taskId > 0) {
        // Finish async trace
        CAMERA_FINISH_ASYNC_TRACE(context->funcName, context->taskId);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}


napi_value CameraSessionNapi::BeginConfig(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("BeginConfig called");
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= 1, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "BeginConfig");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::BeginConfig";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->BeginConfig();
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "BeginConfig( ) failure";
                    }
                    MEDIA_INFO_LOG("BeginConfig return : %{public}d", ret);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for BeginConfig");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::CommitConfig(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("CommitConfig called");
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= 1, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "CommitConfig");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::CommitConfig";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->CommitConfig();
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "CommitConfig( ) failure";
                    }
                    MEDIA_INFO_LOG("CommitConfig return : %{public}d", ret);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for CommitConfig");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value GetJSArgsForCameraInput(napi_env env, size_t argc, const napi_value argv[],
    CameraSessionAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CameraInputNapi *cameraInputNapiObj = nullptr;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_object) {
            napi_unwrap(env, argv[i], reinterpret_cast<void**>(&cameraInputNapiObj));
            if (cameraInputNapiObj != nullptr) {
                context->cameraInput = cameraInputNapiObj->GetCameraInput();
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
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

napi_value CameraSessionNapi::AddInput(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("AddInput called");
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForCameraInput(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "AddInput");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::AddInput";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->AddInput(context->cameraInput);
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "AddInput( ) failure";
                    }
                    MEDIA_INFO_LOG("AddInput status : %{public}d", context->status);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for AddInput");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::RemoveInput(napi_env env, napi_callback_info info)
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
    auto asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForCameraInput(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "RemoveInput");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::RemoveInput";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->RemoveInput(context->cameraInput);
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "RemoveInput( ) failure";
                    }
                    MEDIA_INFO_LOG("RemoveInput return : %{public}d", ret);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for RemoveInput");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value GetJSArgsForCameraOutput(napi_env env, size_t argc, const napi_value argv[],
    CameraSessionAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    PreviewOutputNapi *previewOutputNapiObj = nullptr;
    PhotoOutputNapi *photoOutputNapiObj = nullptr;
    VideoOutputNapi *videoOutputNapiObj = nullptr;
    MetadataOutputNapi *metadataOutputNapiObj = nullptr;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_object) {
            if (PreviewOutputNapi::IsPreviewOutput(env, argv[i])) {
                MEDIA_INFO_LOG("preview output adding..");
                napi_unwrap(env, argv[i], reinterpret_cast<void**>(&previewOutputNapiObj));
                context->cameraOutput = previewOutputNapiObj->GetPreviewOutput();
            } else if (PhotoOutputNapi::IsPhotoOutput(env, argv[i])) {
                MEDIA_INFO_LOG("photo output adding..");
                napi_unwrap(env, argv[i], reinterpret_cast<void**>(&photoOutputNapiObj));
                context->cameraOutput = photoOutputNapiObj->GetPhotoOutput();
            } else if (VideoOutputNapi::IsVideoOutput(env, argv[i])) {
                MEDIA_INFO_LOG("video output adding..");
                napi_unwrap(env, argv[i], reinterpret_cast<void**>(&videoOutputNapiObj));
                context->cameraOutput = videoOutputNapiObj->GetVideoOutput();
            } else if (MetadataOutputNapi::IsMetadataOutput(env, argv[i])) {
                MEDIA_INFO_LOG("metadata output adding..");
                napi_unwrap(env, argv[i], reinterpret_cast<void**>(&metadataOutputNapiObj));
                context->cameraOutput = metadataOutputNapiObj->GetMetadataOutput();
            } else {
                MEDIA_INFO_LOG("invalid output ..");
                NAPI_ASSERT(env, false, "type mismatch");
            }
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

napi_value CameraSessionNapi::AddOutput(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("AddOutput called");
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForCameraOutput(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "AddOutput");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::AddOutput";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->AddOutput(context->cameraOutput);
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "AddOutput( ) failure";
                    }
                    MEDIA_INFO_LOG("AddOutput return : %{public}d", ret);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for AddOutput");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::RemoveOutput(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("RemoveOutput called");
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    auto asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForCameraOutput(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "RemoveOutput");
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::RemoveOutput";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->RemoveOutput(context->cameraOutput);
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "RemoveOutput( ) failure";
                    }
                    MEDIA_INFO_LOG("RemoveOutput return : %{public}d", ret);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for RemoveOutput");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::Start(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("start called");
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= 1, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Start");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::Start";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->Start();
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "Start( ) failure";
                    }
                    MEDIA_INFO_LOG("Start return : %{public}d", ret);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraSessionNapi::Start");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::Stop(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("Stop called");
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= 1, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Stop");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::Stop";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = context->objectInfo->cameraSession_->Stop();
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "Stop( ) failure";
                    }
                    MEDIA_INFO_LOG("Stop return : %{public}d", ret);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraSessionNapi::Stop");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::Release(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= 1, "requires 1 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Release");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                // Start async trace
                context->funcName = "CameraSessionNapi::Release";
                context->taskId = CameraNapiUtils::IncreamentAndGet(cameraSessionTaskId);
                CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    context->objectInfo->cameraSession_->Release();
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraSessionNapi::Release");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::IsVideoStabilizationModeSupported(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "VideoStabilizationMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "IsVideoStabilizationModeSupported");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = true;
                    context->status = true;
                    context->isSupported = context->objectInfo->cameraSession_->
                        IsVideoStabilizationModeSupported(context->videoStabilizationMode);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG(
                "Failed to create napi_create_async_work for CameraSessionNapi::IsVideoStabilizationModeSupported");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void GetActiveVideoStabilizationModeAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<CameraSessionAsyncContext*>(data);

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    status = napi_create_int32(env, context->videoStabilizationMode, &jsContext->data);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("GetActiveVideoStabilizationMode:napi_create_int32() failed");
            CameraNapiUtils::CreateNapiErrorObject(env,
                "GetActiveVideoStabilizationMode:napi_create_int32() failed", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value CameraSessionNapi::GetActiveVideoStabilizationMode(napi_env env, napi_callback_info info)
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
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetActiveVideoStabilizationMode");
        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->videoStabilizationMode = context->objectInfo->cameraSession_->
                        GetActiveVideoStabilizationMode();
                    context->status = true;
                }
            },
            GetActiveVideoStabilizationModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()),
            &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG(
                "Failed to create napi_create_async_work for CameraSessionNapi::GetActiveVideoStabilizationMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::SetVideoStabilizationMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameter maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<CameraSessionAsyncContext> asyncContext = std::make_unique<CameraSessionAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        asyncContext->enumType = "VideoStabilizationMode";
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "SetVideoStabilizationMode");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<CameraSessionAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    if (context->objectInfo->cameraSession_->
                        IsVideoStabilizationModeSupported(context->videoStabilizationMode)) {
                        context->bRetBool = false;
                        context->status = true;
                        ((sptr<CaptureSession> &)(context->objectInfo->cameraSession_))->
                            SetVideoStabilizationMode(context->videoStabilizationMode);
                    } else {
                        MEDIA_ERR_LOG("Video Stabilization Mode is not supported");
                        context->errorMsg = "Video Stabilization Mode is not supported";
                    }
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraSessionNapi::SetVideoStabilizationMode");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraSessionNapi::On(napi_env env, napi_callback_info info)
{
    CAMERA_SYNC_TRACE;
    napi_value undefinedResult = nullptr;
    size_t argCount = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    size_t res = 0;
    char buffer[SIZE];
    std::string eventType;
    const int32_t refCount = 1;
    CameraSessionNapi *obj = nullptr;
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

        if (!eventType.empty() && eventType.compare("error") == 0) {
            std::shared_ptr<SessionCallbackListener> callback =
                std::make_shared<SessionCallbackListener>(SessionCallbackListener(env, callbackRef));
            obj->cameraSession_->SetCallback(callback);
        } else {
            MEDIA_ERR_LOG("Failed to Register Callback: event type is empty!");
            if (callbackRef != nullptr) {
                napi_delete_reference(env, callbackRef);
            }
        }
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS
