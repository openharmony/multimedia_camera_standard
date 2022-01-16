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

#ifndef CAMERA_NAPI_UTILS_H_
#define CAMERA_NAPI_UTILS_H_

#include <vector>
#include "camera_device_ability_items.h"
#include "media_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#define CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar)                 \
    do {                                                                        \
        void* data;                                                             \
        napi_get_cb_info(env, info, &(argc), argv, &(thisVar), &data);          \
    } while (0)

#define CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar)                       \
    do {                                                                                        \
        void* data;                                                                             \
        status = napi_get_cb_info(env, info, nullptr, nullptr, &(thisVar), &data);              \
    } while (0)

#define CAMERA_NAPI_GET_JS_ASYNC_CB_REF(env, arg, count, cbRef)         \
    do {                                                                \
        napi_valuetype valueType = napi_undefined;                      \
        napi_typeof(env, arg, &valueType);                              \
        if (valueType == napi_function) {                               \
            napi_create_reference(env, arg, count, &(cbRef));           \
        } else {                                                        \
            NAPI_ASSERT(env, false, "type mismatch");                   \
        }                                                               \
    } while (0);

#define CAMERA_NAPI_ASSERT_NULLPTR_CHECK(env, result)       \
    do {                                                    \
        if ((result) == nullptr) {                          \
            napi_get_undefined(env, &(result));             \
            return result;                                  \
        }                                                   \
    } while (0);

#define CAMERA_NAPI_CREATE_PROMISE(env, callbackRef, deferred, result)      \
    do {                                                                    \
        if ((callbackRef) == nullptr) {                                     \
            napi_create_promise(env, &(deferred), &(result));               \
        }                                                                   \
    } while (0);

#define CAMERA_NAPI_CREATE_RESOURCE_NAME(env, resource, resourceName)                       \
    do {                                                                                    \
        napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &(resource));          \
    } while (0);

#define CAMERA_NAPI_CHECK_NULL_PTR_RETURN_UNDEFINED(env, ptr, ret, message)     \
    do {                                                            \
        if ((ptr) == nullptr) {                                     \
            HiLog::Error(LABEL, message);                           \
            napi_get_undefined(env, &(ret));                        \
            return ret;                                             \
        }                                                           \
    } while (0)

#define CAMERA_NAPI_CHECK_NULL_PTR_RETURN_VOID(ptr, message)   \
    do {                                           \
        if ((ptr) == nullptr) {                    \
            HiLog::Error(LABEL, message);          \
            return;                                \
        }                                          \
    } while (0)

#define CAMERA_NAPI_ASSERT_EQUAL(condition, errMsg)     \
    do {                                    \
        if (!(condition)) {                 \
            HiLog::Error(LABEL, errMsg);    \
            return;                         \
        }                                   \
    } while (0)

#define CAMERA_NAPI_CHECK_AND_BREAK_LOG(cond, fmt, ...)            \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_ERR_LOG(fmt, ##__VA_ARGS__);         \
            break;                                     \
        }                                              \
    } while (0)

#define CAMERA_NAPI_CHECK_AND_RETURN_LOG(cond, fmt, ...)           \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_ERR_LOG(fmt, ##__VA_ARGS__);         \
            return;                                    \
        }                                              \
    } while (0)


namespace OHOS {
namespace CameraStandard {
/* Constants for array index */
const int32_t PARAM0 = 0;
const int32_t PARAM1 = 1;
const int32_t PARAM2 = 2;

/* Constants for array size */
const int32_t ARGS_ONE = 1;
const int32_t ARGS_TWO = 2;
const int32_t ARGS_THREE = 3;
const int32_t SIZE = 100;
const int32_t REFERENCE_COUNT_ONE = 1;

struct JSAsyncContextOutput {
    napi_value error;
    napi_value data;
    bool status;
};

enum JSCameraPosition {
    CAMERA_POSITION_UNSPECIFIED = 0,
    CAMERA_POSITION_BACK,
    CAMERA_POSITION_FRONT
};

enum JSCameraType {
    CAMERA_TYPE_UNSPECIFIED = 0,
    CAMERA_TYPE_WIDE_ANGLE,
    CAMERA_TYPE_ULTRA_WIDE,
    CAMERA_TYPE_TELEPHOTO,
    CAMERA_TYPE_TRUE_DEAPTH
};

enum JSConnectionType {
    CAMERA_CONNECTION_BUILD_IN = 0,
    CAMERA_CONNECTION_USB_PLUGIN,
    CAMERA_CONNECTION_REMOTE
};

enum JSCameraFormat {
    CAMERA_FORMAT_YCRCb_420_SP = 1003,
    CAMERA_FORMAT_JPEG = 2000,
};

enum JSFlashMode {
    FLASH_MODE_CLOSE = 0,
    FLASH_MODE_OPEN,
    FLASH_MODE_AUTO,
    FLASH_MODE_ALWAYS_OPEN,
};

enum JSExposureMode {
    EXPOSURE_MODE_MANUAL = 0,
    EXPOSURE_MODE_CONTINUOUS_AUTO,
};

enum JSFocusMode {
    FOCUS_MODE_MANUAL = 0,
    FOCUS_MODE_CONTINUOUS_AUTO,
    FOCUS_MODE_AUTO,
    FOCUS_MODE_LOCKED,
};

enum JSFocusState {
    FOCUS_STATE_SCAN = 0,
    FOCUS_STATE_FOCUSED,
    FOCUS_STATE_UNFOCUSED
};

enum JSExposureState {
    EXPOSURE_STATE_SCAN = 0,
    EXPOSURE_STATE_CONVERGED
};

/* Util class used by napi asynchronous methods for making call to js callback function */
class CameraNapiUtils {
public:
    static void MapCameraPositionEnum(camera_position_enum_t nativeCamPos, int32_t &jsCameraPosition)
    {
        MEDIA_INFO_LOG("native cam pos = %{public}d", static_cast<int32_t>(nativeCamPos));
        switch (nativeCamPos) {
            case OHOS_CAMERA_POSITION_FRONT:
                jsCameraPosition = CAMERA_POSITION_FRONT;
                break;
            case OHOS_CAMERA_POSITION_BACK:
                jsCameraPosition = CAMERA_POSITION_BACK;
                break;
            case OHOS_CAMERA_POSITION_OTHER:
            default:
                jsCameraPosition = CAMERA_POSITION_UNSPECIFIED;
        }
    }

    static int32_t MapCameraPositionEnumFromJs(int32_t jsCameraPosition, camera_position_enum_t &nativeCamPos)
    {
        MEDIA_INFO_LOG("js cam pos = %{public}d", static_cast<int32_t>(jsCameraPosition));
        switch (jsCameraPosition) {
            case CAMERA_POSITION_FRONT:
                nativeCamPos = OHOS_CAMERA_POSITION_FRONT;
                break;
            case CAMERA_POSITION_BACK:
                nativeCamPos = OHOS_CAMERA_POSITION_BACK;
                break;
            case CAMERA_POSITION_UNSPECIFIED:
                nativeCamPos = OHOS_CAMERA_POSITION_OTHER;
                break;
            default:
                return -1;
        }

        return 0;
    }

    static void MapCameraFormatEnum(camera_format_t nativeCamFormat, int32_t &jsCameraFormat)
    {
        MEDIA_INFO_LOG("native cam format = %{public}d", static_cast<int32_t>(nativeCamFormat));
        switch (nativeCamFormat) {
            case OHOS_CAMERA_FORMAT_YCRCB_420_SP:
                jsCameraFormat = CAMERA_FORMAT_YCRCb_420_SP;
                break;
            case OHOS_CAMERA_FORMAT_JPEG:
                jsCameraFormat = CAMERA_FORMAT_JPEG;
                break;
            case OHOS_CAMERA_FORMAT_RGBA_8888:
            case OHOS_CAMERA_FORMAT_YCBCR_420_888:
            default:
                jsCameraFormat = -1;
        }
    }

    static int32_t MapCameraFormatEnumFromJs(int32_t jsCameraFormat, camera_format_t &nativeCamFormat)
    {
        MEDIA_INFO_LOG("js cam format = %{public}d", static_cast<int32_t>(jsCameraFormat));
        switch (jsCameraFormat) {
            case CAMERA_FORMAT_YCRCb_420_SP:
                nativeCamFormat = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
                break;
            case CAMERA_FORMAT_JPEG:
                nativeCamFormat = OHOS_CAMERA_FORMAT_JPEG;
                break;
            default:
                return -1;
        }

        return 0;
    }

    static void MapCameraTypeEnum(camera_type_enum_t nativeCamType, int32_t &jsCameraType)
    {
        MEDIA_INFO_LOG("native cam type = %{public}d", static_cast<int32_t>(nativeCamType));
        switch (nativeCamType) {
            case OHOS_CAMERA_TYPE_WIDE_ANGLE:
                jsCameraType = CAMERA_TYPE_WIDE_ANGLE;
                break;
            case OHOS_CAMERA_TYPE_ULTRA_WIDE:
                jsCameraType = CAMERA_TYPE_ULTRA_WIDE;
                break;
            case OHOS_CAMERA_TYPE_TELTPHOTO:
                jsCameraType = CAMERA_TYPE_TELEPHOTO;
                break;
            case OHOS_CAMERA_TYPE_TRUE_DEAPTH:
                jsCameraType = CAMERA_TYPE_TRUE_DEAPTH;
                break;
            case OHOS_CAMERA_TYPE_LOGICAL:
                jsCameraType = -1;
                break;
            case OHOS_CAMERA_TYPE_UNSPECIFIED:
            default:
                jsCameraType = CAMERA_TYPE_UNSPECIFIED;
        }
    }

    static int32_t MapCameraTypeEnumFromJs(int32_t jsCameraType, camera_type_enum_t &nativeCamType)
    {
        MEDIA_INFO_LOG("js cam type = %{public}d", static_cast<int32_t>(jsCameraType));
        switch (jsCameraType) {
            case CAMERA_TYPE_WIDE_ANGLE:
                nativeCamType = OHOS_CAMERA_TYPE_WIDE_ANGLE;
                break;
            case CAMERA_TYPE_ULTRA_WIDE:
                nativeCamType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
                break;
            case CAMERA_TYPE_TELEPHOTO:
                nativeCamType = OHOS_CAMERA_TYPE_TELTPHOTO;
                break;
            case CAMERA_TYPE_TRUE_DEAPTH:
                nativeCamType = OHOS_CAMERA_TYPE_TRUE_DEAPTH;
                break;
            case CAMERA_TYPE_UNSPECIFIED:
                nativeCamType = OHOS_CAMERA_TYPE_UNSPECIFIED;
                break;
            default:
                return -1;
        }

        return 0;
    }

    static void MapCameraConnectionTypeEnum(camera_connection_type_t nativeCamConnType, int32_t &jsCameraConnType)
    {
        MEDIA_INFO_LOG("native cam connection type = %{public}d", static_cast<int32_t>(nativeCamConnType));
        switch (nativeCamConnType) {
            case OHOS_CAMERA_CONNECTION_TYPE_REMOTE:
                jsCameraConnType = CAMERA_CONNECTION_REMOTE;
                break;
            case OHOS_CAMERA_CONNECTION_TYPE_USB_PLUGIN:
                jsCameraConnType = CAMERA_CONNECTION_USB_PLUGIN;
                break;
            case OHOS_CAMERA_CONNECTION_TYPE_BUILTIN:
            default:
                jsCameraConnType = CAMERA_CONNECTION_BUILD_IN;
        }
    }

    static void CreateNapiErrorObject(napi_env env, napi_value &errorObj,
        const int32_t errCode, const std::string errMsg)
    {
        napi_value napiErrorCode = nullptr;
        napi_value napiErrorMsg = nullptr;

        napi_create_int32(env, errCode, &napiErrorCode);
        napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &napiErrorMsg);
        napi_create_error(env, napiErrorCode, napiErrorMsg, &errorObj);
    }

    static void InvokeJSAsyncMethod(napi_env env, napi_deferred deferred,
        napi_ref callbackRef, napi_async_work work, const JSAsyncContextOutput &asyncContext)
    {
        napi_value retVal;
        napi_value callback = nullptr;

        /* Deferred is used when JS Callback method expects a promise value */
        if (deferred) {
            if (asyncContext.status) {
                napi_resolve_deferred(env, deferred, asyncContext.data);
            } else {
                napi_reject_deferred(env, deferred, asyncContext.error);
            }
        } else {
            napi_value result[ARGS_TWO];
            result[PARAM0] = asyncContext.error;
            result[PARAM1] = asyncContext.data;
            napi_get_reference_value(env, callbackRef, &callback);
            napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
            napi_delete_reference(env, callbackRef);
        }
        napi_delete_async_work(env, work);
    }
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_NAPI_UTILS_H_ */
