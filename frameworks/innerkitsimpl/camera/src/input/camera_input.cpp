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

#include "input/camera_input.h"
#include "camera_device_ability_items.h"
#include "camera_util.h"
#include "hcamera_device_callback_stub.h"
#include "media_log.h"

namespace OHOS {
namespace CameraStandard {
class CameraDeviceServiceCallback : public HCameraDeviceCallbackStub {
public:
    sptr<CameraInput> camInput_ = nullptr;
    CameraDeviceServiceCallback() : camInput_(nullptr) {
    }

    explicit CameraDeviceServiceCallback(const sptr<CameraInput>& camInput) : camInput_(camInput) {
    }

    ~CameraDeviceServiceCallback()
    {
        camInput_ = nullptr;
    }

    int32_t OnError(const int32_t errorType, const int32_t errorMsg) override
    {
        MEDIA_ERR_LOG("CameraDeviceServiceCallback::OnError() is called!, errorType: %{public}d, errorMsg: %{public}d",
                      errorType, errorMsg);
        if (camInput_ != nullptr && camInput_->GetErrorCallback() != nullptr) {
            camInput_->GetErrorCallback()->OnError(errorType, errorMsg);
        } else {
            MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() is called!, Discarding callback");
        }
        return CAMERA_OK;
    }

    int32_t OnResult(const uint64_t timestamp, const std::shared_ptr<CameraMetadata> &result) override
    {
        MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() is called!, timestamp: %{public}llu", timestamp);
        return CAMERA_OK;
    }
};

CameraInput::CameraInput(sptr<ICameraDeviceService> &deviceObj,
                         sptr<CameraInfo> &cameraObj) : cameraObj_(cameraObj), deviceObj_(deviceObj)
{
    CameraDeviceSvcCallback_ = new CameraDeviceServiceCallback(this);
    deviceObj_->SetCallback(CameraDeviceSvcCallback_);
}

void CameraInput::Release()
{
    int32_t retCode = deviceObj_->Release();
    if (retCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release Camera Input, retCode: %{public}d", retCode);
    }
    return;
}

void CameraInput::LockForControl()
{
    configBatch_.clear();
    isStartConfig_ = true;
    return;
}

int32_t CameraInput::UnlockForControl()
{
    if (!configBatch_.size()) {
        isStartConfig_ = false;
        return CAMERA_OK;
    }

    int length = [&]() {
        int totalLength = 0;
        for (auto& it: configBatch_) {
            if (it.second.get()->length_ > 4) {
                totalLength += it.second.get()->length_;
            }
        }
        return totalLength;
    }();
    std::shared_ptr<CameraMetadata> changedMetadata = std::make_shared<CameraMetadata>(configBatch_.size(), length);

    /* TODO: Currently we don't realloc metadata if items and data capacity with new change doesn't
    fit in metadata buffer */
    std::shared_ptr<CameraMetadata> baseMetadata = cameraObj_->GetMetadata();

    for (auto& it: configBatch_) {
        if (changedMetadata->addEntry(it.first, it.second.get()->data_, it.second.get()->count_)) {
            camera_metadata_item_t item;
            int ret = find_camera_metadata_item(baseMetadata->get(), it.first, &item);
            if (ret == CAM_META_SUCCESS) {
                baseMetadata->updateEntry(it.first, it.second.get()->data_, it.second.get()->count_);
            } else if (ret == CAM_META_ITEM_NOT_FOUND) {
                baseMetadata->addEntry(it.first, it.second.get()->data_, it.second.get()->count_);
            } else {
                MEDIA_ERR_LOG("CameraInput::UnlockForControl find_camera_metadata_item %{public}d", ret);
            }
        }
    }
    isStartConfig_ = false;
    configBatch_.clear();
    deviceObj_->UpdateSetting(changedMetadata);
    cameraObj_->SetMetadata(baseMetadata);
    return CAMERA_OK;
}

ChangeMetadata::~ChangeMetadata()
{
    free(data_);
}

template <typename DataPtr, typename Vec, typename VecType>
void CameraInput::getVector(DataPtr data, size_t count, Vec &vect, VecType dataType)
{
    for (size_t index = 0; index < count; index++) {
        vect.emplace_back(static_cast<VecType>(data[index]));
    }
}

std::vector<CameraInput::PhotoFormat> CameraInput::GetSupportedPhotoFormats()
{
    std::vector<CameraInput::PhotoFormat> formatList;
    formatList.emplace_back(PhotoFormat::JPEG_FORMAT);
    return formatList;
}

std::vector<CameraInput::VideoFormat> CameraInput::GetSupportedVideoFormats()
{
    std::vector<CameraInput::VideoFormat> formatList;
    formatList.emplace_back(VideoFormat::YUV_FORMAT);
    formatList.emplace_back(VideoFormat::H264_FORMAT);
    formatList.emplace_back(VideoFormat::H265_FORMAT);
    return formatList;
}

bool CameraInput::IsPhotoFormatSupported(CameraInput::PhotoFormat photoFormat)
{
    bool result = false;

    switch (photoFormat)
    {
        case PhotoFormat::JPEG_FORMAT:
            result = true;
            break;

        default :
            break;
    }
    return result;
}

bool CameraInput::IsVideoFormatSupported(CameraInput::VideoFormat videoFormat)
{
    bool result = false;

    switch (videoFormat)
    {
        case VideoFormat::YUV_FORMAT:
        case VideoFormat::H264_FORMAT:
        case VideoFormat::H265_FORMAT:
            result = true;
            break;

        default :
            break;
    }
    return result;
}

std::vector<CameraPicSize *> CameraInput::GetSupportedSizesForPhoto(CameraInput::PhotoFormat photoFormat)
{
    std::vector<CameraPicSize *> result = {};
    CameraPicSize *cameraPicSize = (CameraPicSize *) malloc (sizeof(CameraPicSize));
    if (cameraPicSize != nullptr) {
        cameraPicSize->height = 720;
        cameraPicSize->width = 1280;
        result.emplace_back(cameraPicSize);
    }
    return result;
}

std::vector<CameraPicSize *> CameraInput::GetSupportedSizesForVideo(CameraInput::VideoFormat videoFormat)
{
    std::vector<CameraPicSize *> result = {};
    CameraPicSize *cameraPicSize = (CameraPicSize *) malloc (sizeof(CameraPicSize));
    if (cameraPicSize != nullptr) {
        cameraPicSize->height = 720;
        cameraPicSize->width = 1280;
        result.emplace_back(cameraPicSize);
    }
    return result;
}

std::vector<camera_exposure_mode_enum_t> CameraInput::GetSupportedExposureModes()
{
    std::vector<camera_exposure_mode_enum_t> supportedExposureModes;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_ABILITY_DEVICE_AVAILABLE_EXPOSUREMODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedExposureModes Failed with return code %{public}d", ret);
        return supportedExposureModes;
    }
    getVector(item.data.u8, item.count, supportedExposureModes, camera_exposure_mode_enum_t(0));
    return supportedExposureModes;
}

void CameraInput::SetExposureMode(camera_exposure_mode_enum_t exposureMode)
{
    /* TODO: What should we do when set is called without LockForControl(). Need to check for
     other metadata as well */
    int len = sizeof(uint8_t);
    uint8_t *exposure = (uint8_t *) malloc(len);
    if (exposure == NULL) {
        MEDIA_ERR_LOG("CameraInput::SetExposureMode Memory allocation failed");
        return;
    }
    *exposure = exposureMode;
    configBatch_[OHOS_CONTROL_EXPOSUREMODE] = std::make_unique<ChangeMetadata>(exposure, len, 1);
    return;
}

camera_exposure_mode_enum_t CameraInput::GetExposureMode()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_CONTROL_EXPOSUREMODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetExposureMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_EXPOSURE_MODE_MANUAL;
    }
    return static_cast<camera_exposure_mode_enum_t>(item.data.u8[0]);
}

void CameraInput::SetExposureCallback(std::shared_ptr<ExposureCallback> exposureCallback)
{
    exposurecallback_ = exposureCallback;
    return;
}

std::vector<camera_focus_mode_enum_t> CameraInput::GetSupportedFocusModes()
{
    std::vector<camera_focus_mode_enum_t> supportedFocusModes;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_ABILITY_DEVICE_AVAILABLE_FOCUSMODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedFocusModes Failed with return code %{public}d", ret);
        return supportedFocusModes;
    }
    getVector(item.data.u8, item.count, supportedFocusModes, camera_focus_mode_enum_t(0));
    return supportedFocusModes;
}

void CameraInput::SetFocusCallback(std::shared_ptr<FocusCallback> focusCallback)
{
    focusCallback_ = focusCallback;
    return;
}

void CameraInput::SetFocusMode(camera_focus_mode_enum_t focusMode)
{
    int len = sizeof(uint8_t);
    uint8_t *focus = (uint8_t *) malloc(len);
    if (focus == NULL) {
        MEDIA_ERR_LOG("CameraInput::SetFocusMode Memory allocation failed");
        return;
    }
    *focus = focusMode;
    configBatch_[OHOS_CONTROL_FOCUSMODE] = std::make_unique<ChangeMetadata>(focus, len, 1);
    return;
}

camera_focus_mode_enum_t CameraInput::GetFocusMode()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_CONTROL_FOCUSMODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFocusMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_FOCUS_MODE_MANUAL;
    }
    return static_cast<camera_focus_mode_enum_t>(item.data.u8[0]);
}

std::vector<float> CameraInput::GetSupportedZoomRatioRange()
{
    std::vector<float> zoomRatioRange;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_ABILITY_ZOOM_RATIO_RANGE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedZoomRatioRange Failed with return code %{public}d", ret);
        return zoomRatioRange;
    }
    getVector(item.data.f, item.count, zoomRatioRange, float(0));
    return zoomRatioRange;
}

float CameraInput::GetZoomRatio()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetZoomRatio Failed with return code %{public}d", ret);
        /* TODO: Need to return default upon error ? Need to do same behavior for other metadata
        as well */
        return 0;
    }
    return static_cast<float>(item.data.f[0]);
}

void CameraInput::SetZoomRatio(float zoomRatio)
{
    int len = sizeof(float);
    float *zoom = (float *) malloc(len);
    if (zoom == NULL) {
        MEDIA_ERR_LOG("CameraInput::SetZoomRatio Memory allocation failed");
        return;
    }
    *zoom = zoomRatio;
    configBatch_[OHOS_CONTROL_ZOOM_RATIO] = std::make_unique<ChangeMetadata>(zoom, len, 1);
    return;
}

std::vector<camera_flash_mode_enum_t> CameraInput::GetSupportedFlashModes()
{
    std::vector<camera_flash_mode_enum_t> supportedFlashModes;
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_ABILITY_DEVICE_AVAILABLE_FLASHMODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetSupportedFlashModes Failed with return code %{public}d", ret);
        return supportedFlashModes;
    }
    getVector(item.data.u8, item.count, supportedFlashModes, camera_flash_mode_enum_t(0));
    return supportedFlashModes;
}

camera_flash_mode_enum_t CameraInput::GetFlashMode()
{
    std::shared_ptr<CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = find_camera_metadata_item(metadata->get(), OHOS_CONTROL_FLASHMODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CameraInput::GetFlashMode Failed with return code %{public}d", ret);
        return OHOS_CAMERA_FLASH_MODE_CLOSE;
    }
    return static_cast<camera_flash_mode_enum_t>(item.data.u8[0]);
}

void CameraInput::SetFlashMode(camera_flash_mode_enum_t flashMode)
{
    int len = sizeof(uint8_t);
    uint8_t *flash = (uint8_t *) malloc(len);
    if (flash == NULL) {
        MEDIA_ERR_LOG("CameraInput::SetFlashMode Memory allocation failed");
	        return;
    }
    *flash = flashMode;
    configBatch_[OHOS_CONTROL_FLASHMODE] = std::make_unique<ChangeMetadata>(flash, len, 1);
    return;
}

void CameraInput::SetErrorCallback(std::shared_ptr<ErrorCallback> errorCallback)
{
    if (errorCallback == nullptr) {
        MEDIA_ERR_LOG("SetErrorCallback: Unregistering error callback");
    }
    errorCallback_ = errorCallback;
    return;
}

sptr<ICameraDeviceService> CameraInput::GetCameraDevice()
{
    return deviceObj_;
}

std::shared_ptr<ErrorCallback> CameraInput::GetErrorCallback()
{
    return errorCallback_;
}
} // CameraStandard
} // OHOS
