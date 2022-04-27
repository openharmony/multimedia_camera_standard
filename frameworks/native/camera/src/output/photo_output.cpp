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

#include "output/photo_output.h"
#include <securec.h>
#include "camera_util.h"
#include "hstream_capture_callback_stub.h"
#include "media_log.h"

using namespace std;

namespace OHOS {
namespace CameraStandard {
namespace {
    constexpr uint8_t QUALITY_NORMAL = 90;
    constexpr uint8_t QUALITY_LOW = 50;
}
PhotoCaptureSetting::PhotoCaptureSetting()
{
    int32_t items = 10;
    int32_t dataLength = 100;
    captureMetadataSetting_ = std::make_shared<Camera::CameraMetadata>(items, dataLength);
}

PhotoCaptureSetting::QualityLevel PhotoCaptureSetting::GetQuality()
{
    QualityLevel quality = LOW_QUALITY;
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_JPEG_QUALITY, &item);
    if (ret != CAM_META_SUCCESS) {
        return NORMAL_QUALITY;
    }
    if (item.data.u8[0] > QUALITY_NORMAL) {
        quality = HIGH_QUALITY;
    } else if (item.data.u8[0] > QUALITY_LOW) {
        quality = NORMAL_QUALITY;
    }
    return quality;
}

void PhotoCaptureSetting::SetQuality(PhotoCaptureSetting::QualityLevel qualityLevel)
{
    bool status = false;
    camera_metadata_item_t item;
    uint8_t highQuality = 100;
    uint8_t normalQuality = 90;
    uint8_t quality = 50;

    if (qualityLevel == HIGH_QUALITY) {
        quality = highQuality;
    } else if (qualityLevel == NORMAL_QUALITY) {
        quality = normalQuality;
    }
    int ret = Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_JPEG_QUALITY, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = captureMetadataSetting_->addEntry(OHOS_JPEG_QUALITY, &quality, 1);
    } else if (ret == CAM_META_SUCCESS) {
        status = captureMetadataSetting_->updateEntry(OHOS_JPEG_QUALITY, &quality, 1);
    }

    if (!status) {
        MEDIA_ERR_LOG("PhotoCaptureSetting::SetQuality Failed to set Quality");
    }
}

PhotoCaptureSetting::RotationConfig PhotoCaptureSetting::GetRotation()
{
    RotationConfig rotation;
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_JPEG_ORIENTATION, &item);
    if (ret == CAM_META_SUCCESS) {
        rotation = static_cast<RotationConfig>(item.data.i32[0]);
        return rotation;
    }
    return RotationConfig::Rotation_0;
}

void PhotoCaptureSetting::SetRotation(PhotoCaptureSetting::RotationConfig rotationValue)
{
    bool status = false;
    camera_metadata_item_t item;
    int32_t rotation = rotationValue;

    int ret = Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_JPEG_ORIENTATION, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = captureMetadataSetting_->addEntry(OHOS_JPEG_ORIENTATION, &rotation, 1);
    } else if (ret == CAM_META_SUCCESS) {
        status = captureMetadataSetting_->updateEntry(OHOS_JPEG_ORIENTATION, &rotation, 1);
    }

    if (!status) {
        MEDIA_ERR_LOG("PhotoCaptureSetting::SetRotation Failed to set Rotation");
    }
    return;
}

void PhotoCaptureSetting::SetGpsLocation(double latitude, double longitude)
{
    double gpsCoordinates[2];
    gpsCoordinates[0] = latitude;
    gpsCoordinates[1] = longitude;
    bool status = false;
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_JPEG_GPS_COORDINATES, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = captureMetadataSetting_->addEntry(OHOS_JPEG_GPS_COORDINATES, gpsCoordinates,
            sizeof(gpsCoordinates) / sizeof(gpsCoordinates[0]));
    } else if (ret == CAM_META_SUCCESS) {
        status = captureMetadataSetting_->updateEntry(OHOS_JPEG_GPS_COORDINATES, gpsCoordinates,
            sizeof(gpsCoordinates) / sizeof(gpsCoordinates[0]));
    }

    if (!status) {
        MEDIA_ERR_LOG("PhotoCaptureSetting::SetGpsLocation Failed to set GPS co-ordinates");
    }
    return;
}

bool PhotoCaptureSetting::IsMirrored()
{
    bool isMirrorEnabled = false;
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_CONTROL_CAPTURE_MIRROR, &item);
    if (ret == CAM_META_SUCCESS) {
        isMirrorEnabled = (item.data.u8[0] > 0) ? true : false;
    }
    return isMirrorEnabled;
}

void PhotoCaptureSetting::SetMirror(bool enable)
{
    bool status = false;
    camera_metadata_item_t item;
    uint8_t mirror = enable;

    int ret = Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_CONTROL_CAPTURE_MIRROR, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = captureMetadataSetting_->addEntry(OHOS_CONTROL_CAPTURE_MIRROR, &mirror, 1);
    } else if (ret == CAM_META_SUCCESS) {
        status = captureMetadataSetting_->updateEntry(OHOS_CONTROL_CAPTURE_MIRROR, &mirror, 1);
    }

    if (!status) {
        MEDIA_ERR_LOG("PhotoCaptureSetting::SetMirror Failed to set mirroring in photo capture setting");
    }
    return;
}

std::shared_ptr<Camera::CameraMetadata> PhotoCaptureSetting::GetCaptureMetadataSetting()
{
    return captureMetadataSetting_;
}

class HStreamCaptureCallbackImpl : public HStreamCaptureCallbackStub {
public:
    sptr<PhotoOutput> photoOutput_ = nullptr;
    HStreamCaptureCallbackImpl() : photoOutput_(nullptr) {
    }

    explicit HStreamCaptureCallbackImpl(const sptr<PhotoOutput>& photoOutput) : photoOutput_(photoOutput) {
    }

    ~HStreamCaptureCallbackImpl()
    {
        photoOutput_ = nullptr;
    }

    int32_t OnCaptureStarted(const int32_t captureId) override
    {
        if (photoOutput_ != nullptr && photoOutput_->GetApplicationCallback() != nullptr) {
            photoOutput_->GetApplicationCallback()->OnCaptureStarted(captureId);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamCaptureCallbackImpl::OnCaptureStarted callback");
        }
        return CAMERA_OK;
    }

    int32_t OnCaptureEnded(const int32_t captureId, const int32_t frameCount) override
    {
        if (photoOutput_ != nullptr && photoOutput_->GetApplicationCallback() != nullptr) {
            photoOutput_->GetApplicationCallback()->OnCaptureEnded(captureId, frameCount);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamCaptureCallbackImpl::OnCaptureEnded callback");
        }
        return CAMERA_OK;
    }

    int32_t OnCaptureError(const int32_t captureId, const int32_t errorCode) override
    {
        if (photoOutput_ != nullptr && photoOutput_->GetApplicationCallback() != nullptr) {
            photoOutput_->GetApplicationCallback()->OnCaptureError(captureId, errorCode);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamCaptureCallbackImpl::OnCaptureError callback");
        }
        return CAMERA_OK;
    }

    int32_t OnFrameShutter(const int32_t captureId, const uint64_t timestamp) override
    {
        if (photoOutput_ != nullptr && photoOutput_->GetApplicationCallback() != nullptr) {
            photoOutput_->GetApplicationCallback()->OnFrameShutter(captureId, timestamp);
        } else {
            MEDIA_INFO_LOG("Discarding HStreamCaptureCallbackImpl::OnFrameShutter callback");
        }
        return CAMERA_OK;
    }
};

PhotoOutput::PhotoOutput(sptr<IStreamCapture> &streamCapture)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE::PHOTO_OUTPUT), streamCapture_(streamCapture)
{}

void PhotoOutput::SetCallback(std::shared_ptr<PhotoCallback> callback)
{
    int32_t errorCode = CAMERA_OK;

    appCallback_ = callback;
    if (appCallback_ != nullptr) {
        if (cameraSvcCallback_ == nullptr) {
            cameraSvcCallback_ = new(std::nothrow) HStreamCaptureCallbackImpl(this);
            if (cameraSvcCallback_ == nullptr) {
                MEDIA_ERR_LOG("PhotoOutput::SetCallback new HStreamCaptureCallbackImpl Failed to register callback");
                appCallback_ = nullptr;
                return;
            }
        }
        errorCode = streamCapture_->SetCallback(cameraSvcCallback_);
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("PhotoOutput::SetCallback: Failed to register callback, errorCode: %{public}d", errorCode);
            cameraSvcCallback_ = nullptr;
            appCallback_ = nullptr;
        }
    }
}

std::shared_ptr<PhotoCallback> PhotoOutput::GetApplicationCallback()
{
    return appCallback_;
}

sptr<IStreamCapture> PhotoOutput::GetStreamCapture()
{
    return streamCapture_;
}

int32_t PhotoOutput::Capture(std::shared_ptr<PhotoCaptureSetting> photoCaptureSettings)
{
    return streamCapture_->Capture(photoCaptureSettings->GetCaptureMetadataSetting());
}

int32_t PhotoOutput::Capture()
{
    int32_t items = 0;
    int32_t dataLength = 0;
    std::shared_ptr<Camera::CameraMetadata> captureMetadataSetting =
        std::make_shared<Camera::CameraMetadata>(items, dataLength);
    return streamCapture_->Capture(captureMetadataSetting);
}

int32_t PhotoOutput::CancelCapture()
{
    return streamCapture_->CancelCapture();
}

void PhotoOutput::Release()
{
    int32_t retCode = streamCapture_->Release();
    if (retCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release Camera Input!, retCode: %{public}d", retCode);
    }
    return;
}
} // CameraStandard
} // OHOS
