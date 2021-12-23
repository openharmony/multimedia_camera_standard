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

#include "hcapture_session.h"
#include "camera_util.h"
#include "media_log.h"

namespace OHOS {
namespace CameraStandard {
HCaptureSession::HCaptureSession(sptr<HCameraHostManager> cameraHostManager,
    sptr<StreamOperatorCallback> streamOperatorCallback)
    : cameraHostManager_(cameraHostManager), streamOperatorCallback_(streamOperatorCallback)
{}

HCaptureSession::~HCaptureSession()
{}

int32_t HCaptureSession::BeginConfig()
{
    if (curState_ == CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::BeginConfig Already in config inprogress state!");
        return CAMERA_INVALID_STATE;
    }
    prevState_ = curState_;
    curState_ = CaptureSessionState::SESSION_CONFIG_INPROGRESS;
    tempCameraDevices_.clear();
    tempStreamCaptures_.clear();
    tempStreamRepeats_.clear();
    deletedStreamIds_.clear();
    return CAMERA_OK;
}

int32_t HCaptureSession::AddInput(sptr<ICameraDeviceService> cameraDevice)
{
    sptr<HCameraDevice> localCameraDevice = nullptr;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::AddInput Need to call BeginConfig before adding input");
        return CAMERA_INVALID_STATE;
    }
    if (cameraDevice == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddInput cameraDevice is null");
        return CAMERA_INVALID_ARG;
    }
    if (!tempCameraDevices_.empty() || cameraDevice_ != nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddInput Only one input is supported");
        return CAMERA_INVALID_SESSION_CFG;
    }
    localCameraDevice = static_cast<HCameraDevice*>(cameraDevice.GetRefPtr());
    tempCameraDevices_.emplace_back(localCameraDevice);
    return CAMERA_OK;
}

int32_t HCaptureSession::AddOutput(sptr<IStreamRepeat> streamRepeat)
{
    sptr<HStreamRepeat> localStreamRepeat = nullptr;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput Need to call BeginConfig before adding output");
        return CAMERA_INVALID_STATE;
    }
    // Temp hack to fix the library linking issue
    sptr<Surface> captureSurface = Surface::CreateSurfaceAsConsumer();
    if (streamRepeat == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput streamRepeat is null");
        return CAMERA_INVALID_ARG;
    }
    localStreamRepeat = static_cast<HStreamRepeat *>(streamRepeat.GetRefPtr());
    if (std::find(tempStreamRepeats_.begin(), tempStreamRepeats_.end(), localStreamRepeat) != tempStreamRepeats_.end()
        || std::find(streamRepeats_.begin(), streamRepeats_.end(), localStreamRepeat) != streamRepeats_.end()) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput Adding same output multiple times");
        return CAMERA_INVALID_SESSION_CFG;
    }
    tempStreamRepeats_.emplace_back(localStreamRepeat);
    return CAMERA_OK;
}

int32_t HCaptureSession::AddOutput(sptr<IStreamCapture> streamCapture)
{
    sptr<HStreamCapture> lStreamCapture = nullptr;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput Need to call BeginConfig before adding output");
        return CAMERA_INVALID_STATE;
    }
    if (streamCapture == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput streamCapture is null");
        return CAMERA_INVALID_ARG;
    }
    lStreamCapture = static_cast<HStreamCapture*>(streamCapture.GetRefPtr());
    if (std::find(tempStreamCaptures_.begin(), tempStreamCaptures_.end(), lStreamCapture) != tempStreamCaptures_.end()
        || std::find(streamCaptures_.begin(), streamCaptures_.end(), lStreamCapture) != streamCaptures_.end()) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput Adding same output multiple times");
        return CAMERA_INVALID_SESSION_CFG;
    }
    tempStreamCaptures_.emplace_back(lStreamCapture);
    return CAMERA_OK;
}

int32_t HCaptureSession::RemoveInput(sptr<ICameraDeviceService> cameraDevice)
{
    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Need to call BeginConfig before removing input");
        return CAMERA_INVALID_STATE;
    }
    MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Removing input is not supported");
    return CAMERA_UNSUPPORTED;
}

int32_t HCaptureSession::RemoveOutput(sptr<IStreamRepeat> streamRepeat)
{
    sptr<HStreamRepeat> lStreamRepeat = nullptr;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Need to call BeginConfig before removing output");
        return CAMERA_INVALID_STATE;
    }
    if (streamRepeat == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput streamRepeat is null");
        return CAMERA_INVALID_ARG;
    }
    lStreamRepeat = static_cast<HStreamRepeat *>(streamRepeat.GetRefPtr());
    auto it = std::find(tempStreamRepeats_.begin(), tempStreamRepeats_.end(), lStreamRepeat);
    if (it != tempStreamRepeats_.end()) {
        tempStreamRepeats_.erase(it);
    } else {
        it = std::find(streamRepeats_.begin(), streamRepeats_.end(), lStreamRepeat);
        if (it != streamRepeats_.end()) {
            deletedStreamIds_.emplace_back(lStreamRepeat->GetStreamId());
        } else {
            MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Invalid output");
            return CAMERA_INVALID_SESSION_CFG;
        }
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::RemoveOutput(sptr<IStreamCapture> streamCapture)
{
    sptr<HStreamCapture> lStreamCapture = nullptr;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Need to call BeginConfig before removing output");
        return CAMERA_INVALID_STATE;
    }
    if (streamCapture == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput streamCapture is null");
        return CAMERA_INVALID_ARG;
    }
    lStreamCapture = static_cast<HStreamCapture *>(streamCapture.GetRefPtr());
    auto it = std::find(tempStreamCaptures_.begin(), tempStreamCaptures_.end(), lStreamCapture);
    if (it != tempStreamCaptures_.end()) {
        tempStreamCaptures_.erase(it);
    } else {
        it = std::find(streamCaptures_.begin(), streamCaptures_.end(), lStreamCapture);
        if (it != streamCaptures_.end()) {
            deletedStreamIds_.emplace_back(lStreamCapture->GetStreamId());
        } else {
            MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Invalid output");
            return CAMERA_INVALID_SESSION_CFG;
        }
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::ValidateSessionInputs()
{
    int32_t rc = CAMERA_OK;

    if (prevState_ == CaptureSessionState::SESSION_INIT) {
        if (tempCameraDevices_.size() != 1) {
            MEDIA_ERR_LOG("HCaptureSession::ValidateSessionInputs No camerainput is added");
            return CAMERA_INVALID_SESSION_CFG;
        }
    } else {
        if (tempCameraDevices_.size() != 0) {
            MEDIA_ERR_LOG("HCaptureSession::ValidateSessionInputs Only one camera input is supported");
            return CAMERA_INVALID_SESSION_CFG;
        }
    }
    return rc;
}

int32_t HCaptureSession::ValidateSessionOutputs()
{
    int32_t rc = CAMERA_OK;
    int32_t currentOutputCnt = streamCaptures_.size() + streamRepeats_.size();

    if (prevState_ == CaptureSessionState::SESSION_INIT) {
        if (tempCameraDevices_.size() == 0) {
            MEDIA_ERR_LOG("HCaptureSession::ValidateSessionOutputs No outputs are added");
            return CAMERA_INVALID_SESSION_CFG;
        }
    } else {
        if (deletedStreamIds_.size() != 0 && (currentOutputCnt - deletedStreamIds_.size() == 0)) {
            MEDIA_ERR_LOG("HCaptureSession::ValidateSessionOutputs All outputs are removed");
            return CAMERA_INVALID_SESSION_CFG;
        }
    }
    return rc;
}

int32_t HCaptureSession::GetStreamOperator()
{
    int32_t rc = CAMERA_OK;

    if (prevState_ == CaptureSessionState::SESSION_INIT) {
        cameraDevice_ = tempCameraDevices_[0];
        tempCameraDevices_.clear();
        rc = cameraDevice_->Open();
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::GetStreamOperator(), Failed to open camera, rc: %{public}d", rc);
            return rc;
        }
        rc = cameraDevice_->GetStreamOperator(streamOperatorCallback_, streamOperator_);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::GetStreamOperator(), GetStreamOperator returned %{public}d", rc);
            return rc;
        }
        cameraAbility_ = cameraDevice_->GetSettings();
    } else {
        MEDIA_INFO_LOG("HCaptureSession::GetStreamOperator(), Camera device is already open!");
    }
    return rc;
}

int32_t HCaptureSession::CheckAndCommitStreams(std::vector<std::shared_ptr<Camera::StreamInfo>> &streamInfos)
{
    Camera::CamRetCode hdiRc = Camera::NO_ERROR;
    Camera::StreamSupportType supportType = Camera::DYNAMIC_SUPPORTED;
    std::vector<int32_t> streamIds;
    std::shared_ptr<Camera::StreamInfo> curStreamInfo = nullptr;

    hdiRc = streamOperator_->IsStreamsSupported(Camera::NORMAL, cameraAbility_, streamInfos, supportType);
    if (hdiRc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCaptureSession::CheckAndCommitStreams(), Error from HDI: %{public}d", hdiRc);
        return HdiToServiceError(hdiRc);
    } else if (supportType != Camera::DYNAMIC_SUPPORTED) {
        MEDIA_ERR_LOG("HCaptureSession::CheckAndCommitStreams(), Config not suported %{public}d", supportType);
        return CAMERA_UNSUPPORTED;
    }
    hdiRc = streamOperator_->CreateStreams(streamInfos);
    if (hdiRc == Camera::NO_ERROR) {
        hdiRc = streamOperator_->CommitStreams(Camera::NORMAL, cameraAbility_);
        if (hdiRc != Camera::NO_ERROR) {
            MEDIA_ERR_LOG("HCaptureSession::CheckAndCommitStreams(), Failed to commit %{public}d", hdiRc);
            for (auto item = streamInfos.begin(); item != streamInfos.end(); ++item) {
                curStreamInfo = *item;
                streamIds.emplace_back(curStreamInfo->streamId_);
            }
            if (streamOperator_->ReleaseStreams(streamIds) != Camera::NO_ERROR) {
                MEDIA_ERR_LOG("HCaptureSession::CheckAndCommitStreams(), Failed to release streams");
            }
        }
    }
    return HdiToServiceError(hdiRc);
}

void HCaptureSession::FindAndDeleteStream(int32_t deletedStreamID)
{
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    for (auto item = streamCaptures_.begin(); item != streamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        if (deletedStreamID == curStreamCapture->GetStreamId()) {
            curStreamCapture->Release();
            streamCaptures_.erase(item);
            return;
        }
    }
    for (auto item = streamRepeats_.begin(); item != streamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        if (deletedStreamID == curStreamRepeat->GetStreamId()) {
            curStreamRepeat->Release();
            streamRepeats_.erase(item);
            return;
        }
    }
}

void HCaptureSession::RestorePreviousState()
{
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    for (auto item = tempStreamCaptures_.begin(); item != tempStreamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        curStreamCapture->Release();
    }
    tempStreamCaptures_.clear();
    for (auto item = tempStreamRepeats_.begin(); item != tempStreamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        curStreamRepeat->Release();
    }
    tempStreamRepeats_.clear();
    deletedStreamIds_.clear();
    tempCameraDevices_.clear();
    curState_ = CaptureSessionState::SESSION_INIT;
}

void HCaptureSession::UpdateSessionConfig()
{
    int32_t curDeletedStreamID = 0;

    for (auto item = deletedStreamIds_.begin(); item != deletedStreamIds_.end(); ++item) {
        curDeletedStreamID = *item;
        FindAndDeleteStream(curDeletedStreamID);
    }
    deletedStreamIds_.clear();
    for (auto item = tempStreamCaptures_.begin(); item != tempStreamCaptures_.end(); ++item) {
        streamCaptures_.emplace_back(*item);
    }
    tempStreamCaptures_.clear();
    for (auto item = tempStreamRepeats_.begin(); item != tempStreamRepeats_.end(); ++item) {
        streamRepeats_.emplace_back(*item);
    }
    tempStreamRepeats_.clear();
    streamOperatorCallback_->SetCaptureSession(this);
    curState_ = CaptureSessionState::SESSION_CONFIG_COMMITTED;
    return;
}

int32_t HCaptureSession::HandleCaptureOuputsConfig()
{
    int32_t rc = CAMERA_OK;
    int32_t streamId = streamId_;
    std::vector<std::shared_ptr<Camera::StreamInfo>> streamInfos;
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;
    std::shared_ptr<Camera::StreamInfo> curStreamInfo = nullptr;

    for (auto item = tempStreamCaptures_.begin(); item != tempStreamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        rc = curStreamCapture->LinkInput(streamOperator_, cameraAbility_, streamId);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::HandleCaptureOuputsConfig() Failed to link Output, %{public}d", rc);
            return rc;
        }
        curStreamInfo = std::make_shared<Camera::StreamInfo>();
        curStreamCapture->SetStreamInfo(curStreamInfo);
        streamInfos.push_back(curStreamInfo);
        streamId++;
        curStreamInfo = nullptr;
    }
    for (auto item = tempStreamRepeats_.begin(); item != tempStreamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        rc = curStreamRepeat->LinkInput(streamOperator_, cameraAbility_, streamId);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::HandleCaptureOuputsConfig() Failed to link Output, %{public}d", rc);
            return rc;
        }
        curStreamInfo = std::make_shared<Camera::StreamInfo>();
        curStreamRepeat->SetStreamInfo(curStreamInfo);
        streamInfos.push_back(curStreamInfo);
        streamId++;
        curStreamInfo = nullptr;
    }
    if (!streamInfos.empty()) {
        rc = CheckAndCommitStreams(streamInfos);
        if (rc == CAMERA_OK) {
            streamId_ = streamId;
        }
    } else {
        rc = CAMERA_UNSUPPORTED;
    }
    return rc;
}

int32_t HCaptureSession::CommitConfig()
{
    int32_t rc = CAMERA_OK;
    streamOperator_ = nullptr;
    std::shared_ptr<Camera::StreamInfo> streamInfo = nullptr;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Need to call BeginConfig before committing configuration");
        return CAMERA_INVALID_STATE;
    }
    rc = ValidateSessionInputs();
    if (rc == CAMERA_OK) {
        rc = ValidateSessionOutputs();
        if (rc == CAMERA_OK) {
            rc = GetStreamOperator();
        }
    }
    if (rc == CAMERA_OK) {
        if (!deletedStreamIds_.empty()) {
            rc = HdiToServiceError(streamOperator_->ReleaseStreams(deletedStreamIds_));
        }
        if (rc == CAMERA_OK) {
            rc = HandleCaptureOuputsConfig();
            if (rc == CAMERA_OK) {
                UpdateSessionConfig();
            }
        }
    } else {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig(), Failed to commit config, rc: %{public}d", rc);
        RestorePreviousState();
    }
    return rc;
}

int32_t HCaptureSession::Start()
{
    int32_t rc = CAMERA_INVALID_STATE;
    sptr<HStreamRepeat> curStreamRepeat;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_COMMITTED) {
        return rc;
    }
    for (auto item = streamRepeats_.begin(); item != streamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        if (!curStreamRepeat->IsVideo()) {
            rc = curStreamRepeat->Start();
            if (rc != CAMERA_OK) {
                MEDIA_ERR_LOG("HCaptureSession::Start(), Failed to start preview, rc: %{public}d", rc);
                break;
            }
        }
    }
    return rc;
}

int32_t HCaptureSession::Stop()
{
    int32_t rc = CAMERA_INVALID_STATE;
    sptr<HStreamRepeat> curStreamRepeat;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_COMMITTED) {
        return rc;
    }

    for (auto item = streamRepeats_.begin(); item != streamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        if (!curStreamRepeat->IsVideo()) {
            rc = curStreamRepeat->Stop();
            if (rc != CAMERA_OK) {
                MEDIA_ERR_LOG("HCaptureSession::Stop(), Failed to stop preview, rc: %{public}d", rc);
                break;
            }
        }
    }
    return rc;
}

void HCaptureSession::ReleaseStreams()
{
    std::vector<int32_t> streamIds;
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    for (auto item = streamRepeats_.begin(); item != streamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        streamIds.emplace_back(curStreamRepeat->GetStreamId());
        curStreamRepeat->Release();
    }
    streamRepeats_.clear();
    HStreamRepeat::ResetCaptureIds();
    for (auto item = streamCaptures_.begin(); item != streamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        streamIds.emplace_back(curStreamCapture->GetStreamId());
        curStreamCapture->Release();
    }
    streamCaptures_.clear();
    HStreamCapture::ResetCaptureId();
    if (streamOperator_ != nullptr && !streamIds.empty()) {
        streamOperator_->ReleaseStreams(streamIds);
    }
}

int32_t HCaptureSession::Release()
{
    ReleaseStreams();
    if (streamOperatorCallback_ != nullptr) {
        streamOperatorCallback_->SetCaptureSession(nullptr);
        streamOperatorCallback_ = nullptr;
    }
    if (cameraDevice_ != nullptr) {
        cameraDevice_->Close();
        cameraDevice_ = nullptr;
    }
    return CAMERA_OK;
}

StreamOperatorCallback::StreamOperatorCallback(sptr<HCaptureSession> session)
{
    captureSession_ = session;
}

sptr<HStreamRepeat> StreamOperatorCallback::GetStreamRepeatByStreamID(int32_t streamId)
{
    sptr<HStreamRepeat> curStreamRepeat;
    sptr<HStreamRepeat> result = nullptr;

    if (captureSession_ != nullptr) {
        for (auto item = captureSession_->streamRepeats_.begin();
            item != captureSession_->streamRepeats_.end(); ++item) {
            curStreamRepeat = *item;
            if (curStreamRepeat->GetStreamId() == streamId) {
                result = curStreamRepeat;
                break;
            }
        }
    }
    return result;
}

sptr<HStreamCapture> StreamOperatorCallback::GetStreamCaptureByStreamID(int32_t streamId)
{
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamCapture> result = nullptr;

    if (captureSession_ != nullptr) {
        for (auto item = captureSession_->streamCaptures_.begin();
            item != captureSession_->streamCaptures_.end(); ++item) {
            curStreamCapture = *item;
            if (curStreamCapture->GetStreamId() == streamId) {
                result = curStreamCapture;
                break;
            }
        }
    }
    return result;
}


void StreamOperatorCallback::OnCaptureStarted(int32_t captureId,
                                              const std::vector<int32_t> &streamIds)
{
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    for (auto item = streamIds.begin(); item != streamIds.end(); ++item) {
        curStreamRepeat = GetStreamRepeatByStreamID(*item);
        if (curStreamRepeat != nullptr) {
            curStreamRepeat->OnFrameStarted();
        } else {
            curStreamCapture = GetStreamCaptureByStreamID(*item);
            if (curStreamCapture != nullptr) {
                curStreamCapture->OnCaptureStarted(captureId);
            } else {
                MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureStarted, Stream not found: %{public}d", *item);
            }
        }
    }
}

void StreamOperatorCallback::OnCaptureEnded(int32_t captureId,
                                            const std::vector<std::shared_ptr<Camera::CaptureEndedInfo>> &info)
{
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;
    std::shared_ptr<Camera::CaptureEndedInfo> captureInfo = nullptr;

    for (auto item = info.begin(); item != info.end(); ++item) {
        captureInfo = *item;
        curStreamRepeat = GetStreamRepeatByStreamID(captureInfo->streamId_);
        if (curStreamRepeat != nullptr) {
            curStreamRepeat->OnFrameEnded(info[0]->frameCount_);
        } else {
            curStreamCapture = GetStreamCaptureByStreamID(captureInfo->streamId_);
            if (curStreamCapture != nullptr) {
                curStreamCapture->OnCaptureEnded(captureId);
            } else {
                MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureEnded, not found: %{public}d", captureInfo->streamId_);
            }
        }
    }
}

void StreamOperatorCallback::OnCaptureError(int32_t captureId,
                                            const std::vector<std::shared_ptr<Camera::CaptureErrorInfo>> &info)
{
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;
    std::shared_ptr<Camera::CaptureErrorInfo> errInfo = nullptr;

    for (auto item = info.begin(); item != info.end(); ++item) {
        errInfo = *item;
        curStreamRepeat = GetStreamRepeatByStreamID(errInfo->streamId_);
        if (curStreamRepeat != nullptr) {
            curStreamRepeat->OnFrameError(info[0]->error_);
        } else {
            curStreamCapture = GetStreamCaptureByStreamID(errInfo->streamId_);
            if (curStreamCapture != nullptr) {
                curStreamCapture->OnCaptureError(captureId, info[0]->error_);
            } else {
                MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureEnded, not found: %{public}d", errInfo->streamId_);
            }
        }
    }
}

void StreamOperatorCallback::OnFrameShutter(int32_t captureId,
                                            const std::vector<int32_t> &streamIds, uint64_t timestamp)
{
    sptr<HStreamCapture> curStreamCapture;

    for (auto item = streamIds.begin(); item != streamIds.end(); ++item) {
        curStreamCapture = GetStreamCaptureByStreamID(*item);
        if (curStreamCapture != nullptr) {
            curStreamCapture->OnFrameShutter(captureId, timestamp);
        } else {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnFrameShutter, Stream not found: %{public}d", *item);
        }
    }
}

void StreamOperatorCallback::SetCaptureSession(sptr<HCaptureSession> captureSession)
{
    captureSession_ = captureSession;
}
} // namespace CameraStandard
} // namespace OHOS
