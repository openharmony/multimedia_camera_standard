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

#include "hilog/log.h"
#include "input/camera_input_napi.h"
#include "output/metadata_object_napi.h"

namespace OHOS {
namespace CameraStandard {
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MetadataObjectNapi"};
}

thread_local napi_ref MetadataObjectNapi::sConstructor_ = nullptr;
thread_local sptr<MetadataObject> g_metadataObject;

napi_value MetadataObjectNapi::CreateMetaFaceObj(napi_env env, sptr<MetadataObject> metaObj)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        g_metadataObject = metaObj;
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        g_metadataObject = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create Camera obj instance");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

MetadataObjectNapi::MetadataObjectNapi() : env_(nullptr), wrapper_(nullptr)
{
}

MetadataObjectNapi::~MetadataObjectNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void MetadataObjectNapi::MetadataObjectNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    MetadataObjectNapi *metadataObject = reinterpret_cast<MetadataObjectNapi*>(nativeObject);
    if (metadataObject != nullptr) {
        metadataObject->~MetadataObjectNapi();
    }
}

napi_value MetadataObjectNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor metadata_object_props[] = {
        DECLARE_NAPI_FUNCTION("getType", GetType),
        DECLARE_NAPI_FUNCTION("getTimestamp", GetTimestamp),
        DECLARE_NAPI_FUNCTION("getBoundingBox", GetBoundingBox),
    };

    status = napi_define_class(env, CAMERA_METADATA_OBJECT_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               MetadataObjectNapiConstructor, nullptr,
                               sizeof(metadata_object_props) / sizeof(metadata_object_props[PARAM0]),
                               metadata_object_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_METADATA_OBJECT_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value MetadataObjectNapi::MetadataObjectNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<MetadataObjectNapi> obj = std::make_unique<MetadataObjectNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->metadataObject_ = g_metadataObject;
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               MetadataObjectNapi::MetadataObjectNapiDestructor, nullptr, &(obj->wrapper_));
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

sptr<MetadataObject> MetadataObjectNapi::GetMetadataObject()
{
    return metadataObject_;
}

static void GetTypeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<MetadataObjectAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    int32_t iProp;
    CameraNapiUtils::MapMetadataObjSupportedTypesEnum(context->metadataObjType, iProp);
    status = napi_create_int32(env, iProp, &jsContext->data);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("GetTypeAsyncCallbackComplete:napi_create_int32() failed");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "GetTypeAsyncCallbackComplete:napi_create_int32() failed", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value MetadataObjectNapi::GetType(napi_env env, napi_callback_info info)
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
    std::unique_ptr<MetadataObjectAsyncContext> asyncContext = std::make_unique<MetadataObjectAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetType");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<MetadataObjectAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->status = true;
                    context->metadataObjType = context->objectInfo->metadataObject_->GetType();
                }
            },
            GetTypeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for MetadataObjectNapi::GetType");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetTimestampAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<MetadataObjectAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    status = napi_create_double(env, context->metaTimestamp, &jsContext->data);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("GetTimestampAsyncCallbackComplete:napi_create_double() failed");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "GetTimestampAsyncCallbackComplete:napi_create_double() failed", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value MetadataObjectNapi::GetTimestamp(napi_env env, napi_callback_info info)
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
    std::unique_ptr<MetadataObjectAsyncContext> asyncContext = std::make_unique<MetadataObjectAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetTimestamp");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<MetadataObjectAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->status = true;
                    context->metaTimestamp
                        = context->objectInfo->metadataObject_->GetTimestamp();
                }
            },
            GetTimestampAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for MetadataObjectNapi::GetTimestamp");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetBoundingBoxAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    napi_value propValue;
    napi_status retStatVal;

    auto context = static_cast<MetadataObjectAsyncContext*>(data);
    CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    retStatVal = napi_create_object(env, &jsContext->data);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete napi_create_object failed");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete napi_create_object failed", jsContext);
    }

    retStatVal = napi_create_double(env, context->metaFace.topLeftX, &propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete napi_create_double "
            "failed for topLeftX");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete napi_create_double "
            "failed for topLeftX", jsContext);
    }

    retStatVal = napi_set_named_property(env, jsContext->data, "topLeftX", propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for topLeftX");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for topLeftX", jsContext);
    }

    retStatVal = napi_create_double(env, context->metaFace.topLeftY, &propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_create_double failed for topLeftY");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_create_double failed for topLeftY", jsContext);
    }

    retStatVal = napi_set_named_property(env, jsContext->data, "topLeftY", propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for topLeftY");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for topLeftY", jsContext);
    }

    retStatVal = napi_create_double(env, context->metaFace.width, &propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_create_double failed for width");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_create_double failed for width", jsContext);
    }

    retStatVal = napi_set_named_property(env, jsContext->data, "width", propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for width");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for width", jsContext);
    }

    retStatVal = napi_create_double(env, context->metaFace.height, &propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_create_double failed for height");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_create_double failed for height", jsContext);
    }

    retStatVal = napi_set_named_property(env, jsContext->data, "height", propValue);
    if (retStatVal != napi_ok) {
        MEDIA_ERR_LOG("MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for height");
        CameraNapiUtils::CreateNapiErrorObject(env,
            "MetadataObjectNapi::GetBoundingBoxAsyncCallbackComplete "
            "napi_set_named_property failed for height", jsContext);
    }

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

napi_value MetadataObjectNapi::GetBoundingBox(napi_env env, napi_callback_info info)
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
    std::unique_ptr<MetadataObjectAsyncContext> asyncContext = std::make_unique<MetadataObjectAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "GetBoundingBox");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<MetadataObjectAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->status = true;
                    context->metaFace
                        = context->objectInfo->metadataObject_->GetBoundingBox();
                }
            },
            GetBoundingBoxAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for MetadataObjectNapi::GetBoundingBox");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}
} // namespace CameraStandard
} // namespace OHOS