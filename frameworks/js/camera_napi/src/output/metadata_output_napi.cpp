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

#include <uv.h>
#include "hilog/log.h"
#include "output/metadata_object_napi.h"
#include "output/metadata_output_napi.h"

namespace OHOS {
namespace CameraStandard {
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MetadataOutputNapi"};
}

thread_local napi_ref MetadataOutputNapi::sConstructor_ = nullptr;
thread_local sptr<MetadataOutput> MetadataOutputNapi::sMetadataOutput_ = nullptr;

MetadataOutputCallback::MetadataOutputCallback(napi_env env) : env_(env) {}

void MetadataOutputCallback::OnMetadataObjectsAvailable(const std::vector<sptr<MetadataObject>> metadataObjList) const
{
    MEDIA_INFO_LOG("MetadataOutputCallback::OnMetadataObjectsAvailable");
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("MetadataOutputCallback:OnMetadataObjectsAvailable() failed to get event loop");
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("MetadataOutputCallback:OnMetadataObjectsAvailable() failed to allocate work");
        return;
    }
    std::unique_ptr<MetadataOutputCallbackInfo> callbackInfo =
        std::make_unique<MetadataOutputCallbackInfo>(metadataObjList, this);
    work->data = reinterpret_cast<void *>(callbackInfo.get());
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        MetadataOutputCallbackInfo *callbackInfo = reinterpret_cast<MetadataOutputCallbackInfo *>(work->data);
        if (callbackInfo) {
            callbackInfo->listener_->OnMetadataObjectsAvailableCallback(callbackInfo->info_);
            delete callbackInfo;
        }
        delete work;
    });
    if (ret) {
        MEDIA_ERR_LOG("MetadataOutputCallback:OnMetadataObjectsAvailable() failed to execute work");
        delete work;
    }  else {
        callbackInfo.release();
    }
}

static napi_value CreateMetadataObjJSArray(napi_env env,
    const std::vector<sptr<MetadataObject>> metadataObjList)
{
    napi_value metadataObjArray = nullptr;
    napi_value metadataObj = nullptr;
    napi_status status;

    if (metadataObjList.empty()) {
        MEDIA_ERR_LOG("CreateMetadataObjJSArray: metadataObjList is empty");
        return metadataObjArray;
    }

    status = napi_create_array(env, &metadataObjArray);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("CreateMetadataObjJSArray: napi_create_array failed");
        return metadataObjArray;
    }

    for (size_t i = 0; i < metadataObjList.size(); i++) {
        size_t j = 0;
        metadataObj = MetadataObjectNapi::CreateMetaFaceObj(env, metadataObjList[i]);
        if ((metadataObj == nullptr) || napi_set_element(env, metadataObjArray, j++, metadataObj) != napi_ok) {
            MEDIA_ERR_LOG("CreateMetadataObjJSArray: Failed to create metadata face object napi wrapper object");
            return nullptr;
        }
    }
    return metadataObjArray;
}

void MetadataOutputCallback::OnMetadataObjectsAvailableCallback(
    const std::vector<sptr<MetadataObject>> metadataObjList) const
{
    napi_value result[ARGS_TWO];
    napi_value callback = nullptr;
    napi_value retVal;

    napi_get_undefined(env_, &result[PARAM0]);

    CAMERA_NAPI_CHECK_AND_RETURN_LOG((metadataObjList.size() != 0), "callback metadataObjList is null");

    result[PARAM1] = CreateMetadataObjJSArray(env_, metadataObjList);
    if (result[PARAM1] == nullptr) {
        MEDIA_ERR_LOG("MetadataOutputCallback::OnMetadataObjectsAvailableCallback"
        " invoke CreateMetadataObjJSArray failed");
        return;
    }

    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(metadataObjectsAvailableCallbackRef_,
        "metadataObjectsAvailable callback is not registered by JS");
    napi_get_reference_value(env_, metadataObjectsAvailableCallbackRef_, &callback);
    napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
}

void MetadataOutputCallback::SetCallbackRef(const std::string &eventType, const napi_ref &callbackRef)
{
    if (eventType.compare("metadataObjectsAvailable") == 0) {
        metadataObjectsAvailableCallbackRef_ = callbackRef;
    } else {
        MEDIA_ERR_LOG("Incorrect metadata callback event type received from JS");
    }
}

MetadataOutputNapi::MetadataOutputNapi() : env_(nullptr), wrapper_(nullptr)
{
}

MetadataOutputNapi::~MetadataOutputNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void MetadataOutputNapi::MetadataOutputNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    MetadataOutputNapi *metadataOutput = reinterpret_cast<MetadataOutputNapi*>(nativeObject);
    if (metadataOutput != nullptr) {
        metadataOutput->~MetadataOutputNapi();
    }
}

napi_value MetadataOutputNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor metadata_output_props[] = {
        DECLARE_NAPI_FUNCTION("getSupportedMetadataObjectTypes", GetSupportedMetadataObjectTypes),
        DECLARE_NAPI_FUNCTION("setCapturingMetadataObjectTypes", SetCapturingMetadataObjectTypes),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("on", On)
    };

    status = napi_define_class(env, CAMERA_METADATA_OUTPUT_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               MetadataOutputNapiConstructor, nullptr,
                               sizeof(metadata_output_props) / sizeof(metadata_output_props[PARAM0]),
                               metadata_output_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_METADATA_OUTPUT_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value MetadataOutputNapi::MetadataOutputNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<MetadataOutputNapi> obj = std::make_unique<MetadataOutputNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->metadataOutput_ = sMetadataOutput_;

            std::shared_ptr<MetadataOutputCallback> callback =
                std::make_shared<MetadataOutputCallback>(MetadataOutputCallback(env));
            ((sptr<MetadataOutput> &)(obj->metadataOutput_))->SetCallback(callback);
            obj->metadataCallback_ = callback;

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               MetadataOutputNapi::MetadataOutputNapiDestructor, nullptr, &(obj->wrapper_));
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

bool MetadataOutputNapi::IsMetadataOutput(napi_env env, napi_value obj)
{
    bool result = false;
    napi_status status;
    napi_value constructor = nullptr;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        status = napi_instanceof(env, obj, constructor, &result);
        if (status != napi_ok) {
            result = false;
        }
    }

    return result;
}

sptr<MetadataOutput> MetadataOutputNapi::GetMetadataOutput()
{
    return metadataOutput_;
}

napi_value MetadataOutputNapi::CreateMetadataOutput(napi_env env)
{
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sMetadataOutput_ = CameraManager::GetInstance()->CreateMetadataOutput();
        if (sMetadataOutput_ == nullptr) {
            MEDIA_ERR_LOG("failed to create MetadataOutput");
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sMetadataOutput_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create metadata output instance");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

static void CommonCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<MetadataOutputAsyncContext*>(data);
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

static int32_t ConvertJSArrayToNative(napi_env env, size_t argc, const napi_value argv[], size_t &i,
    MetadataOutputAsyncContext &asyncContext)
{
    uint32_t len = 0;
    auto context = &asyncContext;

    if (napi_get_array_length(env, argv[i], &len) != napi_ok) {
        return -1;
    }

    for (uint32_t j = 0; j < len; j++) {
        napi_value metadataObjectType = nullptr;
        napi_valuetype valueType = napi_undefined;

        napi_get_element(env, argv[j], j, &metadataObjectType);
        napi_typeof(env, metadataObjectType, &valueType);
        if (valueType == napi_number) {
            bool isValid = true;
            int32_t metadataObjectTypeVal = 0;
            MetadataObjectType nativeMetadataObjType;
            napi_get_value_int32(env, metadataObjectType, &metadataObjectTypeVal);
            CameraNapiUtils::MapMetadataObjSupportedTypesEnumFromJS(
                metadataObjectTypeVal, nativeMetadataObjType, isValid);
            if (!isValid) {
                MEDIA_ERR_LOG("Unsupported metadata object type: napi object:%{public}d",
                    metadataObjectTypeVal);
                continue;
            }
            context->setSupportedMetadataObjectTypes.push_back(nativeMetadataObjType);
        }
        i++;
    }
    return static_cast<int32_t>(len);
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    MetadataOutputAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;
    bool isArray = false;
    int32_t ArrayLen = 0;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if ((i == PARAM0) && (napi_is_array(env, argv[i], &isArray) == napi_ok)
            && (isArray == true)) {
            ArrayLen = ConvertJSArrayToNative(env, argc, argv, i, asyncContext);
            if (ArrayLen == -1) {
                napi_get_boolean(env, false, &result);
                return result;
            }
        } else if ((i == static_cast<size_t>(PARAM1 + ArrayLen)) && (valueType == napi_function)) {
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

static void GetSupportedMetadataObjectTypesAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<MetadataOutputAsyncContext*>(data);
    napi_value metadataObjectTypes = nullptr;
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    size_t len = context->SupportedMetadataObjectTypes.size();
    if (context->SupportedMetadataObjectTypes.empty()
        || (napi_create_array(env, &metadataObjectTypes) != napi_ok)) {
        MEDIA_ERR_LOG("No Metadata object Types or create array failed!");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "No Metadata object Types or create array failed!", jsContext);
    }

    size_t i;
    size_t j = 0;
    for (i = 0; i < len; i++) {
        int32_t iProp;
        CameraNapiUtils::MapMetadataObjSupportedTypesEnum(context->SupportedMetadataObjectTypes[i], iProp);
        napi_value value;
        if (iProp != -1 && napi_create_int32(env, iProp, &value) == napi_ok) {
            napi_set_element(env, metadataObjectTypes, j, value);
            j++;
        }
    }
    jsContext->data = metadataObjectTypes;

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value MetadataOutputNapi::GetSupportedMetadataObjectTypes(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;
    int32_t refCount = 1;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameters maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<MetadataOutputAsyncContext> asyncContext = std::make_unique<MetadataOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetSupportedMetadataObjectTypes");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<MetadataOutputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->SupportedMetadataObjectTypes =
                        context->objectInfo->metadataOutput_->GetSupportedMetadataObjectTypes();
                    context->status = true;
                }
            },
            GetSupportedMetadataObjectTypesAsyncCallbackComplete,
            static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for PhotoOutputNapi::Release");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }
    return result;
}

napi_value MetadataOutputNapi::SetCapturingMetadataObjectTypes(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_TWO, "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    std::unique_ptr<MetadataOutputAsyncContext> asyncContext = std::make_unique<MetadataOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "SetCapturingMetadataObjectTypes");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<MetadataOutputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    context->objectInfo->metadataOutput_->SetCapturingMetadataObjectTypes(
                        context->setSupportedMetadataObjectTypes);
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()),
            &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for "
                "MetadataOutputNapi::SetCapturingMetadataObjectTypes");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }
    return result;
}

napi_value MetadataOutputNapi::Start(napi_env env, napi_callback_info info)
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
    std::unique_ptr<MetadataOutputAsyncContext> asyncContext = std::make_unique<MetadataOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Start");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<MetadataOutputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    context->objectInfo->metadataOutput_->Start();
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for MetadataOutputNapi::Start");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value MetadataOutputNapi::Stop(napi_env env, napi_callback_info info)
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
    std::unique_ptr<MetadataOutputAsyncContext> asyncContext = std::make_unique<MetadataOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Stop");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<MetadataOutputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    context->objectInfo->metadataOutput_->Stop();
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for MetadataOutputNapi::Stop");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value MetadataOutputNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    size_t argCount = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    size_t res = 0;
    char buffer[SIZE];
    std::string eventType;
    const int32_t refCount = 1;
    MetadataOutputNapi *obj = nullptr;
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

        if (!eventType.empty()) {
            obj->metadataCallback_->SetCallbackRef(eventType, callbackRef);
        } else {
            MEDIA_ERR_LOG("Failed to Register Callback: event type is empty!");
        }
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS