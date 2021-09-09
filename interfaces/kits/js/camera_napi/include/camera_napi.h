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

#ifndef CAMERA_NAPI_H_
#define CAMERA_NAPI_H_
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <sys/time.h>
#include <sstream>
#include "input/camera_manager.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include <sys/types.h>
#include <fcntl.h>
#include <fstream>
#include <securec.h>
#include "istream_operator_callback.h"
#include "istream_operator.h"
#include "media_log.h"
#include "window_manager.h"
#include "display_type.h"
#include "recorder.h"
namespace OHOS {
namespace CameraStandard {
struct CamRecorderCallback;

class SurfaceListener : public IBufferConsumerListener {
public:
    void OnBufferAvailable() override;

    int32_t SaveData(const char *buffer, int32_t size);

    int32_t SaveYUVPreview(const char *buffer, int32_t size);

    void SetConsumerSurface(sptr<Surface> InCaptureConSurface);

    void SetPhotoPath(std::string InPhotoPath);
private:

    sptr<Surface> captureConsumerSurface_;
    std::string photoPath;
};

struct MediaLocation {
    int32_t ilatitude;
    int32_t ilongitude;
};

static const std::int32_t SIZE = 100;
static const std::string CAMERA_MNGR_NAPI_CLASS_NAME = "Camera";
static const std::int32_t REFERENCE_CREATION_COUNT = 1;
static const std::int32_t ARGS_MAX_TWO_COUNT = 2;
static const std::int32_t SURFACE_QUEUE_SIZE = 10;
static const std::string CAM_CALLBACK_PREPARE = "prepare";
static const std::string CAM_CALLBACK_START_PREVIEW = "start_preview";
static const std::string CAM_CALLBACK_STOP_PREVIEW = "stop_preview";
static const std::string CAM_CALLBACK_PAUSE = "pause_video";
static const std::string CAM_CALLBACK_STOP = "stop_video";
static const std::string CAM_CALLBACK_RESUME = "resume_video";
static const std::string CAM_CALLBACK_START = "start_video";
static const std::string CAM_CALLBACK_RESET = "reset_video";
static const std::string CAM_CALLBACK_ERROR = "error";

static const std::vector<std::string> vecCameraPosition {
    "UNSPECIFIED", "BACK_CAMERA", "FRONT_CAMERA"
};
static const std::vector<std::string> vecCameraType {
    "CAMERATYPE_UNSPECIFIED", "CAMERATYPE_WIDE_ANGLE", "CAMERATYPE_ULTRA_WIDE",
    "CAMERATYPE_TELEPHOTO", "CAMERATYPE_TRUE_DEPTH", "CAMERATYPE_LOGICAL"
};
static const std::vector<std::string> vecExposureMode {
    "EXPOSUREMODE_MANUAL", "EXPOSUREMODE_CONTINUOUS_AUTO"
};
static const std::vector<std::string> vecFocusMode {
    "FOCUSMODE_MANUAL", "FOCUSMODE_CONTINUOUS_AUTO_FOCUS", "FOCUSMODE_AUTO_FOCUS", "FOCUSMODE_LOCKED"
};
static const std::vector<std::string> vecFlashMode {
    "FLASHMODE_CLOSE", "FLASHMODE_OPEN", "FLASHMODE_AUTO", "FLASHMODE_ALWAYS_OPEN"
};
static const std::vector<std::string> vecResolutionScale {
    "WIDTH_HEIGHT_4_3", "WIDTH_HEIGHT_16_9", "WIDTH_HEIGHT_1_1", "FULL_SCREEN"
};
static const std::vector<std::string> vecQuality {
    "QUALITY_HIGH", "QUALITY_NORMAL", "QUALITY_LOW"
};
static const std::vector<std::string> vecQualityLevel {
    "QUALITY_LEVEL_1080P", "QUALITY_LEVEL_2160P", "QUALITY_LEVEL_2K", "QUALITY_LEVEL_480P",
    "QUALITY_LEVEL_4KDCI", "QUALITY_LEVEL_720P", "QUALITY_LEVEL_CIF", "QUALITY_LEVEL_HIGH",
    "QUALITY_LEVEL_HIGH_SPEED_1080P", "QUALITY_LEVEL_HIGH_SPEED_2160P", "QUALITY_LEVEL_HIGH_SPEED_480P",
    "QUALITY_LEVEL_HIGH_SPEED_4KDCI", "QUALITY_LEVEL_HIGH_SPEED_720P", "QUALITY_LEVEL_HIGH_SPEED_CIF",
    "QUALITY_LEVEL_HIGH_SPEED_HIGH", "QUALITY_LEVEL_HIGH_SPEED_LOW", "QUALITY_LEVEL_HIGH_SPEED_VGA",
    "QUALITY_LEVEL_LOW", "QUALITY_LEVEL_QCIF", "QUALITY_LEVEL_QHD", "QUALITY_LEVEL_QVGA",
    "QUALITY_LEVEL_TIME_LAPSE_1080P", "QUALITY_LEVEL_TIME_LAPSE_2160P", "QUALITY_LEVEL_TIME_LAPSE_2K",
    "QUALITY_LEVEL_TIME_LAPSE_480P", "QUALITY_LEVEL_TIME_LAPSE_4KDCI", "QUALITY_LEVEL_TIME_LAPSE_720P",
    "QUALITY_LEVEL_TIME_LAPSE_CIF", "QUALITY_LEVEL_TIME_LAPSE_HIGH", "QUALITY_LEVEL_TIME_LAPSE_LOW",
    "QUALITY_LEVEL_TIME_LAPSE_QCIF", "QUALITY_LEVEL_TIME_LAPSE_QHD", "QUALITY_LEVEL_TIME_LAPSE_QVGA",
    "QUALITY_LEVEL_TIME_LAPSE_VGA", "QUALITY_LEVEL_VGA"
};
static const std::vector<std::string> vecFileFormat { "FORMAT_DEFAULT", "MP4", "FORMAT_M4A", "FORMAT_BUTT" };
static const std::vector<std::string> vecVideoEncoder { "H264" };
static const std::vector<std::string> vecAudioEncoder { "AAC_LC" };
static const std::vector<std::string> vecParameterResult { "ERROR_UNKNOWN", "PARAMETERS_RESULT"};

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num] = {0};    \
    napi_value thisVar = nullptr;  \
    void* data;                    \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

class CameraNapi {
public:
    enum CameraPosition {
        UNSPECIFIED = 1,
        BACK_CAMERA = 2,
        FRONT_CAMERA = 3,
    };

    enum CameraType {
        CAMERATYPE_UNSPECIFIED = 1,
        CAMERATYPE_WIDE_ANGLE = 2,
        CAMERATYPE_ULTRA_WIDE = 3,
        CAMERATYPE_TELEPHOTO = 4,
        CAMERATYPE_TRUE_DEPTH = 5,
        CAMERATYPE_LOGICAL = 6,
    };

    enum ExposureMode {
        EXPOSUREMODE_MANUAL = 1,
        EXPOSUREMODE_CONTINUOUS_AUTO = 2,
    };

    enum FocusMode {
        FOCUSMODE_MANUAL = 1,
        FOCUSMODE_CONTINUOUS_AUTO_FOCUS = 2,
        FOCUSMODE_AUTO_FOCUS = 3,
        FOCUSMODE_LOCKED = 4,
    };

    enum FlashMode {
        FLASHMODE_CLOSE = 1,
        FLASHMODE_OPEN = 2,
        FLASHMODE_AUTO = 3,
        FLASHMODE_ALWAYS_OPEN = 4,
    };

    enum ResolutionScale {
        WIDTH_HEIGHT_4_3 = 1,
        WIDTH_HEIGHT_16_9 = 2,
        WIDTH_HEIGHT_1_1 = 3,
        FULL_SCREEN = 4,
    };

    enum Quality {
        QUALITY_HIGH = 1,
        QUALITY_NORMAL = 2,
        QUALITY_LOW = 3,
    };

    enum QualityLevel {
        QUALITY_LEVEL_1080P = 1,
        QUALITY_LEVEL_2160P = 2,
        QUALITY_LEVEL_2K = 3,
        QUALITY_LEVEL_480P = 4,
        QUALITY_LEVEL_4KDCI = 5,
        QUALITY_LEVEL_720P = 6,
        QUALITY_LEVEL_CIF = 7,
        QUALITY_LEVEL_HIGH = 8,
        QUALITY_LEVEL_HIGH_SPEED_1080P = 9,
        QUALITY_LEVEL_HIGH_SPEED_2160P = 10,
        QUALITY_LEVEL_HIGH_SPEED_480P = 11,
        QUALITY_LEVEL_HIGH_SPEED_4KDCI = 12,
        QUALITY_LEVEL_HIGH_SPEED_720P = 13,
        QUALITY_LEVEL_HIGH_SPEED_CIF = 14,
        QUALITY_LEVEL_HIGH_SPEED_HIGH = 15,
        QUALITY_LEVEL_HIGH_SPEED_LOW = 16,
        QUALITY_LEVEL_HIGH_SPEED_VGA = 17,
        QUALITY_LEVEL_LOW = 18,
        QUALITY_LEVEL_QCIF = 19,
        QUALITY_LEVEL_QHD = 20,
        QUALITY_LEVEL_QVGA = 21,
        QUALITY_LEVEL_TIME_LAPSE_1080P = 22,
        QUALITY_LEVEL_TIME_LAPSE_2160P = 23,
        QUALITY_LEVEL_TIME_LAPSE_2K = 24,
        QUALITY_LEVEL_TIME_LAPSE_480P = 25,
        QUALITY_LEVEL_TIME_LAPSE_4KDCI = 26,
        QUALITY_LEVEL_TIME_LAPSE_720P = 27,
        QUALITY_LEVEL_TIME_LAPSE_CIF = 28,
        QUALITY_LEVEL_TIME_LAPSE_HIGH = 29,
        QUALITY_LEVEL_TIME_LAPSE_LOW = 30,
        QUALITY_LEVEL_TIME_LAPSE_QCIF = 31,
        QUALITY_LEVEL_TIME_LAPSE_QHD = 32,
        QUALITY_LEVEL_TIME_LAPSE_QVGA = 33,
        QUALITY_LEVEL_TIME_LAPSE_VGA = 34,
        QUALITY_LEVEL_VGA = 35,
    };

    enum FileFormat {
        FORMAT_DEFAULT = 0,
        FORMAT_MPEG_4,
        FORMAT_M4A,
        FORMAT_BUTT
    };

    enum PrepareType {
        PICTURE = 1,
        PREVIEW = 2,
        PICNPREVIEW = 3,
        RECORDER = 4
    };

    enum State: int32_t {
        STATE_IDLE,
        STATE_RUNNING,
        STATE_BUTT
    };

    enum VideoEncoder {
        VIDEO_DEFAULT = 0,
        H264 = 2,
        HEVC = 5,
        VIDEO_CODEC_FORMAT_BUTT
    };

    enum AudioEncoder {
        AUDIO_DEFAULT = 0,
        AAC_LC      =   1,
        AAC_HE_V1   =   2,
        AAC_HE_V2   =   3,
        AAC_LD      =   4,
        AAC_ELD     =   5,
        AUDIO_CODEC_FORMAT_BUTT
    };

    enum ParameterResultState {
        ERROR_UNKNOWN = 1,
        PARAMETERS_RESULT = 2,
    };

    enum VideoSourceType {
        VIDEO_SOURCE_SURFACE_YUV = 0,
        VIDEO_SOURCE_SURFACE_RGB,
        VIDEO_SOURCE_SURFACE_ES,
        VIDEO_SOURCE_BUTT
    };

    enum AudioSourceType {
        MIC = 1,
    };

    struct RecorderProfile {
        int32_t aBitRate;
        int32_t aChannels;
        int32_t aCodec;
        int32_t aSampleRate;
        int32_t durationTime;
        int32_t fileFormat;
        int32_t vBitRate;
        int32_t vCodec;
        int32_t vFrameHeight;
        int32_t vFrameRate;
        int32_t vFrameWidth;
        AudioSourceType aSourceType;
        VideoSourceType vSourceType;
    };

    struct RecorderConfig {
        std::string strVideoPath;
        std::string strThumbPath;
        bool bIsMuted;
        MediaLocation stLocation;
        std::unique_ptr<RecorderProfile> recProfile_;
    };

    struct PhotoConfig {
        std::string strPhotoPath;
        bool bIsMirror;
    };

    static napi_value Init(napi_env env, napi_value exports);
    napi_ref GetErrorCallbackRef();
    CameraNapi();
    ~CameraNapi();

private:
    int32_t InitCamera(std::string CameraID);
    static void CameraNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_status AddNamedProperty(napi_env env, napi_value object,
                                        const std::string name, int32_t enumValue);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value CreateCameraPositionObject(napi_env env);
    static napi_value CreateCameraWrapper(napi_env env, std::string strCamID);
    static napi_value CreateCamera(napi_env env, napi_callback_info info);
    static napi_value StartPreview(napi_env env, napi_callback_info info);
    static napi_value StopPreview(napi_env env, napi_callback_info info);
    void SaveCallbackReference(napi_env env, CameraNapi* camWrapper,
                               std::string callbackName, napi_value callback);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value TakePhoto(napi_env env, napi_callback_info info);
    static napi_value CreateCameraTypeObject(napi_env env);
    static napi_value CreateExposureModeObject(napi_env env);
    static napi_value CreateFocusModeObject(napi_env env);
    static napi_value CreateFlashModeObject(napi_env env);
    static napi_value CreateResolutionScaleObject(napi_env env);
    static napi_value CreateQualityObject(napi_env env);
    static napi_value CreateQualityLevelObject(napi_env env);
    static napi_value CreateFileFormatObject(napi_env env);
    static napi_value CreateVideoEncoderObject(napi_env env);
    static napi_value CreateAudioEncoderObject(napi_env env);
    static napi_value CreateParameterResultObject(napi_env env);
    static napi_value GetCameraIDs(napi_env env, napi_callback_info info);
    int32_t PreparePhoto(sptr<CameraManager> camManagerObj);
    sptr<Surface> PreparePreview();
    int32_t PrepareRecorder();
    int32_t PrepareVideo(sptr<CameraManager> camManagerObj);
    int32_t GetPhotoConfig(napi_env env, napi_value arg);
    int32_t GetProfileValue(napi_env env, napi_value arg);
    int32_t GetRecorderConfig(napi_env env, napi_value arg);
    void CloseRecorder();
    int32_t PrepareCommon(napi_env env, int32_t iPrepareType);
    void SetPhotoCallFlag();
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value StartVideoRecording(napi_env env, napi_callback_info info);
    static napi_value StopVideoRecording(napi_env env, napi_callback_info info);
    static napi_value PauseVideoRecording(napi_env env, napi_callback_info info);
    static napi_value ResumeVideoRecording(napi_env env, napi_callback_info info);
    static napi_value ResetVideoRecording(napi_env env, napi_callback_info info);
    static napi_value GetSupportedProperties(napi_env env, napi_callback_info info);
    static napi_value GetPropertyValue(napi_env env, napi_callback_info info);
    static napi_value GetSupportedResolutionScales(napi_env env, napi_callback_info info);
    static napi_value SetPreviewResolutionScale(napi_env env, napi_callback_info info);
    static napi_value SetPreviewQuality(napi_env env, napi_callback_info info);
    static napi_value GetSupportedExposureMode(napi_env env, napi_callback_info info);
    static napi_value SetExposureMode(napi_env env, napi_callback_info info);
    static napi_value GetSupportedFocusMode(napi_env env, napi_callback_info info);
    static napi_value SetFocusMode(napi_env env, napi_callback_info info);
    static napi_value GetSupportedFlashMode(napi_env env, napi_callback_info info);
    static napi_value SetFlashMode(napi_env env, napi_callback_info info);
    static napi_value GetSupportedZoomRange(napi_env env, napi_callback_info info);
    static napi_value SetZoom(napi_env env, napi_callback_info info);
    static napi_value SetParameter(napi_env env, napi_callback_info info);
    sptr<Surface> CreateSubWindowSurface();
    static napi_ref sCtor_;
    static napi_ref cameraPositionRef_;
    static napi_ref cameraTypeRef_;
    static napi_ref exposureModeRef_;
    static napi_ref focusModeRef_;
    static napi_ref flashModeRef_;
    static napi_ref resolutionScaleRef_;
    static napi_ref qualityRef_;
    static napi_ref qualityLevelRef_;
    static napi_ref fileFormatRef_;
    static napi_ref videoEncoderRef_;
    static napi_ref audioEncoderRef_;
    static napi_ref parameterResultRef_;
    napi_ref startPreviewCallback_ = nullptr;
    napi_ref stopPreviewCallback_ = nullptr;
    napi_ref startCallback_ = nullptr;
    napi_ref prepareCallback_ = nullptr;
    napi_ref pauseCallback_ = nullptr;
    napi_ref resumeCallback_ = nullptr;
    napi_ref stopCallback_ = nullptr;
    napi_ref resetCallback_ = nullptr;
    napi_ref errorCallback_ = nullptr;
    sptr<CaptureOutput> photoOutput_;
    sptr<CaptureOutput> previewOutput_;
    sptr<CaptureOutput> videoOutput_;
    sptr<SurfaceListener> listener;
    sptr<Surface> captureConsumerSurface_;
    std::unique_ptr<Window> previewWindow;
    std::unique_ptr<RecorderConfig> recConfig_;
    bool hasCallPreview_ = false;
    bool hasCallPhoto_ = false;
    bool isReady_ = false;
    std::unique_ptr<PhotoConfig> photoConfig_;
    sptr<Surface> previewSurface_;
    sptr<Surface> recorderSurface_;
    std::unique_ptr<OHOS::SubWindow> subWindow_;
    State previewState_ = State::STATE_IDLE;
    State recordState_ = State::STATE_IDLE;
    sptr<CaptureSession> capSession_;
    sptr<CaptureInput> camInput_;
    std::shared_ptr<Media::Recorder> recorder_;
    std::shared_ptr<CamRecorderCallback> nativeCallback_;
    int32_t aSourceID;
    int32_t vSourceID;
    napi_env env_;
    napi_ref wrapper_;
};
// Callback from native
struct CamRecorderCallback : public OHOS::Media::RecorderCallback {
    napi_env env_;
    CameraNapi *cameraWrapper_;

    CamRecorderCallback(napi_env environment = nullptr, CameraNapi *cameraWrapper = nullptr);
    virtual ~CamRecorderCallback() {}

    void OnError(OHOS::Media::RecorderErrorType errorType, int32_t errCode);
    void OnInfo(int32_t type, int32_t extra) {};
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_NAPI_H_ */
