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


#include "output/video_output_napi.h"
#include <uv.h>
#include "hilog/log.h"

namespace OHOS {
namespace CameraStandard {
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoOutputNapi"};
}

napi_ref VideoOutputNapi::sConstructor_ = nullptr;
sptr<CaptureOutput> VideoOutputNapi::sVideoOutput_ = nullptr;
uint64_t VideoOutputNapi::sSurfaceId_ = 0;
sptr<SurfaceListener> VideoOutputNapi::listener = nullptr;

VideoCallbackListener::VideoCallbackListener(napi_env env) : env_(env) {}

void VideoCallbackListener::UpdateJSCallbackAsync(std::string propName, const int32_t value) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("VideoCallbackListener:UpdateJSCallbackAsync() failed to get event loop");
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("VideoCallbackListener:UpdateJSCallbackAsync() failed to allocate work");
        return;
    }
    VideoOutputCallbackInfo *callbackInfo = new(std::nothrow) VideoOutputCallbackInfo(propName, value, this);
    if (!callbackInfo) {
        MEDIA_ERR_LOG("VideoCallbackListener:UpdateJSCallbackAsync() failed to allocate callback info");
        delete work;
        return;
    }
    work->data = callbackInfo;
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        VideoOutputCallbackInfo *callbackInfo = reinterpret_cast<VideoOutputCallbackInfo *>(work->data);
        if (callbackInfo) {
            callbackInfo->listener_->UpdateJSCallback(callbackInfo->eventName_, callbackInfo->value_);
            delete callbackInfo;
        }
        delete work;
    });
    if (ret) {
        MEDIA_ERR_LOG("VideoCallbackListener:UpdateJSCallbackAsync() failed to execute work");
        delete callbackInfo;
        delete work;
    }
}

void VideoCallbackListener::OnFrameStarted() const
{
    MEDIA_INFO_LOG("VideoCallbackListener::OnFrameStarted");
    UpdateJSCallbackAsync("OnFrameStarted", -1);
}

void VideoCallbackListener::OnFrameEnded(const int32_t frameCount) const
{
    MEDIA_INFO_LOG("VideoCallbackListener::OnFrameEnded frameCount: %{public}d", frameCount);
    UpdateJSCallbackAsync("OnFrameEnded", frameCount);
}

void VideoCallbackListener::OnError(const int32_t errorCode) const
{
    MEDIA_INFO_LOG("VideoCallbackListener::OnError errorCode: %{public}d", errorCode);
    UpdateJSCallbackAsync("OnError", errorCode);
}

void VideoCallbackListener::SetCallbackRef(const std::string &eventType, const napi_ref &callbackRef)
{
    if (eventType.compare("frameStart") == 0) {
        frameStartCallbackRef_ = callbackRef;
    } else if (eventType.compare("frameEnd") == 0) {
        frameEndCallbackRef_ = callbackRef;
    } else if (eventType.compare("error") == 0) {
        errorCallbackRef_ = callbackRef;
    } else {
        MEDIA_ERR_LOG("Incorrect video callback event type received from JS");
    }
}

void VideoCallbackListener::UpdateJSCallback(std::string propName, const int32_t value) const
{
    napi_value result[ARGS_TWO];
    napi_value callback = nullptr;
    napi_value retVal;
    napi_value propValue;
    int32_t jsErrorCodeUnknown = -1;

    napi_get_undefined(env_, &result[PARAM0]);

    if (propName.compare("OnFrameStarted") == 0) {
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(frameStartCallbackRef_,
            "OnFrameStart callback is not registered by JS");
        napi_get_undefined(env_, &result[PARAM1]);
        napi_get_reference_value(env_, frameStartCallbackRef_, &callback);
    } else if (propName.compare("OnFrameEnded") == 0) {
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(frameEndCallbackRef_,
            "OnFrameEnd callback is not registered by JS");
        napi_get_undefined(env_, &result[PARAM1]);
        napi_get_reference_value(env_, frameEndCallbackRef_, &callback);
    } else {
        CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(errorCallbackRef_,
            "OnError callback is not registered by JS");
        napi_create_object(env_, &result[PARAM1]);
        napi_create_int32(env_, jsErrorCodeUnknown, &propValue);
        napi_set_named_property(env_, result[PARAM1], "code", propValue);
        napi_get_reference_value(env_, errorCallbackRef_, &callback); // should errorcode be valued as -1
    }

    napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
}

void SurfaceListener::OnBufferAvailable()
{
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

int32_t SurfaceListener::SaveData(const char *buffer, int32_t size)
{
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    struct tm *ltm = localtime(&tv.tv_sec);
    if (ltm != nullptr) {
        std::ostringstream ss("Video_");
        std::string path;
        ss << "Video_" << ltm->tm_hour << "-" << ltm->tm_min << "-" << ltm->tm_sec << ".h264";
        if (photoPath.empty()) {
            photoPath = "/data/media/";
        }
        path = photoPath + ss.str();
        std::ofstream vid(path, std::ofstream::out | std::ofstream::trunc);
        vid.write(buffer, size);
        if (vid.fail()) {
            MEDIA_ERR_LOG("Write videe failed!");
            vid.close();
            return -1;
        }
        vid.close();
    }
    return 0;
}

void SurfaceListener::SetConsumerSurface(sptr<Surface> captureSurface)
{
    captureSurface_ = captureSurface;
}

VideoOutputNapi::VideoOutputNapi() : env_(nullptr), wrapper_(nullptr)
{
}

VideoOutputNapi::~VideoOutputNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void VideoOutputNapi::VideoOutputNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    VideoOutputNapi *videoOutput = reinterpret_cast<VideoOutputNapi*>(nativeObject);
    if (videoOutput != nullptr) {
        videoOutput->~VideoOutputNapi();
    }
}

napi_value VideoOutputNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor video_output_props[] = {
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On)
    };

    status = napi_define_class(env, CAMERA_VIDEO_OUTPUT_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               VideoOutputNapiConstructor, nullptr,
                               sizeof(video_output_props) / sizeof(video_output_props[PARAM0]),
                               video_output_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_VIDEO_OUTPUT_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

// Constructor callback
napi_value VideoOutputNapi::VideoOutputNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<VideoOutputNapi> obj = std::make_unique<VideoOutputNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->videoOutput_ = sVideoOutput_;
            obj->surfaceId_ = sSurfaceId_;

            std::shared_ptr<VideoCallbackListener> callback =
                            std::make_shared<VideoCallbackListener>(VideoCallbackListener(env));
            ((sptr<VideoOutput> &)(obj->videoOutput_))->SetCallback(callback);
            obj->videoCallback_ = callback;

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               VideoOutputNapi::VideoOutputNapiDestructor, nullptr, &(obj->wrapper_));
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

static void CommonCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<VideoOutputAsyncContext*>(data);

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
            napi_get_boolean(env, context->status, &jsContext->data);
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

sptr<CaptureOutput> VideoOutputNapi::GetVideoOutput()
{
    return videoOutput_;
}

bool VideoOutputNapi::IsVideoOutput(napi_env env, napi_value obj)
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

napi_value VideoOutputNapi::CreateVideoOutput(napi_env env, uint64_t surfaceId)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sSurfaceId_ = surfaceId;
        sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(surfaceId);
        if (surface == nullptr) {
            MEDIA_ERR_LOG("failed to get surface from SurfaceUtils");
            return result;
        }
#ifdef RK_CAMERA
        surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_RGBA_8888));
#else
        surface->SetUserData(CameraManager::surfaceFormat, std::to_string(OHOS_CAMERA_FORMAT_YCRCB_420_SP));
#endif
        sVideoOutput_ = CameraManager::GetInstance()->CreateVideoOutput(surface);
        if (sVideoOutput_ == nullptr) {
            MEDIA_ERR_LOG("failed to create VideoOutput");
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sSurfaceId_ = 0;
        sVideoOutput_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create video output instance");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value VideoOutputNapi::Start(napi_env env, napi_callback_info info)
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
    std::unique_ptr<VideoOutputAsyncContext> asyncContext = std::make_unique<VideoOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Start");
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<VideoOutputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = ((sptr<VideoOutput> &)(context->objectInfo->videoOutput_))->Start();
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "VideoOutputNapi::Start failure";
                    }
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for VideoOutputNapi::Start");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value VideoOutputNapi::Stop(napi_env env, napi_callback_info info)
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
    std::unique_ptr<VideoOutputAsyncContext> asyncContext = std::make_unique<VideoOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Stop");
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                auto context = static_cast<VideoOutputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    int32_t ret = ((sptr<VideoOutput> &)(context->objectInfo->videoOutput_))->Stop();
                    if (ret != 0) {
                        context->status = false;
                        context->errorMsg = "VideoOutputNapi::Stop failure";
                    }
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for VideoOutputNapi::Stop");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value VideoOutputNapi::Release(napi_env env, napi_callback_info info)
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
    std::unique_ptr<VideoOutputAsyncContext> asyncContext = std::make_unique<VideoOutputAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }

        CAMERA_NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, "Release");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void* data) {
                auto context = static_cast<VideoOutputAsyncContext*>(data);
                context->status = false;
                if (context->objectInfo != nullptr) {
                    context->bRetBool = false;
                    context->status = true;
                    ((sptr<VideoOutput> &)(context->objectInfo->videoOutput_))->Release();
                }
            },
            CommonCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_ERR_LOG("Failed to create napi_create_async_work for VideoOutputNapi::Release");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value VideoOutputNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    size_t argCount = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    size_t res = 0;
    char buffer[SIZE];
    std::string eventType;
    const int32_t refCount = 1;
    VideoOutputNapi *obj = nullptr;
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
            obj->videoCallback_->SetCallbackRef(eventType, callbackRef);
        } else {
            MEDIA_ERR_LOG("Failed to Register Callback: event type is empty!");
        }
    }

    return undefinedResult;
}
} // namespace CameraStandard
} // namespace OHOS
