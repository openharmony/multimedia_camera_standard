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

#include "camera_util.h"
#include "media_log.h"
#include "surface.h"
#include "ipc_skeleton.h"
#include "hcapture_session.h"

namespace OHOS {
namespace CameraStandard {
static std::map<int32_t, sptr<HCaptureSession>> session_;
HCaptureSession::HCaptureSession(sptr<HCameraHostManager> cameraHostManager,
    sptr<StreamOperatorCallback> streamOperatorCb)
    : cameraHostManager_(cameraHostManager), streamOperatorCallback_(streamOperatorCb)
{
    std::map<int32_t, sptr<HCaptureSession>>::iterator it;
    pid_t pid = IPCSkeleton::GetCallingPid();
    MEDIA_DEBUG_LOG("HCaptureSession: camera stub services(%{public}zu) pid(%{public}d).", session_.size(), pid);
    it = session_.find(pid);
    if (it != session_.end()) {
        MEDIA_ERR_LOG("HCaptureSession::HCaptureSession doesn't support multiple sessions per pid");
    } else {
        session_[pid] = this;
    }
    MEDIA_DEBUG_LOG("HCaptureSession: camera stub services(%{public}zu).", session_.size());
}

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
    if (!tempCameraDevices_.empty() || (cameraDevice_ != nullptr && !cameraDevice_->IsReleaseCameraDevice())) {
        MEDIA_ERR_LOG("HCaptureSession::AddInput Only one input is supported");
        return CAMERA_INVALID_SESSION_CFG;
    }
    localCameraDevice = static_cast<HCameraDevice*>(cameraDevice.GetRefPtr());
    if (cameraDevice_ == localCameraDevice) {
        cameraDevice_->SetReleaseCameraDevice(false);
    } else {
        tempCameraDevices_.emplace_back(localCameraDevice);
    }
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
    localStreamRepeat->SetReleaseStream(false);
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
    lStreamCapture->SetReleaseStream(false);
    tempStreamCaptures_.emplace_back(lStreamCapture);
    return CAMERA_OK;
}

int32_t HCaptureSession::RemoveInput(sptr<ICameraDeviceService> cameraDevice)
{
    sptr<HCameraDevice> localCameraDevice;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveInput Need to call BeginConfig before removing input");
        return CAMERA_INVALID_STATE;
    }
    if (cameraDevice == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveInput cameraDevice is null");
        return CAMERA_INVALID_ARG;
    }
    localCameraDevice = static_cast<HCameraDevice*>(cameraDevice.GetRefPtr());
    auto it = std::find(tempCameraDevices_.begin(), tempCameraDevices_.end(), localCameraDevice);
    if (it != tempCameraDevices_.end()) {
        tempCameraDevices_.erase(it);
    } else if (cameraDevice_ == localCameraDevice) {
        cameraDevice_->SetReleaseCameraDevice(true);
    } else {
        MEDIA_ERR_LOG("HCaptureSession::RemoveInput Invalid camera device");
        return CAMERA_INVALID_SESSION_CFG;
    }
    return CAMERA_OK;
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
            if (!lStreamRepeat->IsReleaseStream()) {
                deletedStreamIds_.emplace_back(lStreamRepeat->GetStreamId());
                lStreamRepeat->SetReleaseStream(true);
            }
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
            if (!lStreamCapture->IsReleaseStream()) {
                deletedStreamIds_.emplace_back(lStreamCapture->GetStreamId());
                lStreamCapture->SetReleaseStream(true);
            }
        } else {
            MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Invalid output");
            return CAMERA_INVALID_SESSION_CFG;
        }
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::ValidateSessionInputs()
{
    if (tempCameraDevices_.empty() && (cameraDevice_ == nullptr || cameraDevice_->IsReleaseCameraDevice())) {
        MEDIA_ERR_LOG("HCaptureSession::ValidateSessionInputs No inputs present");
        return CAMERA_INVALID_SESSION_CFG;
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::ValidateSessionOutputs()
{
    int32_t currentOutputCnt = streamCaptures_.size() + streamRepeats_.size();
    int32_t newOutputCnt = tempStreamCaptures_.size() + tempStreamRepeats_.size();
    if (newOutputCnt + currentOutputCnt - deletedStreamIds_.size() == 0) {
        MEDIA_ERR_LOG("HCaptureSession::ValidateSessionOutputs No outputs present");
        return CAMERA_INVALID_SESSION_CFG;
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::GetCameraDevice(sptr<HCameraDevice> &device)
{
    int32_t rc;
    sptr<HCameraDevice> camDevice;
    sptr<Camera::IStreamOperator> streamOperator;

    if (cameraDevice_ != nullptr && !cameraDevice_->IsReleaseCameraDevice()) {
        MEDIA_DEBUG_LOG("HCaptureSession::GetCameraDevice Camera device has not changed");
        device = cameraDevice_;
        return CAMERA_OK;
    }
    camDevice = tempCameraDevices_[0];
    rc = camDevice->Open();
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::GetCameraDevice Failed to open camera, rc: %{public}d", rc);
        return rc;
    }
    rc = camDevice->GetStreamOperator(streamOperatorCallback_, streamOperator);
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::GetCameraDevice GetStreamOperator returned %{public}d", rc);
        camDevice->Close();
        return rc;
    }
    device = camDevice;
    return rc;
}

int32_t HCaptureSession::GetCurrentStreamInfos(sptr<HCameraDevice> &device,
                                               std::shared_ptr<CameraMetadata> &deviceSettings,
                                               std::vector<std::shared_ptr<Camera::StreamInfo>> &streamInfos)
{
    int32_t rc;
    int32_t streamId = streamId_;
    bool isNeedLink;
    std::shared_ptr<Camera::StreamInfo> curStreamInfo;
    sptr<Camera::IStreamOperator> streamOperator;
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    streamOperator = device->GetStreamOperator();
    isNeedLink = (device != cameraDevice_);
    for (auto item = streamCaptures_.begin(); item != streamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        if (curStreamCapture->IsReleaseStream()) {
            continue;
        }
        if (isNeedLink) {
            rc = curStreamCapture->LinkInput(streamOperator, deviceSettings, streamId);
            if (rc != CAMERA_OK) {
                MEDIA_ERR_LOG("HCaptureSession::GetCurrentStreamInfos() Failed to link Output, %{public}d", rc);
                return rc;
            }
            streamId++;
        }
        curStreamInfo = std::make_shared<Camera::StreamInfo>();
        curStreamCapture->SetStreamInfo(curStreamInfo);
        streamInfos.push_back(curStreamInfo);
    }
    for (auto item = streamRepeats_.begin(); item != streamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        if (curStreamRepeat->IsReleaseStream()) {
            continue;
        }
        if (isNeedLink) {
            rc = curStreamRepeat->LinkInput(streamOperator, deviceSettings, streamId);
            if (rc != CAMERA_OK) {
                MEDIA_ERR_LOG("HCaptureSession::GetCurrentStreamInfos() Failed to link Output, %{public}d", rc);
                return rc;
            }
            streamId++;
        }
        curStreamInfo = std::make_shared<Camera::StreamInfo>();
        curStreamRepeat->SetStreamInfo(curStreamInfo);
        streamInfos.push_back(curStreamInfo);
    }

    if (streamId != streamId_) {
        streamId_ = streamId;
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::CreateAndCommitStreams(sptr<HCameraDevice> &device,
                                                std::shared_ptr<CameraMetadata> &deviceSettings,
                                                std::vector<std::shared_ptr<Camera::StreamInfo>> &streamInfos)
{
    Camera::CamRetCode hdiRc = Camera::NO_ERROR;
    std::vector<int32_t> streamIds;
    std::shared_ptr<Camera::StreamInfo> curStreamInfo;
    sptr<Camera::IStreamOperator> streamOperator;

    streamOperator = device->GetStreamOperator();
    if (!streamInfos.empty()) {
        hdiRc = streamOperator->CreateStreams(streamInfos);
    } else {
        MEDIA_INFO_LOG("HCaptureSession::CreateAndCommitStreams(), No new streams to create");
    }
    if (hdiRc == Camera::NO_ERROR) {
        hdiRc = streamOperator->CommitStreams(Camera::NORMAL, deviceSettings);
        if (hdiRc != Camera::NO_ERROR) {
            MEDIA_ERR_LOG("HCaptureSession::CreateAndCommitStreams(), Failed to commit %{public}d", hdiRc);
            for (auto item = streamInfos.begin(); item != streamInfos.end(); ++item) {
                curStreamInfo = *item;
                streamIds.emplace_back(curStreamInfo->streamId_);
            }
            if (!streamIds.empty() && streamOperator->ReleaseStreams(streamIds) != Camera::NO_ERROR) {
                MEDIA_ERR_LOG("HCaptureSession::CreateAndCommitStreams(), Failed to release streams");
            }
        }
    }
    return HdiToServiceError(hdiRc);
}

int32_t HCaptureSession::CheckAndCommitStreams(sptr<HCameraDevice> &device,
                                               std::shared_ptr<CameraMetadata> &deviceSettings,
                                               std::vector<std::shared_ptr<Camera::StreamInfo>> &allStreamInfos,
                                               std::vector<std::shared_ptr<Camera::StreamInfo>> &newStreamInfos)
{
#ifndef BALTIMORE_CAMERA
    Camera::CamRetCode hdiRc = Camera::NO_ERROR;
    Camera::StreamSupportType supportType = Camera::DYNAMIC_SUPPORTED;

    hdiRc = device->GetStreamOperator()->IsStreamsSupported(Camera::NORMAL, deviceSettings,
                                                            allStreamInfos, supportType);
    if (hdiRc != Camera::NO_ERROR) {
        MEDIA_ERR_LOG("HCaptureSession::CheckAndCommitStreams(), Error from HDI: %{public}d", hdiRc);
        return HdiToServiceError(hdiRc);
    } else if (supportType != Camera::DYNAMIC_SUPPORTED) {
        MEDIA_ERR_LOG("HCaptureSession::CheckAndCommitStreams(), Config not suported %{public}d", supportType);
        return CAMERA_UNSUPPORTED;
    }
#endif
    return CreateAndCommitStreams(device, deviceSettings, newStreamInfos);
}

void HCaptureSession::DeleteReleasedStream()
{
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    for (auto item = streamCaptures_.begin(); item != streamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        if (curStreamCapture->IsReleaseStream()) {
            curStreamCapture->Release();
            streamCaptures_.erase(item--);
        }
    }
    for (auto item = streamRepeats_.begin(); item != streamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        if (curStreamRepeat->IsReleaseStream()) {
            curStreamRepeat->Release();
            streamRepeats_.erase(item--);
        }
    }
}

void HCaptureSession::RestorePreviousState(sptr<HCameraDevice> &device, bool isCreateReleaseStreams)
{
    std::vector<std::shared_ptr<Camera::StreamInfo>> streamInfos;
    std::shared_ptr<Camera::StreamInfo> streamInfo;
    std::shared_ptr<CameraMetadata> settings;
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    MEDIA_DEBUG_LOG("HCaptureSession::RestorePreviousState, Restore to previous state");

    for (auto item = streamCaptures_.begin(); item != streamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        if (isCreateReleaseStreams && curStreamCapture->IsReleaseStream()) {
            streamInfo = std::make_shared<Camera::StreamInfo>();
            curStreamCapture->SetStreamInfo(streamInfo);
            streamInfos.push_back(streamInfo);
        }
        curStreamCapture->SetReleaseStream(false);
    }
    for (auto item = streamRepeats_.begin(); item != streamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        if (isCreateReleaseStreams && curStreamRepeat->IsReleaseStream()) {
            streamInfo = std::make_shared<Camera::StreamInfo>();
            curStreamRepeat->SetStreamInfo(streamInfo);
            streamInfos.push_back(streamInfo);
        }
        curStreamRepeat->SetReleaseStream(false);
    }

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
    if (device != nullptr) {
        device->SetReleaseCameraDevice(false);
        if (isCreateReleaseStreams) {
            settings = device->GetSettings();
            if (settings != nullptr) {
                CreateAndCommitStreams(device, settings, streamInfos);
            }
        }
    }
    curState_ = prevState_;
}

void HCaptureSession::UpdateSessionConfig(sptr<HCameraDevice> &device)
{
    DeleteReleasedStream();
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
    cameraDevice_ = device;
    curState_ = CaptureSessionState::SESSION_CONFIG_COMMITTED;
    return;
}

int32_t HCaptureSession::HandleCaptureOuputsConfig(sptr<HCameraDevice> &device)
{
    int32_t rc;
    int32_t streamId;
    std::vector<std::shared_ptr<Camera::StreamInfo>> newStreamInfos;
    std::vector<std::shared_ptr<Camera::StreamInfo>> allStreamInfos;
    std::shared_ptr<Camera::StreamInfo> curStreamInfo;
    std::shared_ptr<CameraMetadata> settings;
    sptr<Camera::IStreamOperator> streamOperator;
    sptr<HStreamCapture> curStreamCapture;
    sptr<HStreamRepeat> curStreamRepeat;

    settings = device->GetSettings();
    if (settings == nullptr) {
        return CAMERA_UNKNOWN_ERROR;
    }

    rc = GetCurrentStreamInfos(device, settings, allStreamInfos);
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::HandleCaptureOuputsConfig() Failed to get streams info, %{public}d", rc);
        return rc;
    }

    if (cameraDevice_ != device) {
        newStreamInfos = allStreamInfos;
    }

    streamOperator = device->GetStreamOperator();
    streamId = streamId_;
    for (auto item = tempStreamCaptures_.begin(); item != tempStreamCaptures_.end(); ++item) {
        curStreamCapture = *item;
        rc = curStreamCapture->LinkInput(streamOperator, settings, streamId);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::HandleCaptureOuputsConfig() Failed to link Output, %{public}d", rc);
            return rc;
        }
        curStreamInfo = std::make_shared<Camera::StreamInfo>();
        curStreamCapture->SetStreamInfo(curStreamInfo);
        newStreamInfos.push_back(curStreamInfo);
        allStreamInfos.push_back(curStreamInfo);
        streamId++;
    }
    for (auto item = tempStreamRepeats_.begin(); item != tempStreamRepeats_.end(); ++item) {
        curStreamRepeat = *item;
        rc = curStreamRepeat->LinkInput(streamOperator, settings, streamId);
        if (rc != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::HandleCaptureOuputsConfig() Failed to link Output, %{public}d", rc);
            return rc;
        }
        curStreamInfo = std::make_shared<Camera::StreamInfo>();
        curStreamRepeat->SetStreamInfo(curStreamInfo);
        newStreamInfos.push_back(curStreamInfo);
        allStreamInfos.push_back(curStreamInfo);
        streamId++;
    }

    rc = CheckAndCommitStreams(device, settings, allStreamInfos, newStreamInfos);
    if (rc == CAMERA_OK) {
        streamId_ = streamId;
    }
    return rc;
}

int32_t HCaptureSession::CommitConfig()
{
    int32_t rc;
    sptr<HCameraDevice> device = nullptr;

    if (curState_ != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Need to call BeginConfig before committing configuration");
        return CAMERA_INVALID_STATE;
    }

    rc = ValidateSessionInputs();
    if (rc != CAMERA_OK) {
        return rc;
    }
    rc = ValidateSessionOutputs();
    if (rc != CAMERA_OK) {
        return rc;
    }

    rc = GetCameraDevice(device);
    if ((rc == CAMERA_OK) && (device == cameraDevice_) && !deletedStreamIds_.empty()) {
        rc = HdiToServiceError(device->GetStreamOperator()->ReleaseStreams(deletedStreamIds_));
    }

    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Failed to commit config. rc: %{public}d", rc);
        if (device != nullptr && device != cameraDevice_) {
            device->Close();
        }
        RestorePreviousState(cameraDevice_, false);
        return rc;
    }

    rc = HandleCaptureOuputsConfig(device);
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Failed to commit config. rc: %{public}d", rc);
        if (device != nullptr && device != cameraDevice_) {
            device->Close();
        }
        RestorePreviousState(cameraDevice_, !deletedStreamIds_.empty());
        return rc;
    }
    if (cameraDevice_ != nullptr && device != cameraDevice_) {
        cameraDevice_->Close();
        cameraDevice_ = nullptr;
    }
    UpdateSessionConfig(device);
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

void HCaptureSession::ClearCaptureSession(pid_t pid)
{
    MEDIA_DEBUG_LOG("ClearCaptureSession: camera stub services(%{public}zu) pid(%{public}d).", session_.size(), pid);
    auto it = session_.find(pid);
    if (it != session_.end()) {
        session_.erase(it);
    }
    MEDIA_DEBUG_LOG("ClearCaptureSession: camera stub services(%{public}zu).", session_.size());
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
    if ((cameraDevice_ != nullptr) && (cameraDevice_->GetStreamOperator() != nullptr) && !streamIds.empty()) {
        cameraDevice_->GetStreamOperator()->ReleaseStreams(streamIds);
    }
}

int32_t HCaptureSession::Release(pid_t pid)
{
    ClearCaptureSession(pid);
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

void HCaptureSession::DestroyStubObjectForPid(pid_t pid)
{
    MEDIA_DEBUG_LOG("camera stub services(%{public}zu) pid(%{public}d).", session_.size(), pid);
    sptr<HCaptureSession> session;

    auto it = session_.find(pid);
    if (it != session_.end()) {
        session = it->second;
        session->Release(pid);
    }
    MEDIA_DEBUG_LOG("camera stub services(%{public}zu).", session_.size());
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
                MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureStarted StreamId: %{public}d not found", *item);
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
            curStreamRepeat->OnFrameEnded(captureInfo->frameCount_);
        } else {
            curStreamCapture = GetStreamCaptureByStreamID(captureInfo->streamId_);
            if (curStreamCapture != nullptr) {
                curStreamCapture->OnCaptureEnded(captureId, captureInfo->frameCount_);
            } else {
                MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureEnded StreamId: %{public}d not found."
                              " Framecount: %{public}d", captureInfo->streamId_, captureInfo->frameCount_);
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
            curStreamRepeat->OnFrameError(errInfo->error_);
        } else {
            curStreamCapture = GetStreamCaptureByStreamID(errInfo->streamId_);
            if (curStreamCapture != nullptr) {
                curStreamCapture->OnCaptureError(captureId, errInfo->error_);
            } else {
                MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureError StreamId: %{public}d not found."
                              " Error: %{public}d", errInfo->streamId_, errInfo->error_);
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
            MEDIA_ERR_LOG("StreamOperatorCallback::OnFrameShutter StreamId: %{public}d not found", *item);
        }
    }
}

void StreamOperatorCallback::SetCaptureSession(sptr<HCaptureSession> captureSession)
{
    captureSession_ = captureSession;
}
} // namespace CameraStandard
} // namespace OHOS
