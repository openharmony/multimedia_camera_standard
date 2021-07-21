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

#include "output/photo_output.h"
#include "camera_util.h"
#include "media_log.h"

using namespace std;
namespace OHOS {
namespace CameraStandard {

PhotoCaptureSetting::QualityLevel PhotoCaptureSetting::GetQuaility()
{
    return QualityLevel::NORMAL_QUALITY;
}

void PhotoCaptureSetting::SetQuaility(PhotoCaptureSetting::QualityLevel quaility)
{
    return;
}

PhotoCaptureSetting::RotationConfig PhotoCaptureSetting::GetRotation()
{
    return RotationConfig::Rotation_0;
}

void PhotoCaptureSetting::SetRotation(PhotoCaptureSetting::RotationConfig ratationValue)
{
    return;
}

bool PhotoCaptureSetting::IsMirrored()
{
    return false;
}

void PhotoCaptureSetting::SetMirror(bool enable){
    return;
}

PhotoOutput::PhotoOutput (sptr<IStreamCapture> &streamCapture)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE::PHOTO_OUTPUT), streamCapture_(streamCapture) {
}

void PhotoOutput::SetCallback(std::shared_ptr<PhotoCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("PhotoOutput::SetCallback: Unregistering application callback!");
    }
    appCallback_ = callback;
    return;
}

sptr<IStreamCapture> PhotoOutput::GetStreamCapture()
{
    return streamCapture_;
}

int32_t PhotoOutput::Capture(std::shared_ptr<PhotoCaptureSetting> photoCaptureSettings)
{
    return -1;
}

int32_t PhotoOutput::Capture()
{
    return streamCapture_->Capture();
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
