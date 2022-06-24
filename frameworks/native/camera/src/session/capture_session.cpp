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
#include "ipc_skeleton.h"
#include "camera_log.h"
#include "output/photo_output.h"
#include "output/preview_output.h"
#include "output/video_output.h"
#include "bundle_mgr_interface.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::AAFwk;

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
            CAMERA_SYSEVENT_FAULT(CreateMsg("Session OnError! errorCode:%d", errorCode));
            captureSession_->GetApplicationCallback()->OnError(errorCode);
        } else {
            MEDIA_INFO_LOG("CaptureSessionCallback::ApplicationCallback not set!, Discarding callback");
        }
        return CAMERA_OK;
    }
};

static std::string GetClientBundle(int uid)
{
    std::string bundleName = "";
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        MEDIA_ERR_LOG("Get ability manager failed");
        return bundleName;
    }

    sptr<IRemoteObject> object = samgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (object == nullptr) {
        MEDIA_DEBUG_LOG("object is NULL.");
        return bundleName;
    }

    sptr<AppExecFwk::IBundleMgr> bms = iface_cast<AppExecFwk::IBundleMgr>(object);
    if (bms == nullptr) {
        MEDIA_DEBUG_LOG("bundle manager service is NULL.");
        return bundleName;
    }

    auto result = bms->GetBundleNameForUid(uid, bundleName);
    if (!result) {
        MEDIA_ERR_LOG("GetBundleNameForUid fail");
        return "";
    }
    MEDIA_INFO_LOG("bundle name is %{public}s ", bundleName.c_str());

    return bundleName;
}

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
    if (inputDevice_ != nullptr) {
        int32_t pid = IPCSkeleton::GetCallingPid();
        int32_t uid = IPCSkeleton::GetCallingUid();
        POWERMGR_SYSEVENT_CAMERA_CONNECT(pid, uid,
                                         inputDevice_->GetCameraDeviceInfo()->GetID().c_str(),
                                         GetClientBundle(uid));
    }

    return captureSession_->CommitConfig();
}

int32_t CaptureSession::AddInput(sptr<CaptureInput> &input)
{
    CAMERA_SYNC_TRACE;
    if (input == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::AddInput input is null");
        return CAMERA_INVALID_ARG;
    }
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddInput"));
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
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddOutput with %s", output->GetOutputTypeString()));
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
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemoveInput"));
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
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemoveOutput with %s", output->GetOutputTypeString()));
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

std::shared_ptr<SessionCallback> CaptureSession::GetApplicationCallback()
{
    return appCallback_;
}

void CaptureSession::Release()
{
    CAMERA_SYNC_TRACE;
    if (inputDevice_ != nullptr) {
        POWERMGR_SYSEVENT_CAMERA_DISCONNECT(inputDevice_->GetCameraDeviceInfo()->GetID().c_str());
        inputDevice_ = nullptr;
    }
    int32_t errCode = captureSession_->Release(0);
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to Release capture session!, %{public}d", errCode);
    }
}
} // CameraStandard
} // OHOS
