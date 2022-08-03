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

#ifndef OHOS_CAMERA_PHOTO_OUTPUT_H
#define OHOS_CAMERA_PHOTO_OUTPUT_H

#include <iostream>
#include "camera_metadata_info.h"
#include "capture_output.h"
#include "istream_capture.h"

namespace OHOS {
namespace CameraStandard {
class PhotoCallback {
public:
    PhotoCallback() = default;
    virtual ~PhotoCallback() = default;

    /**
     * @brief Called when camera capture started.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     */
    virtual void OnCaptureStarted(const int32_t captureID) const = 0;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     * @param frameCount Obtain the constant number of frames for the photo capture callback.
     */
    virtual void OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const = 0;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    virtual void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const = 0;

    /**
     * @brief Called when error occured during camera capture.
     *
     * @param captureId Indicates the pointer in which captureId will be requested.
     * @param errorCode Indicates a {@link ErrorCode} which will give information for photo capture callback error
     */
    virtual void OnCaptureError(const int32_t captureId, const int32_t errorCode) const = 0;
};

typedef struct {
    /**
     * Latitude.
     */
    double latitude;
    /**
     * Longitude.
     */
    double longitude;
    /**
     * Altitude.
     */
    double altitude;
} Location;

class PhotoCaptureSetting {
public:
    enum QualityLevel {
        HIGH_QUALITY = 0,
        NORMAL_QUALITY,
        LOW_QUALITY
    };
    enum RotationConfig {
        Rotation_0 = 0,
        Rotation_90 = 90,
        Rotation_180 = 180,
        Rotation_270 = 270
    };
    PhotoCaptureSetting();
    virtual ~PhotoCaptureSetting() = default;

    /**
     * @brief Get the quality level for the photo capture settings.
     *
     * @return Returns the quality level.
     */
    QualityLevel GetQuality();

    /**
     * @brief Set the quality level for the photo capture settings.
     *
     * @param qualityLevel to be set.
     */
    void SetQuality(QualityLevel qualityLevel);

    /**
     * @brief Get rotation information for the photo capture settings.
     *
     * @return Returns the RotationConfig.
     */
    RotationConfig GetRotation();

    /**
     * @brief Set the Rotation for the photo capture settings.
     *
     * @param rotationvalue to be set.
     */
    void SetRotation(RotationConfig rotationvalue);

    /**
     * @brief Set the GPS Location for the photo capture settings.
     *
     * @param latitude value to be set.
     * @param longitude value to be set.
     */
    void SetGpsLocation(double latitude, double longitude);

    /**
     * @brief Set the GPS Location for the photo capture settings.
     *
     * @param location value to be set.
     */
    void SetLocation(std::unique_ptr<Location> &location);

    /**
     * @brief Set the mirror option for the photo capture.
     *
     * @param boolean true/false to set/unset mirror respectively.
     */
    void SetMirror(bool enable);

    /**
     * @brief Get the photo capture settings metadata information.
     *
     * @return Returns the pointer where CameraMetadata information is present.
     */
    std::shared_ptr<OHOS::Camera::CameraMetadata> GetCaptureMetadataSetting();

private:
    std::shared_ptr<OHOS::Camera::CameraMetadata> captureMetadataSetting_;
};

class PhotoOutput : public CaptureOutput {
public:
    explicit PhotoOutput(sptr<IStreamCapture> &streamCapture);

    /**
     * @brief Set the photo callback.
     *
     * @param callback Requested for the pointer where photo callback is present.
     */
    void SetCallback(std::shared_ptr<PhotoCallback> callback);

    /**
     * @brief Photo capture request using photocapturesettings.
     *
     * @param photoCaptureSettings Requested for the photoCaptureSettings object which has metadata
     * information such as: Rotation, Location, Mirror & Quality.
     */
    int32_t Capture(std::shared_ptr<PhotoCaptureSetting> photoCaptureSettings);

    /**
     * @brief Initiate for the photo capture.
     */
    int32_t Capture();

    /**
     * @brief cancelling the photo capture. Applicable only for burst/ continuous capture.
     */
    int32_t CancelCapture();

    /**
     * @brief Releases the instance of PhotoOutput.
     */
    void Release() override;

    /**
     * @brief Get the application callback information.
     *
     * @return Returns the pointer to PhotoCallback.
     */
    std::shared_ptr<PhotoCallback> GetApplicationCallback();

    /**
     * @brief To check the photo capture is mirrored or not.
     *
     * @return Returns true/false if the photo capture is mirrored/not-mirrored respectively.
     */
    bool IsMirrorSupported();

private:
    std::shared_ptr<PhotoCallback> appCallback_;
    sptr<IStreamCaptureCallback> cameraSvcCallback_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_PHOTO_OUTPUT_H
