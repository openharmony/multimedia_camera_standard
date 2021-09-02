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

#include "camera_napi.h"
#include "hilog/log.h"

namespace OHOS {
namespace CameraStandard {
using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

napi_ref CameraNapi::sCtor_ = nullptr;
napi_ref CameraNapi::cameraPositionRef_ = nullptr;
napi_ref CameraNapi::cameraTypeRef_ = nullptr;
napi_ref CameraNapi::exposureModeRef_ = nullptr;
napi_ref CameraNapi::focusModeRef_ = nullptr;
napi_ref CameraNapi::flashModeRef_ = nullptr;
napi_ref CameraNapi::resolutionScaleRef_ = nullptr;
napi_ref CameraNapi::qualityRef_ = nullptr;
napi_ref CameraNapi::qualityLevelRef_ = nullptr;
napi_ref CameraNapi::fileFormatRef_ = nullptr;
napi_ref CameraNapi::videoEncoderRef_ = nullptr;
napi_ref CameraNapi::audioEncoderRef_ = nullptr;
napi_ref CameraNapi::parameterResultRef_ = nullptr;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CameraNapi"};
    const int ARGS_TWO = 2;
    const int ARGS_THREE = 3;
}

struct CameraNapiAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef = nullptr;
    int32_t status;
    int32_t iExposureMode;
    int32_t iFocusMode;
    int32_t iFlashMode;
    double dZoomRatio;
    int32_t iResScale;
    int32_t iQuality;
    CameraNapi* objectInfo;
    std::vector<sptr<OHOS::CameraStandard::CameraInfo>> cameraObjList;
    std::vector<std::string> vecSupportedPropertyList;
    std::vector<CameraNapi::ResolutionScale> vecSupportedResolutionScalesList;
    std::vector<camera_exposure_mode_enum_t> vecSupportedExposureModeList;
    std::vector<camera_focus_mode_enum_t> vecSupportedFocusModeList;
    std::vector<camera_flash_mode_enum_t> vecSupportedFlashModeList;
    std::vector<float> vecSupportedZoomRangeList;
};

void SurfaceListener::OnBufferAvailable()
{
    int32_t flushFence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    captureConsumerSurface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
    if (buffer != nullptr) {
        const char *addr = static_cast<char *>(buffer->GetVirAddr());
        uint32_t size = buffer->GetSize();
        int32_t intResult = 0;
        intResult = SaveData(addr, size);
        if (intResult != 0) {
            MEDIA_ERR_LOG("Save Data Failed!");
        }
        captureConsumerSurface_->ReleaseBuffer(buffer, -1);
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
        std::ostringstream ss("Capture_");
        std::string path;
        ss << "Capture" << ltm->tm_hour << "-" << ltm->tm_min << "-" << ltm->tm_sec << ".jpg";
        if (photoPath.empty()) {
            photoPath = "/data/";
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

void SurfaceListener::SetPhotoPath(std::string InPhotoPath)
{
    photoPath = InPhotoPath;
}

void SurfaceListener::SetConsumerSurface(sptr<Surface> InCaptureConsumerSurface)
{
    captureConsumerSurface_ = InCaptureConsumerSurface;
}

int32_t SurfaceListener::SaveYUVPreview(const char *buffer, int32_t size)
{
    std::ostringstream ss("Preview_raw.yuv");
    std::ofstream preview;
    preview.open("/data/" + ss.str(), std::ofstream::out | std::ofstream::app);
    preview.write(buffer, size);
    preview.close();

    return 0;
}

CamRecorderCallback::CamRecorderCallback(napi_env environment, CameraNapi *cameraWrapper)
    : env_(environment), cameraWrapper_(cameraWrapper)
{
}

void CamRecorderCallback::OnError(int32_t errorType, int32_t errCode)
{
    size_t argCount = 1;
    napi_value jsCallback = nullptr;
    napi_status status;
    napi_value args[1], result, errorCodeVal, errorTypeVal;
    std::string errCodeStr, errTypeStr;

    if (cameraWrapper_->GetErrorCallbackRef() == nullptr) {
        HiLog::Error(LABEL, "No error callback reference received from JS");
        return;
    }

    status = napi_get_reference_value(env_, cameraWrapper_->GetErrorCallbackRef(), &jsCallback);
    if (status == napi_ok && jsCallback != nullptr) {
        errCodeStr = std::to_string(errCode);
        errTypeStr = std::to_string(errorType);
        if (napi_create_string_utf8(env_, errCodeStr.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal) == napi_ok
            && napi_create_string_utf8(env_, errTypeStr.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal) == napi_ok) {
            status = napi_create_error(env_, errorCodeVal, errorTypeVal, &args[0]);
            if (status == napi_ok) {
                status = napi_call_function(env_, nullptr, jsCallback, argCount, args, &result);
                if (status == napi_ok) {
                    return;
                }
            }
        }
    }

    HiLog::Error(LABEL, "Failed to call error callback!");
}

CameraNapi::CameraNapi() : env_(nullptr), wrapper_(nullptr), capSession_(nullptr)
{
    recConfig_ = nullptr;
    recorder_ = nullptr;
    photoConfig_ = nullptr;
    listener = nullptr;
    previewOutput_ = nullptr;
    photoOutput_ = nullptr;
    camInput_ = nullptr;
    vSourceID = 0;
    aSourceID = 0;
}

CameraNapi::~CameraNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    if (errorCallback_ != nullptr) {
        napi_delete_reference(env_, errorCallback_);
    }
    if (startCallback_ != nullptr) {
        napi_delete_reference(env_, startCallback_);
    }
    if (stopCallback_ != nullptr) {
        napi_delete_reference(env_, stopCallback_);
    }
    if (pauseCallback_ != nullptr) {
        napi_delete_reference(env_, pauseCallback_);
    }
    if (resumeCallback_ != nullptr) {
        napi_delete_reference(env_, resumeCallback_);
    }
    if (resetCallback_ != nullptr) {
        napi_delete_reference(env_, resetCallback_);
    }
    if (prepareCallback_ != nullptr) {
        napi_delete_reference(env_, prepareCallback_);
    }
    if (startPreviewCallback_ != nullptr) {
        napi_delete_reference(env_, startPreviewCallback_);
    }
    if (stopPreviewCallback_ != nullptr) {
        napi_delete_reference(env_, stopPreviewCallback_);
    }
}

napi_status CameraNapi::AddNamedProperty(napi_env env, napi_value object,
                                         const std::string name, int32_t enumValue)
{
    napi_status status;
    napi_value enumNapiValue;

    status = napi_create_int32(env, enumValue, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), enumNapiValue);
    }

    return status;
}

void CameraNapi::CameraNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        CameraNapi *obj = static_cast<CameraNapi *>(nativeObject);
        delete obj;
    }
}

napi_value CameraNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctor;
    napi_value result = nullptr;
    napi_property_descriptor camera_mngr_properties[] = {
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("takePhoto", TakePhoto),
        DECLARE_NAPI_FUNCTION("startPreview", StartPreview),
        DECLARE_NAPI_FUNCTION("stopPreview", StopPreview),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("startVideoRecording", StartVideoRecording),
        DECLARE_NAPI_FUNCTION("stopVideoRecording", StopVideoRecording),
        DECLARE_NAPI_FUNCTION("pauseVideoRecording", PauseVideoRecording),
        DECLARE_NAPI_FUNCTION("resumeVideoRecording", ResumeVideoRecording),
        DECLARE_NAPI_FUNCTION("resetVideoRecording", ResetVideoRecording),
        DECLARE_NAPI_FUNCTION("getSupportedProperties", GetSupportedProperties),
        DECLARE_NAPI_FUNCTION("getPropertyValue", GetPropertyValue),
        DECLARE_NAPI_FUNCTION("getSupportedResolutionScales", GetSupportedResolutionScales),
        DECLARE_NAPI_FUNCTION("setPreviewResolutionScale", SetPreviewResolutionScale),
        DECLARE_NAPI_FUNCTION("setPreviewQuality", SetPreviewQuality),
        DECLARE_NAPI_FUNCTION("getSupportedExposureMode", GetSupportedExposureMode),
        DECLARE_NAPI_FUNCTION("setExposureMode", SetExposureMode),
        DECLARE_NAPI_FUNCTION("getSupportedFocusMode", GetSupportedFocusMode),
        DECLARE_NAPI_FUNCTION("setFocusMode", SetFocusMode),
        DECLARE_NAPI_FUNCTION("getSupportedFlashMode", GetSupportedFlashMode),
        DECLARE_NAPI_FUNCTION("setFlashMode", SetFlashMode),
        DECLARE_NAPI_FUNCTION("getSupportedZoomRange", GetSupportedZoomRange),
        DECLARE_NAPI_FUNCTION("setZoom", SetZoom)
    };
    napi_property_descriptor camera_static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getCameraIDs", GetCameraIDs),
        DECLARE_NAPI_STATIC_FUNCTION("createCamera", CreateCamera),
        DECLARE_NAPI_PROPERTY("CameraPosition", CreateCameraPositionObject(env)),
        DECLARE_NAPI_PROPERTY("CameraType", CreateCameraTypeObject(env)),
        DECLARE_NAPI_PROPERTY("ExposureMode", CreateExposureModeObject(env)),
        DECLARE_NAPI_PROPERTY("FocusMode", CreateFocusModeObject(env)),
        DECLARE_NAPI_PROPERTY("FlashMode", CreateFlashModeObject(env)),
        DECLARE_NAPI_PROPERTY("ResolutionScale", CreateResolutionScaleObject(env)),
        DECLARE_NAPI_PROPERTY("Quality", CreateQualityObject(env)),
        DECLARE_NAPI_PROPERTY("QualityLevel", CreateQualityLevelObject(env)),
        DECLARE_NAPI_PROPERTY("FileFormat", CreateFileFormatObject(env)),
        DECLARE_NAPI_PROPERTY("VideoEncoder", CreateVideoEncoderObject(env)),
        DECLARE_NAPI_PROPERTY("AudioEncoder", CreateAudioEncoderObject(env)),
        DECLARE_NAPI_PROPERTY("ParameterResultState", CreateParameterResultObject(env))
    };

    status = napi_define_class(env, CAMERA_MNGR_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct, nullptr,
        sizeof(camera_mngr_properties) / sizeof(camera_mngr_properties[0]),
        camera_mngr_properties, &ctor);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctor, 1, &sCtor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_MNGR_NAPI_CLASS_NAME.c_str(), ctor);
            if (status == napi_ok) {
                status = napi_define_properties(env, exports,
                                                sizeof(camera_static_prop) / sizeof(camera_static_prop[0]),
                                                camera_static_prop);
                if (status == napi_ok) {
                    HiLog::Info(LABEL, "All props and functions are configured..");
                    return exports;
                }
            }
        }
    }
    HiLog::Error(LABEL, "Failure in CameraManagerNapi::Init()");
    napi_get_undefined(env, &result);

    return result;
}

static OHOS::Media::AudioSourceType GetNativeAudioSourceType(int32_t iAudioSourceType)
{
    OHOS::Media::AudioSourceType result;

    switch (iAudioSourceType) {
        case CameraNapi::MIC:
            result = OHOS::Media::AudioSourceType::AUDIO_MIC;
            break;

        default:
            result = OHOS::Media::AudioSourceType::AUDIO_MIC;
            HiLog::Error(LABEL, "Unknown Audio Source type!, %{public}d", iAudioSourceType);
            break;
    }
    return result;
}

static OHOS::Media::VideoSourceType GetNativeVideoSourceType(int32_t iVideoSourceType)
{
    OHOS::Media::VideoSourceType result;

    switch (iVideoSourceType) {
        case CameraNapi::VIDEO_SOURCE_SURFACE_YUV:
            result = OHOS::Media::VideoSourceType::VIDEO_SOURCE_SURFACE_YUV;
            break;
        case CameraNapi::VIDEO_SOURCE_SURFACE_RGB:
            result = OHOS::Media::VideoSourceType::VIDEO_SOURCE_SURFACE_RGB;
            break;
        case CameraNapi::VIDEO_SOURCE_SURFACE_ES:
            result = OHOS::Media::VideoSourceType::VIDEO_SOURCE_SURFACE_ES;
            break;
        case CameraNapi::VIDEO_SOURCE_BUTT:
            result = OHOS::Media::VideoSourceType::VIDEO_SOURCE_BUTT;
            break;
        default:
            result = OHOS::Media::VideoSourceType::VIDEO_SOURCE_BUTT;
            HiLog::Error(LABEL, "Unknown Audio Source type!, %{public}d", iVideoSourceType);
            break;
    }

    return result;
}

static OHOS::Media::VideoCodecFormat GetNativeVideoCodecFormat(int32_t iVideoCodecFormat)
{
    OHOS::Media::VideoCodecFormat result;

    switch (iVideoCodecFormat) {
        case CameraNapi::VIDEO_DEFAULT:
            result = OHOS::Media::VideoCodecFormat::VIDEO_DEFAULT;
            break;
        case CameraNapi::H264:
            result = OHOS::Media::VideoCodecFormat::H264;
            break;
        case CameraNapi::HEVC:
            result = OHOS::Media::VideoCodecFormat::HEVC;
            break;
        case CameraNapi::VIDEO_CODEC_FORMAT_BUTT:
            result = OHOS::Media::VideoCodecFormat::VIDEO_CODEC_FORMAT_BUTT;
            break;
        default:
            result = OHOS::Media::VideoCodecFormat::VIDEO_CODEC_FORMAT_BUTT;
            HiLog::Error(LABEL, "Unknown Audio Source type!, %{public}d", iVideoCodecFormat);
            break;
    }

    return result;
}

static OHOS::Media::AudioCodecFormat GetNativeAudioCodecFormat(int32_t iAudioCodecFormat)
{
    OHOS::Media::AudioCodecFormat result;

    switch (iAudioCodecFormat) {
        case CameraNapi::AUDIO_DEFAULT:
            result = OHOS::Media::AudioCodecFormat::AUDIO_DEFAULT;
            break;
        case CameraNapi::AAC_LC:
            result = OHOS::Media::AudioCodecFormat::AAC_LC;
            break;
        case CameraNapi::AAC_HE_V1:
            result = OHOS::Media::AudioCodecFormat::AAC_HE_V1;
            break;
        case CameraNapi::AAC_HE_V2:
            result = OHOS::Media::AudioCodecFormat::AAC_HE_V2;
            break;
        case CameraNapi::AAC_LD:
            result = OHOS::Media::AudioCodecFormat::AAC_LD;
            break;
        case CameraNapi::AAC_ELD:
            result = OHOS::Media::AudioCodecFormat::AAC_ELD;
            break;
        case CameraNapi::AUDIO_CODEC_FORMAT_BUTT:
            result = OHOS::Media::AudioCodecFormat::AUDIO_CODEC_FORMAT_BUTT;
            break;
        default:
            result = OHOS::Media::AudioCodecFormat::AUDIO_CODEC_FORMAT_BUTT;
            HiLog::Error(LABEL, "Unknown Audio Source type!, %{public}d", iAudioCodecFormat);
            break;
    }

    return result;
}

// Function to read string argument from napi_value
static std::string GetStringArgument(napi_env env, napi_value value)
{
    napi_status status;
    std::string strValue = "";
    size_t bufLength = 0;
    char *buffer = nullptr;

    // get buffer length first and get buffer based on length
    status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0) {
        // Create a buffer and create std::string later from it
        buffer = (char *)malloc((bufLength + 1) * sizeof(char));
        if (buffer != nullptr) {
            status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
            if (status == napi_ok) {
                strValue = buffer;
            }
            free(buffer);
            buffer = nullptr;
        }
    }

    return strValue;
}

void CameraNapi::SaveCallbackReference(napi_env env, CameraNapi* camWrapper,
    std::string callbackName, napi_value callback)
{
    napi_ref *ref = nullptr;
    napi_status status;
    int32_t refCount = 1;

    if (callbackName == CAM_CALLBACK_START) {
        ref = &(camWrapper->startCallback_);
    } else if (callbackName == CAM_CALLBACK_PAUSE) {
        ref = &(camWrapper->pauseCallback_);
    } else if (callbackName == CAM_CALLBACK_RESUME) {
        ref = &(camWrapper->resumeCallback_);
    } else if (callbackName == CAM_CALLBACK_STOP) {
        ref = &(camWrapper->stopCallback_);
    } else if (callbackName == CAM_CALLBACK_RESET) {
        ref = &(camWrapper->resetCallback_);
    } else if (callbackName == CAM_CALLBACK_ERROR) {
        ref = &(camWrapper->errorCallback_);
    } else if (callbackName == CAM_CALLBACK_PREPARE) {
        ref = &(camWrapper->prepareCallback_);
    } else if (callbackName == CAM_CALLBACK_START_PREVIEW) {
        ref = &(camWrapper->startPreviewCallback_);
    } else if (callbackName == CAM_CALLBACK_STOP_PREVIEW) {
        ref = &(camWrapper->stopPreviewCallback_);
    } else {
        HiLog::Error(LABEL, "Unknown callback!!, %{public}s", callbackName.c_str());
        return;
    }

    status = napi_create_reference(env, callback, refCount, ref);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Unknown error while creating reference for callback!");
    }
}

napi_ref CameraNapi::GetErrorCallbackRef()
{
    napi_ref errorCb = errorCallback_;
    return errorCb;
}

napi_value CameraNapi::On(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = ARGS_TWO;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_value args[ARGS_TWO] = {0};
    CameraNapi* cameraWrapper = nullptr;
    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    std::string callbackName;

    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&cameraWrapper));
    if (status == napi_ok) {
        if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string
            || napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
            HiLog::Error(LABEL, "Invalid arguments type!");
            return undefinedResult;
        }

        callbackName = GetStringArgument(env, args[0]);
        cameraWrapper->SaveCallbackReference(env, cameraWrapper, callbackName, args[1]);
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to set callback");
    return undefinedResult;
}

int32_t CameraNapi::GetProfileValue(napi_env env, napi_value arg)
{
    napi_value temp = nullptr;
    int32_t iTemp;
    recConfig_->recProfile_ = std::make_unique<RecorderProfile>();
    if (recConfig_->recProfile_ == nullptr) {
        HiLog::Error(LABEL, "Recorder Profile Allocation Failed!");
        return -1;
    }
    bool bIsPresent = false;
    if (napi_get_named_property(env, arg, "aBitRate", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the abitrate argument!");
        return -1;
    } else {
        recConfig_->recProfile_->aBitRate = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "aChannels", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the aChannels argument!");
        return -1;
    } else {
        recConfig_->recProfile_->aChannels = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "aCodec", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the aCodec argument!");
        return -1;
    } else {
        recConfig_->recProfile_->aCodec = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "aSampleRate", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the aSampleRate argument!");
        return -1;
    } else {
        recConfig_->recProfile_->aSampleRate = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "durationTime", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the durationTime argument!");
        return -1;
    } else {
        recConfig_->recProfile_->durationTime = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "fileFormat", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the fileFormat argument!");
        return -1;
    } else {
        recConfig_->recProfile_->fileFormat = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "vBitRate", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the vBitRate argument!");
        return -1;
    } else {
        recConfig_->recProfile_->vBitRate = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "vCodec", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the vCodec argument!");
        return -1;
    } else {
        recConfig_->recProfile_->vCodec = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "vFrameHeight", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the vFrameHeight argument!");
        return -1;
    } else {
        recConfig_->recProfile_->vFrameHeight = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "vFrameRate", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the vFrameRate argument!");
        return -1;
    } else {
        recConfig_->recProfile_->vFrameRate = iTemp;
        iTemp = 0;
    }
    if (napi_get_named_property(env, arg, "vFrameWidth", &temp) != napi_ok
        || napi_get_value_int32(env, temp, &iTemp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the vFrameWidth argument!");
        return -1;
    } else {
        recConfig_->recProfile_->vFrameWidth = iTemp;
        iTemp = 0;
    }
    napi_has_named_property(env, arg, "audioSourceType", &bIsPresent);
    if (!bIsPresent) {
        HiLog::Debug(LABEL, "Set default Value for Audio Source Type");
        iTemp = 1;
        recConfig_->recProfile_->aSourceType = static_cast<CameraNapi::AudioSourceType>(iTemp);
        iTemp = 0;
    } else {
        if ((napi_get_named_property(env, arg, "audioSourceType", &temp) != napi_ok
            || napi_get_value_int32(env, temp, &iTemp) != napi_ok)) {
            HiLog::Error(LABEL, "Could not get the audio Source Type argument!");
            return -1;
        } else {
            recConfig_->recProfile_->aSourceType = static_cast<CameraNapi::AudioSourceType>(iTemp);
            iTemp = 0;
            bIsPresent = false;
        }
    }
    napi_has_named_property(env, arg, "videoSourceType", &bIsPresent);
    if (!bIsPresent) {
        HiLog::Debug(LABEL, "Set default Value for Audio Source Type");
        iTemp = 1;
        recConfig_->recProfile_->vSourceType = static_cast<CameraNapi::VideoSourceType>(iTemp);
        iTemp = 0;
    } else {
        if ((napi_get_named_property(env, arg, "videoSourceType", &temp) != napi_ok
            || napi_get_value_int32(env, temp, &iTemp) != napi_ok)) {
            HiLog::Error(LABEL, "Could not get the vSource Type argument!");
            return -1;
        } else {
            recConfig_->recProfile_->vSourceType = static_cast<CameraNapi::VideoSourceType>(iTemp);
            iTemp = 0;
            bIsPresent = false;
        }
    }
    return 0;
}

int32_t CameraNapi::GetRecorderConfig(napi_env env, napi_value arg)
{
    char buffer[SIZE];
    size_t res = 0;
    uint32_t len;
    napi_value temp, tmediaLocation, trecProf;
    bool bIsMuted = false;
    bool bIsPresent = false;
    napi_status status = napi_ok;
    int32_t iLocation = 0;
    int32_t intResult = -1;
    if (recConfig_ != nullptr) {
        HiLog::Error(LABEL, "Recorder Config needs reset");
        recConfig_.reset();
    }
    recConfig_ = std::make_unique<RecorderConfig>();
    if (recConfig_ == nullptr) {
        HiLog::Error(LABEL, "Recorder Config Alloc failed");
        return -1;
    }
    if (napi_get_named_property(env, arg, "videoPath", &temp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the videoPath argument!");
        return -1;
    }
    recConfig_->strVideoPath = GetStringArgument(env, temp);
    if (recConfig_->strVideoPath.empty()) {
        HiLog::Error(LABEL, "Set Default videoPath!");
        recConfig_->strVideoPath = "/data/";
    }
    if (napi_get_named_property(env, arg, "thumbPath", &temp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the thumbPath argument!");
        return -1;
    }
    recConfig_->strThumbPath = GetStringArgument(env, temp);
    if (recConfig_->strThumbPath.empty()) {
        HiLog::Error(LABEL, "Set Default thumbPath!");
        recConfig_->strThumbPath = "/data/";
    }

    napi_has_named_property(env, arg, "muted", &bIsPresent);
    if (bIsPresent) {
        if ((napi_get_named_property(env, arg, "muted", &temp) == napi_ok)) {
            if (napi_get_value_bool(env, temp, &bIsMuted) != napi_ok) {
                HiLog::Debug(LABEL, "Could not get the muted Argument");
            }
        }
    }
    recConfig_->bIsMuted = bIsMuted;
    bIsPresent = false;

    if (napi_get_named_property(env, arg, "profile", &trecProf) == napi_ok) {
        intResult = GetProfileValue(env, trecProf);
        if (intResult != 0) {
            HiLog::Error(LABEL, "Could not get the Profile argument!");
            return -1;
        }
    }

    return 0;
}

int32_t CameraNapi::PrepareVideo(sptr<CameraManager> camManagerObj)
{
    int32_t ret = 0;
    if ((recorder_ == nullptr) || (recConfig_ == nullptr) || (recConfig_->recProfile_ == nullptr)) {
        HiLog::Error(LABEL, "Recorder Create is not Proper!");
        return -1;
    }
    ret = PrepareRecorder();
    if (ret != 0) {
        HiLog::Error(LABEL, "Prepare Recorder failed");
        return -1;
    }
    recorderSurface_ = (recorder_->GetSurface(vSourceID));
    if (recorderSurface_ == nullptr) {
        HiLog::Error(LABEL, "Get Recorder Surface failed");
        return -1;
    }
    videoOutput_ = camManagerObj->CreateVideoOutput(recorderSurface_);
    if (videoOutput_ == nullptr) {
        HiLog::Error(LABEL, "Create Video Output failed");
        return -1;
    }
    return 0;
}
int32_t CameraNapi::PrepareRecorder()
{
    int32_t ret = 0;
    aSourceID = 0;
    vSourceID = 0;
    ret = recorder_->SetVideoSource(GetNativeVideoSourceType(recConfig_->recProfile_->vSourceType), vSourceID);
    if (ret != 0) {
        return ret;
    }
    if (!recConfig_->bIsMuted) {
        ret = recorder_->SetAudioSource(GetNativeAudioSourceType(recConfig_->recProfile_->aSourceType), aSourceID);
        if (ret != 0) {
            return ret;
        }
    }

    ret = recorder_->SetOutputFormat(static_cast<OHOS::Media::OutputFormatType>(recConfig_->recProfile_->fileFormat));
    if (ret != 0) {
        return ret;
    }
    ret = recorder_->SetVideoEncoder(vSourceID,
                                     static_cast<OHOS::Media::VideoCodecFormat>(recConfig_->recProfile_->vCodec));
    if (ret != 0) {
        return ret;
    }
    ret = recorder_->SetVideoSize(vSourceID, recConfig_->recProfile_->vFrameWidth,
                                  recConfig_->recProfile_->vFrameHeight);
    if (ret != 0) {
        return ret;
    }
    ret = recorder_->SetVideoFrameRate(vSourceID, recConfig_->recProfile_->vFrameRate);
    if (ret != 0) {
        return ret;
    }
    ret = recorder_->SetVideoEncodingBitRate(vSourceID, recConfig_->recProfile_->vBitRate);
    if (ret != 0) {
        return ret;
    }
    ret = recorder_->SetMaxDuration(recConfig_->recProfile_->durationTime);
    if (ret != 0) {
        return ret;
    }
    if (!(recConfig_->bIsMuted)) {
        ret = recorder_->SetAudioEncoder(aSourceID,
                                         static_cast<OHOS::Media::AudioCodecFormat>(recConfig_->recProfile_->aCodec));
        if (ret != 0) {
            return ret;
        }
        ret = recorder_->SetAudioSampleRate(aSourceID, recConfig_->recProfile_->aSampleRate);
        if (ret != 0) {
            return ret;
        }
        ret = recorder_->SetAudioChannels(aSourceID, recConfig_->recProfile_->aChannels);
        if (ret != 0) {
            return ret;
        }
        ret = recorder_->SetAudioEncodingBitRate(aSourceID, recConfig_->recProfile_->aBitRate);
        if (ret != 0) {
            return ret;
        }
    }
    ret = recorder_->SetOutputPath(recConfig_->strVideoPath);
    if (ret != 0) {
        return ret;
    }
    ret = recorder_->Prepare();
    if (ret != 0) {
        HiLog::Error(LABEL, "Prepare Failed");
        return ret;
    }

    return ret;
}

sptr<Surface> CameraNapi::CreateSubWindowSurface()
{
    if (!subWindow_) {
        WindowConfig config = {
            .width = SURFACE_DEFAULT_WIDTH,
            .height = SURFACE_DEFAULT_HEIGHT,
            .pos_x = 0,
            .pos_y = 0,
            .format = PIXEL_FMT_RGBA_8888
        };
        WindowConfig subConfig = config;
        subConfig.type = WINDOW_TYPE_VIDEO;
        subConfig.format = PIXEL_FMT_YCRCB_420_P;
        subConfig.subwindow = true;
        previewWindow = WindowManager::GetInstance()->CreateWindow(&config);
        if (previewWindow == nullptr) {
            HiLog::Error(LABEL, "Preview Window was not created");
            return nullptr;
        }
        sptr<SurfaceBuffer> buffer;
        int32_t releaseFence = -1;
        BufferRequestConfig requestConfig;
        previewWindow->GetRequestConfig(requestConfig);
        SurfaceError error = previewWindow->GetSurface()->RequestBuffer(buffer, releaseFence, requestConfig);
        if (error != SURFACE_ERROR_OK) {
            MEDIA_ERR_LOG("Camera Request Buffer Failed");
            return nullptr;
        }
        uint32_t buffSize = buffer->GetSize();
        void *bufferVirAddr = buffer->GetVirAddr();
        if (bufferVirAddr == nullptr) {
            MEDIA_ERR_LOG("bufferVirAddr is nullptr");
            return nullptr;
        }
        memset_s(bufferVirAddr, buffSize, 0xFF, buffSize);
        BufferFlushConfig flushConfig = {
            .damage = {
                .x = 0,
                .y = 0,
                .w = requestConfig.width,
                .h = requestConfig.height,
            },
            .timestamp = 0,
        };
        error = previewWindow->GetSurface()->FlushBuffer(buffer, -1, flushConfig);
        if (error != SURFACE_ERROR_OK) {
            MEDIA_ERR_LOG("Camera Flush Buffer Failed");
            return nullptr;
        }

        subWindow_ = WindowManager::GetInstance()->CreateSubWindow(previewWindow->GetWindowID(), &subConfig);
        if (subWindow_ == nullptr) {
            HiLog::Error(LABEL, "SubWindow is not proper");
            return nullptr;
        }
        subWindow_->GetSurface()->SetQueueSize(SURFACE_QUEUE_SIZE);
    }
    previewSurface_ = subWindow_->GetSurface();
    if (previewSurface_ == nullptr) {
        HiLog::Error(LABEL, "Preview Surface is not proper");
        return nullptr;
    }
    return previewSurface_;
}

int32_t CameraNapi::PreparePhoto(sptr<CameraManager> camManagerObj)
{
    int32_t intResult = 0;
    captureConsumerSurface_ = Surface::CreateSurfaceAsConsumer();
    if (captureConsumerSurface_ == nullptr) {
        HiLog::Error(LABEL, "CreateSurface failed");
        return -1;
    }
    captureConsumerSurface_->SetDefaultWidthAndHeight(PHOTO_DEFAULT_WIDTH, PHOTO_DEFAULT_HEIGHT);
    listener = new SurfaceListener();
    if (listener == nullptr) {
        HiLog::Error(LABEL, "Create Listener failed");
        return -1;
    }
    listener->SetConsumerSurface(captureConsumerSurface_);
    captureConsumerSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);

    photoOutput_ = camManagerObj->CreatePhotoOutput(captureConsumerSurface_);
    if (photoOutput_ == nullptr) {
        HiLog::Error(LABEL, "Failed to create PhotoOutput");
        return -1;
    }
    intResult = capSession_->AddOutput(photoOutput_);
    if (intResult != 0) {
        HiLog::Error(LABEL, "Failed to Add output to session");
        return -1;
    }
    return 0;
}

int32_t CameraNapi::PrepareCommon(napi_env env, int32_t iPrepareType)
{
    int32_t intResult = 0;
    sptr<OHOS::CameraStandard::CameraManager> camManagerObj = OHOS::CameraStandard::CameraManager::GetInstance();
    if (camManagerObj == nullptr) {
        HiLog::Error(LABEL, "Failed to get Camera Manager!");
        return -1;
    }
    if (camInput_ != nullptr) {
        ((sptr<CameraInput> &)camInput_)->UnlockForControl();
    }
    capSession_ = camManagerObj->CreateCaptureSession();
    if ((capSession_ == nullptr) || (camInput_ == nullptr)) {
        HiLog::Error(LABEL, "Create was not Proper!");
        return -1;
    }
    capSession_->BeginConfig();
    intResult = capSession_->AddInput(camInput_);
    if (intResult != 0) {
        HiLog::Error(LABEL, "AddInput Failed!");
        return -1;
    }
    previewOutput_ = camManagerObj->CreatePreviewOutput(CreateSubWindowSurface());
    if (previewOutput_ == nullptr) {
        HiLog::Error(LABEL, "Failed to create PreviewOutput");
        return -1;
    }
    intResult = capSession_->AddOutput(previewOutput_);
    if (intResult != 0) {
        HiLog::Error(LABEL, "Failed to Add Preview Output");
        return -1;
    }
    intResult = PreparePhoto(camManagerObj);
    if (intResult != 0) {
        HiLog::Error(LABEL, "Failed to Prepare Photo");
        return -1;
    }
    // Needs to be integrated with Recorder
    intResult = capSession_->CommitConfig();
    if (intResult != 0) {
        HiLog::Error(LABEL, "Failed to Commit config");
        return -1;
    }
    isReady_ = true;
    return 0;
}

napi_value CameraNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_status status = napi_ok;
    int32_t iPrepareType = 0;
    napi_value undefinedResult = nullptr;
    CameraNapi *CameraWrapper = nullptr;
    int32_t intResult = -1;
    napi_value jsCallback, result;

    GET_PARAMS(env, info, ARGS_THREE);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_THREE) {
        HiLog::Error(LABEL, "CameraNapi::Prepare() Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, (void**)&CameraWrapper);
    if (status == napi_ok && CameraWrapper != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &iPrepareType);
            } else if (i == 1 && valueType == napi_object) {
                // Needs to be integrated with Recorder
            }
        }
        CameraWrapper->PrepareCommon(env, iPrepareType);

        if (CameraWrapper->prepareCallback_ == nullptr) {
            HiLog::Error(LABEL, "Prepare Callback is not registered!");
            return undefinedResult;
        }
        status = napi_get_reference_value(env, CameraWrapper->prepareCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
    }

    return undefinedResult;
}

static void GetCameraIdsAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto asyncContext = static_cast<CameraNapiAsyncContext*>(data);
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    size_t size = asyncContext->cameraObjList.size();
    napi_create_array_with_length(env, size, &result[1]);

    for (unsigned int i = 0; i < size; i += 1) {
        std::string strCurCamID = asyncContext->cameraObjList[i]->GetID();
        napi_value value;
        status = napi_create_string_utf8(env, strCurCamID.c_str(),
                                         NAPI_AUTO_LENGTH, &value);
        if (status != napi_ok) {
            continue;
        }
        napi_set_element(env, result[1], i, value);
    }

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetCameraIDs(napi_env env, napi_callback_info info)
{
    napi_status status = napi_ok;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);

    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    if (status == napi_ok) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], REFERENCE_CREATION_COUNT, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetCameraIDs", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = (CameraNapiAsyncContext*)data;
                sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
                context->cameraObjList = camManagerObj->GetCameras();
                context->status = 0;
            },
            GetCameraIdsAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

int32_t CameraNapi::InitCamera(std::string CameraID)
{
    int32_t intResult = -1;
    CameraManager *camManagerObj = CameraManager::GetInstance();
    if (camManagerObj == nullptr) {
        HiLog::Error(LABEL, "Failed to get Camera Manager");
        return -1;
    }
    std::vector<sptr<OHOS::CameraStandard::CameraInfo>> cameraObjList = camManagerObj->GetCameras();

    if (cameraObjList.size() > 0) {
        camInput_ = camManagerObj->CreateCameraInput(cameraObjList[0]);
        if (camInput_ == nullptr) {
            HiLog::Error(LABEL, "CameraNapi::InitCamera() CreateCameraInput() Failed!");
            return -1;
        }
    }
    // Needs to be integrated with Recorder
    if (camInput_ != nullptr) {
        ((sptr<CameraInput> &)camInput_)->LockForControl();
    }
    return 0;
}

napi_value CameraNapi::StartVideoRecording(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    CameraNapi* camWrapper = nullptr;
    napi_value jsCallback, result;

    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**)&camWrapper);
    if (status == napi_ok) {
        if (camWrapper->recordState_ == State::STATE_RUNNING) {
            HiLog::Error(LABEL, "Camera is already recording.");
            return undefinedResult;
        }
        int intResult = camWrapper->recorder_->Start();
        if (intResult != ERR_OK) {
            HiLog::Error(LABEL, "Failed to Start Recording");
            camWrapper->CloseRecorder();
            return undefinedResult;
        }
        if (camWrapper->videoOutput_ == nullptr) {
            HiLog::Error(LABEL, "Video Output is null");
            camWrapper->CloseRecorder();
            return undefinedResult;
        }
        ((sptr<OHOS::CameraStandard::VideoOutput> &)(camWrapper->videoOutput_))->Start();
        camWrapper->recordState_ = State::STATE_RUNNING;

        if (camWrapper->startCallback_ == nullptr) {
            HiLog::Error(LABEL, "Start Callback is not registered!");
            return undefinedResult;
        }
        status = napi_get_reference_value(env, camWrapper->startCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to Start Recording");
    return undefinedResult;
}

napi_value CameraNapi::StopVideoRecording(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    CameraNapi* camWrapper = nullptr;
    napi_value jsCallback, result;

    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**)&camWrapper);
    if (status == napi_ok) {
        if (camWrapper->recordState_ != State::STATE_RUNNING) {
            HiLog::Error(LABEL, "Failed to Stop Recording");
            return undefinedResult;
        }
        if (camWrapper->videoOutput_ != nullptr) {
            ((sptr<OHOS::CameraStandard::VideoOutput> &)(camWrapper->videoOutput_))->Stop();
            ((sptr<OHOS::CameraStandard::VideoOutput> &)(camWrapper->videoOutput_))->Release();
        }
        if (camWrapper->recorder_->Stop(false) != 0) {
            HiLog::Error(LABEL, "Failed to Stop Recording");
            return undefinedResult;
        }
        camWrapper->recordState_ = State::STATE_IDLE;
        if (camWrapper->stopCallback_ == nullptr) {
            HiLog::Error(LABEL, "Stop Callback is not registered!");
            return undefinedResult;
        }

        status = napi_get_reference_value(env, camWrapper->stopCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to Stop Recording");
    return undefinedResult;
}

napi_value CameraNapi::ResetVideoRecording(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    CameraNapi* camWrapper = nullptr;
    napi_value jsCallback, result;

    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**)&camWrapper);
    if (status == napi_ok) {
        if (camWrapper->recorder_->Reset() != 0) {
            HiLog::Error(LABEL, "Failed to Reset Recording");
            return undefinedResult;
        }
        if (camWrapper->resetCallback_ == nullptr) {
            HiLog::Error(LABEL, "Reset Callback is not registered!");
            return undefinedResult;
        }

        status = napi_get_reference_value(env, camWrapper->resetCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to Reset Recording");
    return undefinedResult;
}

napi_value CameraNapi::PauseVideoRecording(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    CameraNapi* camWrapper = nullptr;
    napi_value jsCallback, result;

    napi_get_undefined(env, &undefinedResult);
    HiLog::Debug(LABEL, "PauseVideoRecording() is called!");
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**)&camWrapper);
    if (status == napi_ok) {
        if (camWrapper->recorder_->Pause() != 0) {
            HiLog::Error(LABEL, "Failed to Pause Recording");
            return undefinedResult;
        }
        if (camWrapper->pauseCallback_ == nullptr) {
            HiLog::Error(LABEL, "Pause Callback is not registered!");
            return undefinedResult;
        }

        status = napi_get_reference_value(env, camWrapper->pauseCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to Pause Recording");
    return undefinedResult;
}

napi_value CameraNapi::ResumeVideoRecording(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    CameraNapi* camWrapper = nullptr;
    napi_value jsCallback, result;

    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**)&camWrapper);
    if (status == napi_ok) {
        if (camWrapper->recorder_->Resume() != 0) {
            HiLog::Error(LABEL, "Failed to Resume Recording");
            return undefinedResult;
        }
        if (camWrapper->resumeCallback_ == nullptr) {
            HiLog::Error(LABEL, "Resume Callback is not registered!");
            return undefinedResult;
        }
        status = napi_get_reference_value(env, camWrapper->resumeCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to Resume Recording");
    return undefinedResult;
}

napi_value CameraNapi::StartPreview(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    CameraNapi* cameraWrapper = nullptr;
    int32_t intResult = -1;
    napi_value jsCallback, result;
    napi_get_undefined(env, &undefinedResult);

    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**)&cameraWrapper);
    if (status == napi_ok) {
        if (!(cameraWrapper->isReady_) || (cameraWrapper->capSession_ == nullptr)) {
            HiLog::Error(LABEL, "Not ready for StartPreview.");
            cameraWrapper->hasCallPreview_ = true;
            return undefinedResult;
        }

        if (cameraWrapper->previewState_ == State::STATE_RUNNING) {
            HiLog::Error(LABEL, "Camera is already previewing.");
            return undefinedResult;
        }

        intResult = cameraWrapper->capSession_->Start();
        if (intResult != 0) {
            HiLog::Error(LABEL, "Camera::Start Capture Session Failed");
            return undefinedResult;
        }
        cameraWrapper->previewState_ = State::STATE_RUNNING;
        cameraWrapper->hasCallPreview_ = false;
        if (cameraWrapper->startPreviewCallback_ == nullptr) {
            HiLog::Error(LABEL, "Start Preview Callback is not registered!");
            return undefinedResult;
        }
        status = napi_get_reference_value(env, cameraWrapper->startPreviewCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
        return undefinedResult;
    }
    HiLog::Error(LABEL, "CameraNapi::Failed to Start Preview");

    return undefinedResult;
}

napi_value CameraNapi::StopPreview(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    CameraNapi* cameraWrapper = nullptr;
    napi_value jsCallback, result;

    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, jsThis, (void**)&cameraWrapper);
    if (status == napi_ok) {
        if (cameraWrapper->capSession_ != nullptr) {
            cameraWrapper->capSession_->Stop();
            cameraWrapper->capSession_->Release();
        }
        if (cameraWrapper->subWindow_ != nullptr) {
            cameraWrapper->subWindow_.reset();
        }
        if (cameraWrapper->previewWindow != nullptr) {
            cameraWrapper->previewWindow.reset();
        }
        cameraWrapper->previewState_ = State::STATE_IDLE;
        if (cameraWrapper->stopPreviewCallback_ == nullptr) {
            HiLog::Error(LABEL, "Stop Preview Callback is not registered!");
            return undefinedResult;
        }
        status = napi_get_reference_value(env, cameraWrapper->stopPreviewCallback_, &jsCallback);
        if (status == napi_ok && jsCallback != nullptr) {
            if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                return undefinedResult;
            }
        }
        return undefinedResult;
    }
    HiLog::Error(LABEL, "CameraNapi::Failed to Stop Preview");

    return undefinedResult;
}

int32_t CameraNapi::GetPhotoConfig(napi_env env, napi_value arg)
{
    size_t res = 0;
    napi_value temp, mirtemp;
    bool bIsMirror = false;
    bool bIsPresent = false;
    napi_has_named_property(env, arg, "photoPath", &bIsPresent);
    if (!bIsPresent) {
        HiLog::Error(LABEL, "No Photo Path Exists");
        return -1;
    }
    if (photoConfig_) {
        photoConfig_.reset();
    }
    photoConfig_ = std::make_unique<PhotoConfig>();
    if (photoConfig_ == nullptr) {
        HiLog::Error(LABEL, "No Photo Config Exists");
        return -1;
    }
    bIsPresent = false;
    if (napi_get_named_property(env, arg, "photoPath", &temp) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the photoPath argument!");
        return -1;
    }
    photoConfig_->strPhotoPath = GetStringArgument(env, temp);
    if (photoConfig_->strPhotoPath.empty()) {
        photoConfig_->strPhotoPath = "/data/";
    }

    if (listener != nullptr) {
        listener->SetPhotoPath(photoConfig_->strPhotoPath);
    }
    napi_has_named_property(env, arg, "mirror", &bIsPresent);
    if (!bIsPresent) {
        HiLog::Error(LABEL, "No mirror flag Exists");
        return 0;
    }
    if (napi_get_named_property(env, arg, "mirror", &mirtemp) != napi_ok
        || napi_get_value_bool(env, mirtemp, &bIsMirror) != napi_ok) {
        HiLog::Error(LABEL, "Could not get the IsMirror argument!");
        return -1;
    } else {
        photoConfig_->bIsMirror = bIsMirror;
        bIsMirror = false;
    }
    return 0;
}

static void TakePhotoAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto asyncContext = static_cast<CameraNapiAsyncContext*>(data);
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::TakePhoto(napi_env env, napi_callback_info info)
{
    napi_status status = napi_ok;
    napi_value result = nullptr;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();

    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &result);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return result;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_object) {
                (asyncContext->objectInfo)->GetPhotoConfig(env, argv[i]);
            } else if (i == 1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], 1, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }
        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }
        napi_value resource = nullptr;
        napi_create_string_utf8(env, "TakePhoto", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                if (context->objectInfo->photoOutput_ == nullptr) {
                    HiLog::Error(LABEL, "Context Photo Output is null!");
                } else if (context->objectInfo->isReady_ != true) {
                    HiLog::Error(LABEL, "Camera not ready for Taking Photo!");
                } else {
                    if (context->objectInfo->recordState_ == State::STATE_RUNNING) {
                        HiLog::Error(LABEL, "Camera not ready for Taking Photo!");
                        context->objectInfo->CloseRecorder();
                    }
                    context->status = ((sptr<PhotoOutput> &)(context->objectInfo->photoOutput_))->Capture();
                    context->objectInfo->SetPhotoCallFlag();
                }
            },
            TakePhotoAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void CameraNapi::CloseRecorder()
{
    if (recordState_ == State::STATE_RUNNING) {
        if (recorder_ != nullptr) {
            recorder_->Stop(true);
            recorder_ = nullptr;
        }
        recordState_ = State::STATE_IDLE;
    }
}

void CameraNapi::SetPhotoCallFlag()
{
    hasCallPhoto_ = true;
}

napi_value CameraNapi::CreateCameraWrapper(napi_env env, std::string strCamID)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;
    napi_value args[1];

    status = napi_create_string_utf8(env, strCamID.c_str(), NAPI_AUTO_LENGTH, &args[0]);
    if (status != napi_ok) {
        return result;
    }
    status = napi_get_reference_value(env, sCtor_, &ctor);
    if (status == napi_ok) {
        status = napi_new_instance(env, ctor, 1, args, &result);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed in CameraNapi::CreateCameraWrapper()!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateCamera(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 1;
    napi_value args[1];

    status = napi_get_cb_info(env, info, &argCount, args, nullptr, nullptr);
    if (status != napi_ok || argCount != 1) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }
    std::string strCamID = GetStringArgument(env, args[0]);  // Init Camera ID

    return CameraNapi::CreateCameraWrapper(env, strCamID);
}

napi_value CameraNapi::CreateCameraPositionObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::UNSPECIFIED; i <= vecCameraPosition.size(); i++) {
            propName = vecCameraPosition[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        // The reference count is for creating Camera Position Object Reference
        status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &cameraPositionRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "CreateCameraPositionObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateCameraTypeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::CAMERATYPE_UNSPECIFIED; i <= vecCameraType.size(); i++) {
            propName = vecCameraType[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            // The reference count is for creating Camera Type object reference
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &cameraTypeRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateCameraTypeObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateExposureModeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::EXPOSUREMODE_MANUAL; i <= vecExposureMode.size(); i++) {
            propName = vecExposureMode[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            // The reference count is for creating Camera Position Object reference
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &exposureModeRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateExposureModeObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateFocusModeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::FOCUSMODE_MANUAL; i <= vecFocusMode.size(); i++) {
            propName = vecFocusMode[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
            // The reference count is for creation of Focus Mode Object Reference
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &focusModeRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    HiLog::Error(LABEL, "CreateFocusModeObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateFlashModeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::FLASHMODE_CLOSE; i <= vecFlashMode.size(); i++) {
            propName = vecFlashMode[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        // The reference count is for creation of Flash Mode Object Reference
        status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &flashModeRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "CreateFlashModeObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateResolutionScaleObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::WIDTH_HEIGHT_4_3; i <= vecResolutionScale.size(); i++) {
            propName = vecResolutionScale[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        // The reference count is for creation of Resolution Scale Object reference.
        status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &resolutionScaleRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "CreateResolutionScaleObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateQualityObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::QUALITY_HIGH; i <= vecQuality.size(); i++) {
            propName = vecQuality[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            // The reference count is for creation of Quality Object reference
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &qualityRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateQualityObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateQualityLevelObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::QUALITY_LEVEL_1080P; i <= vecQualityLevel.size(); i++) {
            propName = vecQualityLevel[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        // The reference count is for creation of Quality Level Object reference
        status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &qualityLevelRef_);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "CreateQualityLevelObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateFileFormatObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::FORMAT_DEFAULT; i <= vecFileFormat.size(); i++) {
            propName = vecFileFormat[i];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            // The reference count is for creation of File Format Object
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &fileFormatRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateFileFormatObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateVideoEncoderObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::H264; i <= vecVideoEncoder.size(); i++) {
            propName = vecVideoEncoder[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            // The reference count is for creation of Video Encoder Object reference
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &videoEncoderRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateVideoEncoderObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateAudioEncoderObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::AAC_LC; i <= vecAudioEncoder.size(); i++) {
            propName = vecAudioEncoder[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            // The reference count is for creation of Audio Encoder Object reference
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &audioEncoderRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateAudioEncoderObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

napi_value CameraNapi::CreateParameterResultObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (unsigned int i = CameraNapi::ERROR_UNKNOWN; i <= vecParameterResult.size(); i++) {
            propName = vecParameterResult[i - 1];
            status = AddNamedProperty(env, result, propName, i);
            if (status != napi_ok) {
                HiLog::Error(LABEL, "Failed to add named prop!");
                break;
            }
            propName.clear();
        }

        if (status == napi_ok) {
            // The reference count is for Creation of Create Parameter Object Reference
            status = napi_create_reference(env, result, REFERENCE_CREATION_COUNT, &parameterResultRef_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    HiLog::Error(LABEL, "CreateParameterResultObject is Failed!");
    napi_get_undefined(env, &result);

    return result;
}

static void GetSupportedPropertiesAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);

    size_t size = asyncContext->vecSupportedPropertyList.size();
    napi_create_array_with_length(env, size, &result[1]);
    for (unsigned int i = 0; i < size; i += 1) {
        std::string strProp = asyncContext->vecSupportedPropertyList[i];
        napi_value value;
        status = napi_create_string_utf8(env, strProp.c_str(),
                                         NAPI_AUTO_LENGTH, &value);
        if (status != napi_ok) {
            continue;
        }
        napi_set_element(env, result[1], i, value);
    }

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetSupportedProperties(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "getSupportedProperties", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to get supported Exposure Mode
                context->status = 0;
            },
            GetSupportedPropertiesAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetPropertyValueAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetPropertyValue(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);

    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetPropertyValue", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to get supported Exposure Mode
                context->status = 0;
            },
            GetPropertyValueAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetSupportedResolutionScalesAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    size_t size = asyncContext->vecSupportedResolutionScalesList.size();
    napi_create_array_with_length(env, size, &result[1]);

    for (unsigned int i = 0; i < size; i += 1) {
        int32_t  iProp = asyncContext->vecSupportedResolutionScalesList[i];
        napi_value value;
        status = napi_create_int32(env, iProp, &value);
        if (status != napi_ok) {
            continue;
        }
        napi_set_element(env, result[1], i, value);
    }

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetSupportedResolutionScales(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);

    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetSupportedResolutionScales", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to get supported Resolution Scales
                context->status = 0;
            },
            GetSupportedResolutionScalesAsyncCallbackComplete,
            static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void SetPreviewResolutionScaleAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::SetPreviewResolutionScale(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->iResScale);
            } else if (i == 1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "SetPreviewResolutionScale", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to set Preview Resolution Scale
                context->status = 0;
            },
            SetPreviewResolutionScaleAsyncCallbackComplete,
            static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void SetPreviewQualityAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::SetPreviewQuality(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->iQuality);
            } else if (i == 1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "setPreviewQuality", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to get supported Exposure Mode
                context->status = 0;
            },
            SetPreviewQualityAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetSupportedExposureModeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    size_t size = asyncContext->vecSupportedExposureModeList.size();
    napi_create_array_with_length(env, size, &result[1]);
    for (unsigned int i = 0; i < size; i += 1) {
        int32_t  iProp = asyncContext->vecSupportedExposureModeList[i];
        napi_value value;
        status = napi_create_int32(env, iProp, &value);
        if (status != napi_ok) {
            continue;
        }
        napi_set_element(env, result[1], i, value);
    }

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetSupportedExposureMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetSupportedExposureMode", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                context->vecSupportedExposureModeList = ((sptr<CameraInput> &)
                                                         (context->objectInfo->camInput_))->GetSupportedExposureModes();
                context->status = 0;
            },
            GetSupportedExposureModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }
    return result;
}

static void SetExposureModeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::SetExposureMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->iExposureMode);
            } else if (i == 1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "SetExposureMode", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                ((sptr<CameraInput> &)
                 (context->objectInfo->camInput_))->SetExposureMode(static_cast<camera_exposure_mode_enum_t>
               (context->iExposureMode));
                context->status = 0;
            },
            SetExposureModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetSupportedFocusModeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    size_t size = asyncContext->vecSupportedFocusModeList.size();
    napi_create_array_with_length(env, size, &result[1]);
    for (unsigned int i = 0; i < size; i += 1) {
        int32_t  iProp = asyncContext->vecSupportedFocusModeList[i];
        napi_value value;
        status = napi_create_int32(env, iProp, &value);
        if (status != napi_ok) {
            continue;
        }
        napi_set_element(env, result[1], i, value);
    }

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetSupportedFocusMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetSupportedFocusMode", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                context->vecSupportedFocusModeList = ((sptr<CameraInput> &)
                                                      (context->objectInfo->camInput_))->GetSupportedFocusModes();
                context->status = 0;
            },
            GetSupportedFocusModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void SetFocusModeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::SetFocusMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);

    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->iFocusMode);
            } else if (i == 1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "SetFocusMode", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                ((sptr<CameraInput> &)
                 (context->objectInfo->camInput_)
                 )->SetFocusMode(static_cast<camera_focus_mode_enum_t>(context->iFocusMode));
                context->status = 0;
            },
            SetFocusModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetSupportedFlashModeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);

    size_t size = asyncContext->vecSupportedFlashModeList.size();
    napi_create_array_with_length(env, size, &result[1]);
    for (unsigned int i = 0; i < size; i += 1) {
        int32_t  iProp = asyncContext->vecSupportedFlashModeList[i];
        napi_value value;
        status = napi_create_int32(env, iProp, &value);
        if (status != napi_ok) {
            continue;
        }
        napi_set_element(env, result[1], i, value);
    }

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetSupportedFlashMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);

    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "GetSupportedFlashMode", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to get supported Exposure Mode
                context->vecSupportedFlashModeList = ((sptr<CameraInput> &)
                                                      (context->objectInfo->camInput_))->GetSupportedFlashModes();
                context->status = 0;
            },
            GetSupportedFlashModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void SetFlashModeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::SetFlashMode(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->iFlashMode);
            } else if (i == 1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "SetFlashMode", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                ((sptr<CameraInput> &)
                 (context->objectInfo->camInput_)
                 )->SetFlashMode(static_cast<camera_flash_mode_enum_t>(context->iFlashMode));
                context->status = 0;
            },
            SetFlashModeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void GetSupportedZoomRangeAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    size_t size = asyncContext->vecSupportedZoomRangeList.size();
    napi_create_array_with_length(env, size, &result[1]);

    for (unsigned int i = 0; i < size; i += 1) {
        double  dProp = static_cast<double>(asyncContext->vecSupportedZoomRangeList[i]);
        napi_value value;
        status = napi_create_double(env, dProp, &value);
        if (status != napi_ok) {
            continue;
        }
        napi_set_element(env, result[1], i, value);
    }

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::GetSupportedZoomRange(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);

    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "getSupportedZoomRange", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to get supported Exposure Mode
                context->vecSupportedZoomRangeList = ((sptr<CameraInput> &)
                                                      (context->objectInfo->camInput_))->GetSupportedZoomRatioRange();
                context->status = 0;
            },
            GetSupportedZoomRangeAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static void SetZoomAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
}

napi_value CameraNapi::SetZoom(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;

    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_MAX_TWO_COUNT) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0 && valueType == napi_number) {
                napi_get_value_double(env, argv[i], &asyncContext->dZoomRatio);
            } else if (i == 1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "SetZoom", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*>(data);
                ((sptr<CameraInput> &)
                 (context->objectInfo->camInput_))->SetZoomRatio(static_cast<float>(context->dZoomRatio));
                context->status = 0;
            },
            SetZoomAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }
    return result;
}

static void SetParameterAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    CameraNapiAsyncContext* asyncContext = (CameraNapiAsyncContext*)data;
    napi_value result[ARGS_TWO] = {0};
    napi_value retVal;
    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    if (asyncContext->deferred) {
        napi_resolve_deferred(env, asyncContext->deferred, result[1]);
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
}

napi_value CameraNapi::SetParameter(napi_env env, napi_callback_info info)
{
    napi_status status;
    std::unique_ptr<CameraNapiAsyncContext> asyncContext = std::make_unique<CameraNapiAsyncContext>();
    const int32_t refCount = 1;
    napi_value result = nullptr;
    napi_value undefinedResult = nullptr;
    GET_PARAMS(env, info, ARGS_TWO);
    napi_get_undefined(env, &undefinedResult);
    if (argc > ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == 0) {
                continue;
            } else if (i == 1) {
                continue;
            } else if (i == ARGS_TWO && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            napi_get_undefined(env, &result);
        }

        napi_value resource = nullptr;
        napi_create_string_utf8(env, "SetParameter", NAPI_AUTO_LENGTH, &resource);

        status = napi_create_async_work(
            env, nullptr, resource,
            [](napi_env env, void* data) {
                CameraNapiAsyncContext* context = static_cast<CameraNapiAsyncContext*> (data);
                // Need to add logic for calling native to set Parameters
                context->status = 0;
            },
            SetParameterAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            result = nullptr;
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value CameraNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis = nullptr;
    napi_value  result = nullptr;
    size_t argCount = 1;
    napi_value args[1] = {0};
    int32_t intResult = 0;

    napi_get_undefined(env, &result);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status == napi_ok) {
        std::string CamID = GetStringArgument(env, args[0]);
        std::unique_ptr<CameraNapi> obj = std::make_unique<CameraNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            intResult = obj->InitCamera(CamID);
            if (intResult != 0) {
                HiLog::Error(LABEL, "Init Camera Failed");
                return result;
            }
            // Object of the Camera Manager needs to be assigned.
            status = napi_wrap(env, jsThis, reinterpret_cast<void*>(obj.get()),
                               CameraNapi::CameraNapiDestructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                // Needs to be integrated with Recorder
                obj.release();
                return jsThis;
            }
        }
    }
    HiLog::Error(LABEL, "Failed in CameraNapi::Construct()!");

    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    // Need to add the init logic here
    CameraNapi::Init(env, exports);
    return exports;
}

static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "multimedia.camera_napi",
    .nm_priv = ((void *)0),
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HiLog::Debug(LABEL, "RegisterModule() is called!");
    napi_module_register(&g_module);
}
} // namespace CameraStandard
} // namespace OHOS
