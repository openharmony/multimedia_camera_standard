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

#include "session/capture_session.h"
#include "camera_util.h"
#include "hcapture_session_callback_stub.h"
#include "input/camera_input.h"
#include "camera_log.h"
#include "output/photo_output.h"
#include "output/preview_output.h"
#include "output/video_output.h"

namespace OHOS {
namespace CameraStandard {
class CaptureSessionCallback : public HCaptureSessionCallbackStub {
public:
    sptr<CaptureSession> captureSession_ = nullptr;
    CaptureSessionCallback() : captureSession_(nullptr) {
    }

    explicit CaptureSessionCallback(const sptr<CaptureSession> &captureSession) : captureSession_(captureSession) {
    }

    ~CaptureSessionCallback()
    {
        captureSession_ = nullptr;
    }

    int32_t OnError(int32_t errorCode) override
    {
        MEDIA_INFO_LOG("CaptureSessionCallback::OnError() is called!, errorCode: %{public}d",
                       errorCode);
        if (captureSession_ != nullptr && captureSession_->GetApplicationCallback() != nullptr) {
            captureSession_->GetApplicationCallback()->OnError(errorCode);
        } else {
            MEDIA_INFO_LOG("CaptureSessionCallback::ApplicationCallback not set!, Discarding callback");
        }
        return CAMERA_OK;
    }
};

CaptureSession::CaptureSession(sptr<ICaptureSession> &captureSession)
{
    captureSession_ = captureSession;
    inputDevice_ = nullptr;
}

CaptureSession::~CaptureSession()
{
    if (inputDevice_ != nullptr) {
        inputDevice_ = nullptr;
    }
}

int32_t CaptureSession::BeginConfig()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->BeginConfig();
}

int32_t CaptureSession::CommitConfig()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->CommitConfig();
}

int32_t CaptureSession::AddInput(sptr<CaptureInput> &input)
{
    CAMERA_SYNC_TRACE;
    if (input == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::AddInput input is null");
        return CAMERA_INVALID_ARG;
    }
    inputDevice_ = input;
    return captureSession_->AddInput(((sptr<CameraInput> &)input)->GetCameraDevice());
}

int32_t CaptureSession::AddOutput(sptr<CaptureOutput> &output)
{
    CAMERA_SYNC_TRACE;
    if (output == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::AddOutput output is null");
        return CAMERA_INVALID_ARG;
    }
    output->SetSession(this);
    return captureSession_->AddOutput(output->GetStreamType(), output->GetStream());
}

int32_t CaptureSession::RemoveInput(sptr<CaptureInput> &input)
{
    CAMERA_SYNC_TRACE;
    if (input == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::RemoveInput input is null");
        return CAMERA_INVALID_ARG;
    }
    if (inputDevice_ != nullptr) {
        inputDevice_ = nullptr;
    }
    return captureSession_->RemoveInput(((sptr<CameraInput> &)input)->GetCameraDevice());
}

int32_t CaptureSession::RemoveOutput(sptr<CaptureOutput> &output)
{
    CAMERA_SYNC_TRACE;
    if (output == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::RemoveOutput output is null");
        return CAMERA_INVALID_ARG;
    }
    output->SetSession(nullptr);
    return captureSession_->RemoveOutput(output->GetStreamType(), output->GetStream());
}

int32_t CaptureSession::Start()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->Start();
}

int32_t CaptureSession::Stop()
{
    CAMERA_SYNC_TRACE;
    return captureSession_->Stop();
}

void CaptureSession::SetCallback(std::shared_ptr<SessionCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetCallback: Unregistering application callback!");
    }
    int32_t errorCode = CAMERA_OK;

    appCallback_ = callback;
    if (appCallback_ != nullptr) {
        if (captureSessionCallback_ == nullptr) {
            captureSessionCallback_ = new(std::nothrow) CaptureSessionCallback(this);
        }
        errorCode = captureSession_->SetCallback(captureSessionCallback_);
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("CaptureSession::SetCallback: Failed to register callback, errorCode: %{public}d", errorCode);
            captureSessionCallback_ = nullptr;
            appCallback_ = nullptr;
        }
    }
    return;
}

const std::unordered_map<CameraVideoStabilizationMode,
VideoStabilizationMode> CaptureSession::metaToFwVideoStabModes_ = {
    {OHOS_CAMERA_VIDEO_STABILIZATION_OFF, OFF},
    {OHOS_CAMERA_VIDEO_STABILIZATION_LOW, LOW},
    {OHOS_CAMERA_VIDEO_STABILIZATION_MIDDLE, MIDDLE},
    {OHOS_CAMERA_VIDEO_STABILIZATION_HIGH, HIGH},
    {OHOS_CAMERA_VIDEO_STABILIZATION_AUTO, AUTO}
};

const std::unordered_map<VideoStabilizationMode,
CameraVideoStabilizationMode> CaptureSession::fwToMetaVideoStabModes_ = {
    {OFF, OHOS_CAMERA_VIDEO_STABILIZATION_OFF},
    {LOW, OHOS_CAMERA_VIDEO_STABILIZATION_LOW},
    {MIDDLE, OHOS_CAMERA_VIDEO_STABILIZATION_MIDDLE},
    {HIGH, OHOS_CAMERA_VIDEO_STABILIZATION_HIGH},
    {AUTO, OHOS_CAMERA_VIDEO_STABILIZATION_AUTO}
};

VideoStabilizationMode CaptureSession::GetActiveVideoStabilizationMode()
{
    sptr<CameraInfo> cameraObj_;
    if (inputDevice_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::GetActiveVideoStabilizationMode camera device is null");
        return OFF;
    }
    cameraObj_ = inputDevice_->GetCameraDeviceInfo();
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_VIDEO_STABILIZATION_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        auto itr = metaToFwVideoStabModes_.find(static_cast<CameraVideoStabilizationMode>(item.data.u8[0]));
        if (itr != metaToFwVideoStabModes_.end()) {
            return itr->second;
        }
    }
    return OFF;
}

void CaptureSession::SetVideoStabilizationMode(VideoStabilizationMode stabilizationMode)
{
    auto itr = fwToMetaVideoStabModes_.find(stabilizationMode);
    if ((itr == fwToMetaVideoStabModes_.end()) || !IsVideoStabilizationModeSupported(stabilizationMode)) {
        MEDIA_ERR_LOG("CaptureSession::SetVideoStabilizationMode Mode: %{public}d not supported", stabilizationMode);
        return;
    }
    SetVideoStabilizingMode((sptr<CameraInput> &) inputDevice_, itr->second);
}

bool CaptureSession::IsVideoStabilizationModeSupported(VideoStabilizationMode stabilizationMode)
{
    std::vector<VideoStabilizationMode> stabilizationModes = GetSupportedStabilizationMode();
    if (std::find(stabilizationModes.begin(), stabilizationModes.end(), stabilizationMode)
       != stabilizationModes.end()) {
        return true;
    }
    return false;
}

std::vector<VideoStabilizationMode> CaptureSession::GetSupportedStabilizationMode()
{
    std::vector<VideoStabilizationMode> stabilizationModes;
    
    sptr<CameraInfo> cameraObj_;
    if (inputDevice_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedStabilizationMode camera device is null");
        return stabilizationModes;
    }
    cameraObj_ = inputDevice_->GetCameraDeviceInfo();
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_VIDEO_STABILIZATION_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupporteStabilizationModes Failed with return code %{public}d", ret);
        return stabilizationModes;
    }

    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwVideoStabModes_.find(static_cast<CameraVideoStabilizationMode>(item.data.u8[i]));
        if (itr != metaToFwVideoStabModes_.end()) {
            stabilizationModes.emplace_back(itr->second);
        }
    }
    return stabilizationModes;
}

std::shared_ptr<SessionCallback> CaptureSession::GetApplicationCallback()
{
    return appCallback_;
}

void CaptureSession::Release()
{
    CAMERA_SYNC_TRACE;
    int32_t errCode = captureSession_->Release(0);
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to Release capture session!, %{public}d", errCode);
    }
}
} // CameraStandard
} // OHOS
