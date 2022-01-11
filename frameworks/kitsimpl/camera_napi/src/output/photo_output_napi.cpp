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
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CameraNapi"};
}

napi_ref PhotoOutputNapi::sConstructor_ = nullptr;
sptr<CaptureOutput> PhotoOutputNapi::sPhotoOutput_ = nullptr;
long PhotoOutputNapi::sSurfaceId_ = 0;
sptr<PhotoSurfaceListener> PhotoOutputNapi::listener = nullptr;

class PhotoOutputCallback : public PhotoCallback {
public:
    PhotoOutputCallback(napi_env env, napi_ref callbackRef)
    {
        env_ = env;
        callbackRef_ = callbackRef;
    }

private:
    napi_env env_;
    napi_ref callbackRef_;

    void UpdateJSCallback(std::string propName, const int32_t value) const
    {
        napi_value result[ARGS_TWO];
        napi_value callback = nullptr;
        napi_value retVal;
        napi_value propValue;

        napi_get_undefined(env_, &result[PARAM0]);

        napi_create_object(env_, &result[PARAM1]);
        napi_create_int32(env_, value, &propValue);

        napi_set_named_property(env_, result[PARAM1], propName.c_str(), propValue);

        napi_get_reference_value(env_, callbackRef_, &callback);
        napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
    }

    void OnCaptureStarted(const int32_t captureID) const override
    {
        MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureStarted() is called!, captureID: %{public}d", captureID);
        UpdateJSCallback("OnCaptureStarted", captureID);
    }

    void OnCaptureEnded(const int32_t captureID) const override
    {
        MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureEnded() is called!, captureID: %{public}d", captureID);
        UpdateJSCallback("OnCaptureEnded", captureID);
    }

    void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const override
    {
        UpdateJSCallback("OnFrameShutter", captureId);
    }

    void OnCaptureError(const int32_t captureId, const int32_t errorCode) const override
    {
        MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureError() is called!, captureID: %{public}d, errorCode: %{public}d",
                       captureId, errorCode);
        UpdateJSCallback("OnCaptureError", captureId);
    }
};

void PhotoSurfaceListener::OnBufferAvailable()
{
    MEDIA_INFO_LOG("OnBufferAvailable called!");
    int32_t flushFence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    captureSurface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
    if (buffer != nullptr) {
        const char *addr = static_cast<char *>(buffer->GetVirAddr());
        uint32_t size = buffer->GetSize();
        int32_t intResult = 0;
        intResult = SaveData(addr, size);
        if (intResult != 0) {
            MEDIA_ERR_LOG("Save Data Failed!");
        }
        captureSurface_->ReleaseBuffer(buffer, -1);
    } else {
        MEDIA_ERR_LOG("AcquireBuffer failed!");
    }
}

int32_t PhotoSurfaceListener::SaveData(const char *buffer, int32_t size)
{
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    struct tm *ltm = localtime(&tv.tv_sec);
    if (ltm != nullptr) {
        std::ostringstream ss("Capture_");
        std::string path;
        ss << "Capture" << ltm->tm_hour << "-" << ltm->tm_min << "-" << ltm->tm_sec << ".jpg";
        if (photoPath.empty()) {
            photoPath = "/data/media/";
        }
        path = photoPath + ss.str();
        std::ofstream pic(path, std::ofstream::out | std::ofstream::trunc);
        pic.write(buffer, size);
        if (pic.fail()) {
            MEDIA_ERR_LOG("Write Picture failed!");
            pic.close();
            return -1;
        }
        pic.close();
    }
    return 0;
}

void PhotoSurfaceListener::SetConsumerSurface(sptr<Surface> captureSurface)
{
    captureSurface_ = captureSurface;
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

long PhotoOutputNapi::GetSurfaceId()
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

napi_value PhotoOutputNapi::CreatePhotoOutput(napi_env env, long surfaceId)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;
    int32_t photoWidth = 4160;
    int32_t photoHeight = 3120;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sSurfaceId_ = surfaceId;
        sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
        if (surface == nullptr) {
            MEDIA_ERR_LOG("failed to get surface from XComponentManager");
            return result;
        }
        surface->SetDefaultWidthAndHeight(photoWidth, photoHeight);
        listener = new PhotoSurfaceListener();
        if (listener == nullptr) {
            MEDIA_ERR_LOG("Create Listener failed");
            return result;
        }
        listener->SetConsumerSurface(surface);
        surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
        surface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);
        sPhotoOutput_ = CameraManager::GetInstance()->CreatePhotoOutput(surface);
        if (sPhotoOutput_ == nullptr) {
            MEDIA_ERR_LOG("failed to create previewOutput");
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sSurfaceId_ = 0;
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
    napi_get_boolean(env, context->status, &jsContext->data);

    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                             context->work, *jsContext);
    }
    delete context;
}

static void GetFetchOptionsParam(napi_env env, napi_value arg, const PhotoOutputAsyncContext &context, bool &err)
{
    PhotoOutputAsyncContext *asyncContext = const_cast<PhotoOutputAsyncContext *>(&context);
    int32_t intValue;
    std::string strValue;
    bool boolValue;
    napi_value property = nullptr;
    napi_value location = nullptr;
    bool present = false;

    napi_has_named_property(env, arg, "quality", &present);
    if (present) {
        if (napi_get_named_property(env, arg, "quality", &property) != napi_ok
            || napi_get_value_int32(env, property, &intValue) != napi_ok) {
            HiLog::Error(LABEL, "Could not get the string argument!");
            err = true;
            asyncContext->quality = -1;
            return;
        } else {
            asyncContext->quality = intValue;
        }
        present = false;
    }

    napi_has_named_property(env, arg, "rotaion", &present);
    if (present) {
        intValue = 0;
        if (napi_get_named_property(env, arg, "rotaion", &property) != napi_ok
            || napi_get_value_int32(env, property, &intValue) != napi_ok) {
            HiLog::Error(LABEL, "Could not get the string argument!");
            err = true;
            asyncContext->rotaion = -1;
            return;
        } else {
            asyncContext->rotaion = intValue;
        }
        present = false;
    }

    napi_has_named_property(env, arg, "mirror", &present);
    if (present) {
        if (napi_get_named_property(env, arg, "mirror", &property) != napi_ok
            || napi_get_value_bool(env, property, &boolValue) != napi_ok) {
            HiLog::Error(LABEL, "Could not get the string argument!");
            err = true;
            asyncContext->mirror = -1;
            return;
        } else {
            asyncContext->mirror = boolValue ? 1 : 0;
        }
        present = false;
    }

    napi_has_named_property(env, arg, "location", &present);
    if (present && napi_get_named_property(env, arg, "location", &location) == napi_ok) {
    } else {
        HiLog::Error(LABEL, "Could not get the string argument!");
        err = true;
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
                    if (context->rotaion != -1) {
                        capSettings->SetRotation(static_cast<PhotoCaptureSetting::RotationConfig>(context->rotaion));
                    }
                    if (context->latitude != -1 && context->longitude != -1) {
                        capSettings->SetGpsLocation(context->latitude, context->longitude);
                    }
                    context->status = photoOutput->Capture(capSettings);
                } else {
                    context->status = photoOutput->Capture();
                }
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
    NAPI_ASSERT(env, argc <= 1, "requires 1 parameter maximum");

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
                context->status = true;
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

void PhotoOutputNapi::RegisterCallback(napi_env env, napi_ref callbackRef)
{
    if (callbackList_.empty()) {
        MEDIA_ERR_LOG("Failed to Register Callback callbackList is empty!");
        return;
    }

    for (std::string type : callbackList_) {
        if (type.compare("captureStart") || type.compare("captureEnd") || type.compare("frameShutter")) {
            std::shared_ptr<PhotoOutputCallback> callback =
                            std::make_shared<PhotoOutputCallback>(PhotoOutputCallback(env, callbackRef));
            ((sptr<PhotoOutput> &)(photoOutput_))->SetCallback(callback);
            MEDIA_INFO_LOG("Photo callback register successfully");
            break;
        }
    }
}

napi_value PhotoOutputNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    size_t argCount = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    size_t res = 0;
    uint32_t len = 0;
    char buffer[SIZE];
    std::string strItem;
    const int32_t refCount = 1;
    napi_value stringItem = nullptr;
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
        bool boolResult = false;
        if (napi_is_array(env, argv[PARAM0], &boolResult) != napi_ok || boolResult == false
            || napi_typeof(env, argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
            return undefinedResult;
        }

        napi_get_array_length(env, argv[PARAM0], &len);
        for (size_t i = 0; i < len; i++) {
            napi_get_element(env, argv[PARAM0], i, &stringItem);
            napi_get_value_string_utf8(env, stringItem, buffer, SIZE, &res);
            strItem = std::string(buffer);
            obj->callbackList_.push_back(strItem);
            if (memset_s(buffer, SIZE, 0, sizeof(buffer)) != 0) {
                MEDIA_ERR_LOG("Memset for buffer failed");
                return undefinedResult;
            }
        }

        napi_ref callbackRef;
        napi_create_reference(env, argv[PARAM1], refCount, &callbackRef);

        obj->RegisterCallback(env, callbackRef);
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS
