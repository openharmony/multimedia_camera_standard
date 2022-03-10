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

#include "output/photo_output_napi.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PhotoOutputNapi"};
}

napi_ref PhotoOutputNapi::sConstructor_ = nullptr;
sptr<CaptureOutput> PhotoOutputNapi::sPhotoOutput_ = nullptr;
std::string PhotoOutputNapi::sSurfaceId_ = "invalid";

PhotoOutputCallback::PhotoOutputCallback(napi_env env) : env_(env) {}

void PhotoOutputCallback::OnCaptureStarted(const int32_t captureID) const
{
    MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureStarted() is called!, captureID: %{public}d", captureID);
    CallbackInfo info;
    info.captureID = captureID;
    UpdateJSCallback("OnCaptureStarted", info);
}

void PhotoOutputCallback::OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const
{
    MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureEnded() is called!, captureID: %{public}d, frameCount: %{public}d",
                   captureID, frameCount);
    CallbackInfo info;
    info.captureID = captureID;
    info.frameCount = frameCount;
    UpdateJSCallback("OnCaptureEnded", info);
}

void PhotoOutputCallback::OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_INFO_LOG("PhotoOutputCallback:OnFrameShutter() called, captureID: %{public}d, timestamp: %{public}" PRIu64,
        captureId, timestamp);
    CallbackInfo info;
    info.captureID = captureId;
    info.timestamp = timestamp;
    UpdateJSCallback("OnFrameShutter", info);
}

void PhotoOutputCallback::OnCaptureError(const int32_t captureId, const int32_t errorCode) const
{
    MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureError() is called!, captureID: %{public}d, errorCode: %{public}d",
        captureId, errorCode);
    CallbackInfo info;
    info.captureID = captureId;
    info.errorCode = errorCode;
    UpdateJSCallback("OnCaptureError", info);
}

void PhotoOutputCallback::SetCallbackRef(const std::string &eventType, const napi_ref &callbackRef)
{
    if (eventType.compare("captureStart") == 0) {
        captureStartCallbackRef_ = callbackRef;
    } else if (eventType.compare("captureEnd") == 0) {
        captureEndCallbackRef_ = callbackRef;
    } else if (eventType.compare("frameShutter") == 0) {
        frameShutterCallbackRef_ = callbackRef;
    } else if (eventType.compare("error") == 0) {
        errorCallbackRef_ = callbackRef;
    } else {
        MEDIA_ERR_LOG("Incorrect photo callback event type received from JS");
    }
}

void PhotoOutputCallback::UpdateJSCallback(std::string propName, const CallbackInfo &info) const
{
    napi_value result[ARGS_TWO];
    napi_value callback = nullptr;
    napi_value retVal;
    napi_value propValue;

    napi_get_undefined(env_, &result[PARAM0]);

    if (propName.compare("OnCaptureStarted") == 0) {
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(captureStartCallbackRef_,
            "OnCaptureStart callback is not registered by JS");
        napi_create_int32(env_, info.captureID, &result[PARAM1]);
        napi_get_reference_value(env_, captureStartCallbackRef_, &callback);
    } else if (propName.compare("OnCaptureEnded") == 0) {
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(captureEndCallbackRef_,
            "OnCaptureEnd callback is not registered by JS");
        napi_create_object(env_, &result[PARAM1]);
        napi_create_int32(env_, info.captureID, &propValue);
        napi_set_named_property(env_, result[PARAM1], "captureId", propValue);
        napi_create_int32(env_, info.frameCount, &propValue);
        napi_set_named_property(env_, result[PARAM1], "frameCount", propValue);
        napi_get_reference_value(env_, captureEndCallbackRef_, &callback);
    } else if (propName.compare("OnFrameShutter") == 0) {
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(frameShutterCallbackRef_,
            "OnFrameShutter callback is not registered by JS");
        napi_create_object(env_, &result[PARAM1]);
        napi_create_int32(env_, info.captureID, &propValue);
        napi_set_named_property(env_, result[PARAM1], "captureId", propValue);
        napi_create_int64(env_, info.timestamp, &propValue);
        napi_set_named_property(env_, result[PARAM1], "timestamp", propValue);
        napi_get_reference_value(env_, frameShutterCallbackRef_, &callback);
    } else {
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(errorCallbackRef_,
            "OnError callback is not registered by JS");
        napi_create_object(env_, &result[PARAM1]);
        napi_create_int32(env_, info.errorCode, &propValue);
        napi_set_named_property(env_, result[PARAM1], "code", propValue);
        napi_get_reference_value(env_, errorCallbackRef_, &callback);
    }

    napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
}

PhotoOutputNapi::PhotoOutputNapi() : env_(nullptr), wrapper_(nullptr)
{
}

PhotoOutputNapi::~PhotoOutputNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void PhotoOutputNapi::PhotoOutputNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    PhotoOutputNapi *photoOutput = reinterpret_cast<PhotoOutputNapi*>(nativeObject);
    if (photoOutput != nullptr) {
        photoOutput->~PhotoOutputNapi();
    }
}

napi_value PhotoOutputNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor photo_output_props[] = {
        DECLARE_NAPI_FUNCTION("capture", Capture),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On)
    };

    status = napi_define_class(env, CAMERA_PHOTO_OUTPUT_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               PhotoOutputNapiConstructor, nullptr,
                               sizeof(photo_output_props) / sizeof(photo_output_props[PARAM0]),
                               photo_output_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_PHOTO_OUTPUT_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

// Constructor callback
napi_value PhotoOutputNapi::PhotoOutputNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<PhotoOutputNapi> obj = std::make_unique<PhotoOutputNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->photoOutput_ = sPhotoOutput_;
            obj->surfaceId_ = sSurfaceId_;

            std::shared_ptr<PhotoOutputCallback> callback =
                std::make_shared<PhotoOutputCallback>(PhotoOutputCallback(env));
            ((sptr<PhotoOutput> &)(obj->photoOutput_))->SetCallback(callback);
            obj->photoCallback_ = callback;

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               PhotoOutputNapi::PhotoOutputNapiDestructor, nullptr, &(obj->wrapper_));
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

sptr<CaptureOutput> PhotoOutputNapi::GetPhotoOutput()
{
    return photoOutput_;
}

std::string PhotoOutputNapi::GetSurfaceId()
{
    return surfaceId_;
}

bool PhotoOutputNapi::IsPhotoOutput(napi_env env, napi_value obj)
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

napi_value PhotoOutputNapi::CreatePhotoOutput(napi_env env, std::string surfaceId)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sSurfaceId_ = surfaceId;
        MEDIA_INFO_LOG("CreatePhotoOutput surfaceId: %{public}s", surfaceId.c_str());
        sptr<Surface> surface = Media::ImageReceiver::getSurfaceById(surfaceId);
        if (surface == nullptr) {
            MEDIA_ERR_LOG("failed to get surface from ImageReceiver");
            return result;
        }
        MEDIA_INFO_LOG("surface width: %{public}d, height: %{public}d", surface->GetDefaultWidth(),
                       surface->GetDefaultHeight());
#ifdef RK_CAMERA
        surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_RGBA_8888));
#else
        surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_JPEG));
#endif
        CameraManager::GetInstance()->SetPermissionCheck(true);
        sPhotoOutput_ = CameraManager::GetInstance()->CreatePhotoOutput(surface);
        if (sPhotoOutput_ == nullptr) {
            MEDIA_ERR_LOG("failed to create CreatePhotoOutput");
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sPhotoOutput_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create photo output instance");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

static void CommonCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<PhotoOutputAsyncContext*>(data);
    if (context == nullptr) {
        MEDIA_ERR_LOG("Async context is null");
        return;
    }

    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);
    napi_get_undefined(env, &jsContext->data);

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

int32_t QueryAndGetProperty(napi_env env, napi_value arg, const string &propertyName, napi_value &property)
{
    bool present = false;
    int32_t retval = 0;
    if ((napi_has_named_property(env, arg, propertyName.c_str(), &present) != napi_ok)
        || (!present) || (napi_get_named_property(env, arg, propertyName.c_str(), &property) != napi_ok)) {
            HiLog::Error(LABEL, "Failed to obtain property: %{public}s", propertyName.c_str());
            retval = -1;
    }

    return retval;
}

int32_t GetLocationProperties(napi_env env, napi_value locationObj, const PhotoOutputAsyncContext &context)
{
    PhotoOutputAsyncContext *asyncContext = const_cast<PhotoOutputAsyncContext *>(&context);
    napi_value property1 = nullptr;
    napi_value property2 = nullptr;
    double latitude = -1.0;
    double longitude = -1.0;

    if ((QueryAndGetProperty(env, locationObj, "latitude", property1) == 0) &&
        (QueryAndGetProperty(env, locationObj, "longitude", property2) == 0)) {
        if ((napi_get_value_double(env, property1, &latitude) != napi_ok) ||
            (napi_get_value_double(env, property2, &longitude) != napi_ok)) {
            return -1;
        } else {
            asyncContext->latitude = latitude;
            asyncContext->longitude = longitude;
        }
    } else {
        return -1;
    }

    // Return 0 after location properties are successfully obtained
    return 0;
}

static void GetFetchOptionsParam(napi_env env, napi_value arg, const PhotoOutputAsyncContext &context, bool &err)
{
    PhotoOutputAsyncContext *asyncContext = const_cast<PhotoOutputAsyncContext *>(&context);
    int32_t intValue;
    std::string strValue;
    bool boolValue;
    napi_value property = nullptr;
    PhotoCaptureSetting::QualityLevel quality;
    PhotoCaptureSetting::RotationConfig rotation;

    if (QueryAndGetProperty(env, arg, "quality", property) == 0) {
        if (napi_get_value_int32(env, property, &intValue) != napi_ok
            || CameraNapiUtils::MapQualityLevelFromJs(intValue, quality) == -1) {
            err = true;
            return;
        } else {
            asyncContext->quality = intValue;
        }
    }

    if (QueryAndGetProperty(env, arg, "rotation", property) == 0) {
        if (napi_get_value_int32(env, property, &intValue) != napi_ok
            || CameraNapiUtils::MapImageRotationFromJs(intValue, rotation) == -1) {
            err = true;
            return;
        } else {
            asyncContext->rotation = intValue;
        }
    }

    if (QueryAndGetProperty(env, arg, "mirror", property) == 0) {
        if (napi_get_value_bool(env, property, &boolValue) != napi_ok) {
            err = true;
            return;
        } else {
            asyncContext->mirror = boolValue ? 1 : 0;
        }
    }

    if (QueryAndGetProperty(env, arg, "location", property) == 0) {
        if (GetLocationProperties(env, property, context) == -1) {
            err = true;
            return;
        }
    }
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    PhotoOutputAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;
    bool err;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_object) {
            GetFetchOptionsParam(env, argv[PARAM0], asyncContext, err);
            if (err) {
                HiLog::Error(LABEL, "fetch options retrieval failed");
                NAPI_ASSERT(env, false, "type mismatch");
            }
            asyncContext.hasPhotoSettings = true;
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

napi_value PhotoOutputNapi::Capture(napi_env env, napi_callback_info info)
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
    unique_ptr<PhotoOutputAsyncContext> asyncContext = make_unique<PhotoOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Capture");
        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                PhotoOutputAsyncContext* context = static_cast<PhotoOutputAsyncContext*>(data);
                sptr<PhotoOutput> photoOutput = ((sptr<PhotoOutput> &)(context->objectInfo->photoOutput_));
                if (context->hasPhotoSettings) {
                    std::shared_ptr<PhotoCaptureSetting> capSettings = make_shared<PhotoCaptureSetting>();
                    if (context->mirror != -1) {
                        capSettings->SetMirror(context->mirror);
                    }

                    if (context->quality != -1) {
                        capSettings->SetQuality(static_cast<PhotoCaptureSetting::QualityLevel>(context->quality));
                    }

                    if (context->rotation != -1) {
                        capSettings->SetRotation(static_cast<PhotoCaptureSetting::RotationConfig>(context->rotation));
                    }

                    if (context->latitude != -1.0 && context->longitude != -1.0) {
                        capSettings->SetGpsLocation(context->latitude, context->longitude);
                    }

                    context->status = photoOutput->Capture(capSettings);
                } else {
                    context->status = photoOutput->Capture();
                }
            }, CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value PhotoOutputNapi::Release(napi_env env, napi_callback_info info)
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
    std::unique_ptr<PhotoOutputAsyncContext> asyncContext = std::make_unique<PhotoOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Release");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<PhotoOutputAsyncContext*>(data);
                ((sptr<PhotoOutput> &)(context->objectInfo->photoOutput_))->Release();
                context->status = 0;
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value PhotoOutputNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    size_t argCount = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    size_t res = 0;
    char buffer[SIZE];
    std::string eventType;
    const int32_t refCount = 1;
    PhotoOutputNapi *obj = nullptr;
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
            obj->photoCallback_->SetCallbackRef(eventType, callbackRef);
        } else {
            MEDIA_ERR_LOG("Failed to Register Callback: event type is empty!");
        }
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS
