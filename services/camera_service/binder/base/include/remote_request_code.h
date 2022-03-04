/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_REMOTE_REQUEST_CODE_H
#define OHOS_CAMERA_REMOTE_REQUEST_CODE_H

namespace OHOS {
namespace CameraStandard {
/**
 * @brief Camera device remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum CameraDeviceRequestCode {
    CAMERA_DEVICE_OPEN = 0,
    CAMERA_DEVICE_CLOSE,
    CAMERA_DEVICE_RELEASE,
    CAMERA_DEVICE_SET_CALLBACK,
    CAMERA_DEVICE_UPDATE_SETTNGS,
    CAMERA_DEVICE_GET_ENABLED_RESULT,
    CAMERA_DEVICE_ENABLED_RESULT,
    CAMERA_DEVICE_DISABLED_RESULT
};

/**
 * @brief Camera service callback remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum CameraServiceCallbackRequestCode {
    CAMERA_CALLBACK_STATUS_CHANGED = 0,
    CAMERA_CALLBACK_FLASHLIGHT_STATUS_CHANGED
};

/**
 * @brief Camera service remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum CameraServiceRequestCode {
    CAMERA_SERVICE_CREATE_DEVICE = 0,
    CAMERA_SERVICE_SET_CALLBACK,
    CAMERA_SERVICE_GET_CAMERAS,
    CAMERA_SERVICE_CREATE_CAPTURE_SESSION,
    CAMERA_SERVICE_CREATE_PHOTO_OUTPUT,
    CAMERA_SERVICE_CREATE_PREVIEW_OUTPUT,
    CAMERA_SERVICE_CREATE_PREVIEW_OUTPUT_CUSTOM_SIZE,
    CAMERA_SERVICE_CREATE_VIDEO_OUTPUT,
    CAMERA_SERVICE_SET_LISTENER_OBJ
};

/**
 * @brief Capture session remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum CaptureSessionRequestCode {
    CAMERA_CAPTURE_SESSION_BEGIN_CONFIG = 0,
    CAMERA_CAPTURE_SESSION_ADD_INPUT,
    CAMERA_CAPTURE_SESSION_ADD_OUTPUT_CAPTURE,
    CAMERA_CAPTURE_SESSION_ADD_OUTPUT_REPEAT,
    CAMERA_CAPTURE_SESSION_REMOVE_INPUT,
    CAMERA_CAPTURE_SESSION_REMOVE_OUTPUT_CAPTURE,
    CAMERA_CAPTURE_SESSION_REMOVE_OUTPUT_REPEAT,
    CAMERA_CAPTURE_SESSION_COMMIT_CONFIG,
    CAMERA_CAPTURE_SESSION_START,
    CAMERA_CAPTURE_SESSION_STOP,
    CAMERA_CAPTURE_SESSION_RELEASE,
    CAMERA_CAPTURE_SESSION_SET_CALLBACK
};

/**
 * @brief Stream capture remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum StreamCaptureRequestCode {
    CAMERA_STREAM_CAPTURE_START = 0,
    CAMERA_STREAM_CAPTURE_CANCEL,
    CAMERA_STREAM_CAPTURE_SET_CALLBACK,
    CAMERA_STREAM_CAPTURE_RELEASE
};

/**
 * @brief Stream repeat remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum StreamRepeatRequestCode {
    CAMERA_START_VIDEO_RECORDING = 0,
    CAMERA_STOP_VIDEO_RECORDING,
    CAMERA_STREAM_REPEAT_SET_FPS,
    CAMERA_STREAM_REPEAT_SET_CALLBACK,
    CAMERA_STREAM_REPEAT_RELEASE
};

/**
 * @brief Camera device callback remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum CameraDeviceCallbackRequestCode {
    CAMERA_DEVICE_ON_ERROR = 0,
    CAMERA_DEVICE_ON_RESULT
};

/**
 * @brief Camera repeat stream callback remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum StreamRepeatCallbackRequestCode {
    CAMERA_STREAM_REPEAT_ON_FRAME_STARTED = 0,
    CAMERA_STREAM_REPEAT_ON_FRAME_ENDED,
    CAMERA_STREAM_REPEAT_ON_ERROR
};

/**
 * @brief Camera capture stream callback remote request code for IPC.
 *
 * @since 1.0
 * @version 1.0
 */
enum StreamCaptureCallbackRequestCode {
    CAMERA_STREAM_CAPTURE_ON_CAPTURE_STARTED = 0,
    CAMERA_STREAM_CAPTURE_ON_CAPTURE_ENDED,
    CAMERA_STREAM_CAPTURE_ON_CAPTURE_ERROR,
    CAMERA_STREAM_CAPTURE_ON_FRAME_SHUTTER
};

/**
* @brief Capture session callback remote request code for IPC.
*
* @since 1.0
* @version 1.0
*/
enum CaptureSessionCallbackRequestCode {
    CAMERA_CAPTURE_SESSION_ON_ERROR = 0
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_REMOTE_REQUEST_CODE_H
