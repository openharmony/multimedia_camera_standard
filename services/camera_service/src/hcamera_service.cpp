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

#include "hcamera_service.h"

#include <securec.h>
#include <unordered_set>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "camera_util.h"
#include "iservice_registry.h"
#include "camera_log.h"
#include "system_ability_definition.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace CameraStandard {
REGISTER_SYSTEM_ABILITY_BY_ID(HCameraService, CAMERA_SERVICE_ID, true)
HCameraService::HCameraService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate),
      cameraHostManager_(nullptr),
      streamOperatorCallback_(nullptr),
      cameraServiceCallback_(nullptr)
{
}

HCameraService::~HCameraService()
{
}

void HCameraService::OnStart()
{
    if (cameraHostManager_ == nullptr) {
        cameraHostManager_ = new(std::nothrow) HCameraHostManager(this);
        if (cameraHostManager_ == nullptr) {
            MEDIA_ERR_LOG("HCameraService OnStart failed to create HCameraHostManager obj");
            return;
        }
    }
    if (cameraHostManager_->Init() != CAMERA_OK) {
        MEDIA_ERR_LOG("HCameraService OnStart failed to init camera host manager.");
    }
    bool res = Publish(this);
    if (res) {
        MEDIA_INFO_LOG("HCameraService OnStart res=%{public}d", res);
    }
}

void HCameraService::OnDump()
{
    MEDIA_INFO_LOG("HCameraService::OnDump called");
}

void HCameraService::OnStop()
{
    MEDIA_INFO_LOG("HCameraService::OnStop called");

    if (cameraHostManager_) {
        cameraHostManager_->DeInit();
        delete cameraHostManager_;
        cameraHostManager_ = nullptr;
    }
}

int32_t HCameraService::GetCameras(std::vector<std::string> &cameraIds,
    std::vector<std::shared_ptr<OHOS::Camera::CameraMetadata>> &cameraAbilityList)
{
    CAMERA_SYNC_TRACE;
    int32_t ret = cameraHostManager_->GetCameras(cameraIds);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("HCameraService::GetCameras failed");
        return ret;
    }

    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility;
    for (auto id : cameraIds) {
        ret = cameraHostManager_->GetCameraAbility(id, cameraAbility);
        if (ret != CAMERA_OK) {
            MEDIA_ERR_LOG("HCameraService::GetCameraAbility failed");
            return ret;
        }

        camera_metadata_item_t item;
        common_metadata_header_t *metadata = cameraAbility->get();
        camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_OTHER;
        int ret = OHOS::Camera::FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &item);
        if (ret == CAM_META_SUCCESS) {
            cameraPosition = static_cast<camera_position_enum_t>(item.data.u8[0]);
        }

        camera_type_enum_t cameraType = OHOS_CAMERA_TYPE_UNSPECIFIED;
        ret = OHOS::Camera::FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &item);
        if (ret == CAM_META_SUCCESS) {
            cameraType = static_cast<camera_type_enum_t>(item.data.u8[0]);
        }

        camera_connection_type_t connectionType = OHOS_CAMERA_CONNECTION_TYPE_BUILTIN;
        ret = OHOS::Camera::FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
        if (ret == CAM_META_SUCCESS) {
            connectionType = static_cast<camera_connection_type_t>(item.data.u8[0]);
        }

        bool isMirrorSupported = false;
        ret = OHOS::Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
        if (ret == CAM_META_SUCCESS) {
            isMirrorSupported = ((item.data.u8[0] == 1) || (item.data.u8[0] == 0));
        }
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CameraManager GetCameras camera ID:%s, Camera position:%d,"
                                            " Camera Type:%d, Connection Type:%d, Mirror support:%d", id.c_str(),
                                            cameraPosition, cameraType, connectionType, isMirrorSupported));
        cameraAbilityList.emplace_back(cameraAbility);
    }

    return ret;
}

int32_t HCameraService::CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService> &device)
{
    CAMERA_SYNC_TRACE;
    sptr<HCameraDevice> cameraDevice;

    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    std::string permissionName = "ohos.permission.CAMERA";

    int permission_result
        = OHOS::Security::AccessToken::TypePermissionState::PERMISSION_DENIED;
    Security::AccessToken::ATokenTypeEnum tokenType
        = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(callerToken);
    if ((tokenType == OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE)
        || (tokenType == OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_HAP)) {
        permission_result = OHOS::Security::AccessToken::AccessTokenKit::VerifyAccessToken(
            callerToken, permissionName);
    } else {
        MEDIA_ERR_LOG("HCameraService::CreateCameraDevice: Unsupported Access Token Type");
        return CAMERA_INVALID_ARG;
    }
    if (permission_result != OHOS::Security::AccessToken::TypePermissionState::PERMISSION_GRANTED) {
        MEDIA_ERR_LOG("HCameraService::CreateCameraDevice: Permission to Access Camera Denied!!!!");
        return CAMERA_ALLOC_ERROR;
    } else {
        MEDIA_DEBUG_LOG("HCameraService::CreateCameraDevice: Permission to Access Camera Granted!!!!");
    }

    cameraDevice = new(std::nothrow) HCameraDevice(cameraHostManager_, cameraId);
    if (cameraDevice == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateCameraDevice HCameraDevice allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    devices_.insert(std::make_pair(cameraId, cameraDevice));
    device = cameraDevice;
    CAMERA_SYSEVENT_STATISTIC(CreateMsg("CameraManager_CreateCameraInput CameraId:%s", cameraId.c_str()));
    return CAMERA_OK;
}

int32_t HCameraService::CreateCaptureSession(sptr<ICaptureSession> &session)
{
    CAMERA_SYNC_TRACE;
    sptr<HCaptureSession> captureSession;
    if (streamOperatorCallback_ == nullptr) {
        streamOperatorCallback_ = new(std::nothrow) StreamOperatorCallback();
        if (streamOperatorCallback_ == nullptr) {
            MEDIA_ERR_LOG("HCameraService::CreateCaptureSession streamOperatorCallback_ allocation failed");
            return CAMERA_ALLOC_ERROR;
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    OHOS::Security::AccessToken::AccessTokenID callerToken = IPCSkeleton::GetCallingTokenID();
    captureSession = new(std::nothrow) HCaptureSession(cameraHostManager_, streamOperatorCallback_, callerToken);
    if (captureSession == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateCaptureSession HCaptureSession allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    session = captureSession;
    return CAMERA_OK;
}

int32_t HCameraService::CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                          sptr<IStreamCapture> &photoOutput)
{
    CAMERA_SYNC_TRACE;
    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePhotoOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    sptr<HStreamCapture> streamCapture = new(std::nothrow) HStreamCapture(producer, format);
    if (streamCapture == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePhotoOutput HStreamCapture allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    POWERMGR_SYSEVENT_CAMERA_CONFIG(PHOTO, producer->GetDefaultWidth(),
                                    producer->GetDefaultHeight());
    photoOutput = streamCapture;
    return CAMERA_OK;
}

int32_t HCameraService::CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                            sptr<IStreamRepeat> &previewOutput)
{
    CAMERA_SYNC_TRACE;
    sptr<HStreamRepeat> streamRepeatPreview;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePreviewOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatPreview = new(std::nothrow) HStreamRepeat(producer, format);
    if (streamRepeatPreview == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePreviewOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    POWERMGR_SYSEVENT_CAMERA_CONFIG(PREVIEW, producer->GetDefaultWidth(),
                                    producer->GetDefaultHeight());
    previewOutput = streamRepeatPreview;
    return CAMERA_OK;
}

int32_t HCameraService::CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                                  int32_t width, int32_t height, sptr<IStreamRepeat> &previewOutput)
{
    CAMERA_SYNC_TRACE;
    sptr<HStreamRepeat> streamRepeatPreview;

    if ((producer == nullptr) || (width == 0) || (height == 0)) {
        MEDIA_ERR_LOG("HCameraService::CreateCustomPreviewOutput producer is null or invalid custom size is set");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatPreview = new(std::nothrow) HStreamRepeat(producer, format, width, height);
    if (streamRepeatPreview == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateCustomPreviewOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    POWERMGR_SYSEVENT_CAMERA_CONFIG(PREVIEW, width, height);
    previewOutput = streamRepeatPreview;
    return CAMERA_OK;
}

int32_t HCameraService::CreateMetadataOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                             sptr<IStreamMetadata> &metadataOutput)
{
    CAMERA_SYNC_TRACE;
    sptr<HStreamMetadata> streamMetadata;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateMetadataOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamMetadata = new(std::nothrow) HStreamMetadata(producer, format);
    if (streamMetadata == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateMetadataOutput HStreamMetadata allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    POWERMGR_SYSEVENT_CAMERA_CONFIG(METADATA, producer->GetDefaultWidth(),
                                    producer->GetDefaultHeight());
    metadataOutput = streamMetadata;
    return CAMERA_OK;
}

int32_t HCameraService::CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                          sptr<IStreamRepeat> &videoOutput)
{
    CAMERA_SYNC_TRACE;
    sptr<HStreamRepeat> streamRepeatVideo;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateVideoOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatVideo = new(std::nothrow) HStreamRepeat(producer, format, true);
    if (streamRepeatVideo == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateVideoOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    POWERMGR_SYSEVENT_CAMERA_CONFIG(VIDEO, producer->GetDefaultWidth(),
                                    producer->GetDefaultHeight());
    videoOutput = streamRepeatVideo;
    return CAMERA_OK;
}

void HCameraService::OnCameraStatus(const std::string& cameraId, CameraStatus status)
{
    if (cameraServiceCallback_) {
        cameraServiceCallback_->OnCameraStatusChanged(cameraId, status);
        CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("OnCameraStatusChanged! for cameraId:%s, current Camera Status:%d",
                                           cameraId.c_str(), status));
    }
}

void HCameraService::OnFlashlightStatus(const std::string& cameraId, FlashStatus status)
{
    if (cameraServiceCallback_) {
        cameraServiceCallback_->OnFlashlightStatusChanged(cameraId, status);
        CAMERA_SYSEVENT_BEHAVIOR(CreateMsg("OnFlashlightStatusChanged! for cameraId:%s, current Flash Status:%d",
                                           cameraId.c_str(), status));
        POWERMGR_SYSEVENT_TORCH_STATE(IPCSkeleton::GetCallingPid(),
                                      IPCSkeleton::GetCallingUid(), status);
    }
}

int32_t HCameraService::SetCallback(sptr<ICameraServiceCallback> &callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("HCameraService::SetCallback callback is null");
        return CAMERA_INVALID_ARG;
    }
    cameraServiceCallback_ = callback;
    return CAMERA_OK;
}

void HCameraService::CameraSummary(std::vector<std::string> cameraIds,
    std::string& dumpString)
{
    dumpString += "# Number of Cameras:[" + std::to_string(cameraIds.size()) + "]:\n";
    dumpString += "# Number of Active Cameras:[" + std::to_string(devices_.size()) + "]:\n";
    HCaptureSession::CameraSessionSummary(dumpString);
}

void HCameraService::CameraDumpAbility(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    dumpString += "    ## Camera Ability List: \n";

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_CAMERA_POSITION, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            g_cameraPos.find(item.data.u8[0]);
        if (iter != g_cameraPos.end()) {
            dumpString += "        Camera Position:["
                + iter->second
                + "]:    ";
        }
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_CAMERA_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            g_cameraType.find(item.data.u8[0]);
        if (iter != g_cameraType.end()) {
            dumpString += "Camera Type:["
                + iter->second
                + "]:    ";
        }
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            g_cameraConType.find(item.data.u8[0]);
        if (iter != g_cameraConType.end()) {
            dumpString += "Camera Connection Type:["
                + iter->second
                + "]:\n";
        }
    }
}

void HCameraService::CameraDumpStreaminfo(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    constexpr uint32_t unitLen = 3;
    uint32_t widthOffset = 1;
    uint32_t heightOffset = 2;
    dumpString += "        ### Camera Available stream configuration List: \n";
    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry,
                                               OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret == CAM_META_SUCCESS) {
        dumpString += "            Number Stream Info: "
            + std::to_string(item.count/unitLen) + "\n";
        for (uint32_t index = 0; index < item.count; index += unitLen) {
            std::map<int, std::string>::const_iterator iter =
                g_cameraFormat.find(item.data.i32[index]);
            if (iter != g_cameraFormat.end()) {
                dumpString += "            Format:["
                        + iter->second
                        + "]:    ";
                dumpString += "Size:[Width:"
                        + std::to_string(item.data.i32[index + widthOffset])
                        + " Height:"
                        + std::to_string(item.data.i32[index + heightOffset])
                        + "]:\n";
            }
        }
    }
}

void HCameraService::CameraDumpZoom(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    dumpString += "    ## Zoom Related Info: \n";
    camera_metadata_item_t item;
    int ret;
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    uint32_t zoomRangeCount = 2;
    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_ZOOM_CAP, &item);
    if ((ret == CAM_META_SUCCESS) && (item.count == zoomRangeCount)) {
        dumpString += "        Available Zoom Capability:["
            + std::to_string(item.data.i32[minIndex]) + "  "
            + std::to_string(item.data.i32[maxIndex])
            + "]:\n";
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_SCENE_ZOOM_CAP, &item);
    if ((ret == CAM_META_SUCCESS) && (item.count == zoomRangeCount)) {
        dumpString += "        Available scene Zoom Capability:["
            + std::to_string(item.data.i32[minIndex]) + "  "
            + std::to_string(item.data.i32[maxIndex])
            + "]:\n";
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_ZOOM_RATIO_RANGE, &item);
    if ((ret == CAM_META_SUCCESS) && (item.count == zoomRangeCount)) {
        dumpString += "        Available Zoom Ratio Range:["
            + std::to_string(item.data.f[minIndex])
            + std::to_string(item.data.f[maxIndex])
            + "]:\n";
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_ZOOM_RATIO, &item);
    if (ret == CAM_META_SUCCESS) {
        dumpString += "        Set Zoom Ratio:["
            + std::to_string(item.data.f[0])
            + "]:\n";
    }
}

void HCameraService::CameraDumpFlash(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    dumpString += "    ## Flash Related Info: \n";
    dumpString += "        Available Flash Modes:[";
    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_FLASH_MODES, &item);
    if (ret == CAM_META_SUCCESS) {
        for (uint32_t i = 0; i < item.count; i++) {
            std::map<int, std::string>::const_iterator iter =
                g_cameraFlashMode.find(item.data.u8[i]);
            if (iter != g_cameraFlashMode.end()) {
                dumpString += " " + iter->second;
            }
        }
        dumpString += "]:\n";
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_FLASH_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            g_cameraFlashMode.find(item.data.u8[0]);
        if (iter != g_cameraFlashMode.end()) {
            dumpString += "        Set Flash Mode:["
                + iter->second
                + "]:\n";
        }
    }
}

void HCameraService::CameraDumpAF(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    dumpString += "    ## AF Related Info: \n";
    dumpString += "        Available Focus Modes:[";

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_FOCUS_MODES, &item);
    if (ret == CAM_META_SUCCESS) {
        for (uint32_t i = 0; i < item.count; i++) {
            std::map<int, std::string>::const_iterator iter =
                g_cameraFocusMode.find(item.data.u8[i]);
            if (iter != g_cameraFocusMode.end()) {
                dumpString += " " + iter->second;
            }
        }
        dumpString += "]:\n";
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            g_cameraFocusMode.find(item.data.u8[0]);
        if (iter != g_cameraFocusMode.end()) {
            dumpString += "        Set Focus Mode:["
                + iter->second
                + "]:\n";
        }
    }
}

void HCameraService::CameraDumpAE(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    dumpString += "    ## AE Related Info: \n";
    dumpString += "        Available Exposure Modes:[";

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_EXPOSURE_MODES, &item);
    if (ret == CAM_META_SUCCESS) {
        for (uint32_t i = 0; i < item.count; i++) {
            std::map<int, std::string>::const_iterator iter =
                g_cameraExposureMode.find(item.data.u8[i]);
            if (iter != g_cameraExposureMode.end()) {
                dumpString += " " + iter->second;
            }
        }
        dumpString += "]:\n";
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            g_cameraExposureMode.find(item.data.u8[0]);
        if (iter != g_cameraExposureMode.end()) {
            dumpString += "        Set exposure Mode:["
                + iter->second
                + "]:\n";
        }
    }
}

void HCameraService::CameraDumpSensorInfo(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    dumpString += "    ## Sensor Info Info: \n";
    int32_t leftIndex = 0;
    int32_t topIndex = 1;
    int32_t rightIndex = 2;
    int32_t bottomIndex = 3;
    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &item);
    if (ret == CAM_META_SUCCESS) {
        dumpString += "        Array:["
            + std::to_string(item.data.i32[leftIndex]) + " "
            + std::to_string(item.data.i32[topIndex]) + " "
            + std::to_string(item.data.i32[rightIndex]) + " "
            + std::to_string(item.data.i32[bottomIndex])
            + "]:\n";
    }
}

void HCameraService::CameraDumpVideoStabilization(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    dumpString += "    ## Video Stabilization Related Info: \n";
    dumpString += "        Available Video Stabilization Modes:[";

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_VIDEO_STABILIZATION_MODES, &item);
    if (ret == CAM_META_SUCCESS) {
        for (uint32_t i = 0; i < item.count; i++) {
            std::map<int, std::string>::const_iterator iter =
                g_cameraVideoStabilizationMode.find(item.data.u8[i]);
            if (iter != g_cameraVideoStabilizationMode.end()) {
                dumpString += " " + iter->second;
            }
        }
        dumpString += "]:\n";
    }

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_VIDEO_STABILIZATION_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            g_cameraVideoStabilizationMode.find(item.data.u8[0]);
        if (iter != g_cameraVideoStabilizationMode.end()) {
            dumpString += "        Set Stabilization Mode:["
                + iter->second
                + "]:\n";
        }
    }
}

void HCameraService::CameraDumpVideoFrameRateRange(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    const int32_t FRAME_RATE_RANGE_STEP = 2;
    int ret;
    dumpString += "    ## Video FrameRateRange Related Info: \n";
    dumpString += "        Available FrameRateRange :\n";

    ret = OHOS::Camera::FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_FPS_RANGES, &item);
    if (ret == CAM_META_SUCCESS) {
        for (uint32_t i = 0; i < (item.count - 1); i += FRAME_RATE_RANGE_STEP) {
            dumpString += "            [ " + std::to_string(item.data.i32[i]) + ", " +
                          std::to_string(item.data.i32[i+1]) + " ]\n";
        }
        dumpString += "\n";
    }
}

int32_t HCameraService::Dump(int fd, const std::vector<std::u16string>& args)
{
    std::unordered_set<std::u16string> argSets;
    std::u16string arg1(u"summary");
    std::u16string arg2(u"ability");
    std::u16string arg3(u"clientwiseinfo");
    for (decltype(args.size()) index = 0; index < args.size(); ++index) {
        argSets.insert(args[index]);
    }
    std::string dumpString;
    std::vector<std::string> cameraIds;
    std::vector<std::shared_ptr<OHOS::Camera::CameraMetadata>> cameraAbilityList;
    int32_t capIdx = 0;
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata;
    int ret;

    ret = GetCameras(cameraIds, cameraAbilityList);
    if ((ret != CAMERA_OK) || cameraIds.empty()
        || (cameraAbilityList.empty())) {
        return CAMERA_INVALID_STATE;
    }
    if (args.size() == 0 || argSets.count(arg1) != 0) {
        dumpString += "-------- Summary -------\n";
        CameraSummary(cameraIds, dumpString);
    }
    if (args.size() == 0 || argSets.count(arg2) != 0) {
        dumpString += "-------- CameraInfo -------\n";
        for (auto& it : cameraIds) {
            metadata = cameraAbilityList[capIdx++];
            common_metadata_header_t *metadataEntry = metadata->get();
            dumpString += "# Camera ID:[" + it + "]: \n";
            CameraDumpAbility(metadataEntry, dumpString);
            CameraDumpStreaminfo(metadataEntry, dumpString);
            CameraDumpZoom(metadataEntry, dumpString);
            CameraDumpFlash(metadataEntry, dumpString);
            CameraDumpAF(metadataEntry, dumpString);
            CameraDumpAE(metadataEntry, dumpString);
            CameraDumpSensorInfo(metadataEntry, dumpString);
            CameraDumpVideoStabilization(metadataEntry, dumpString);
            CameraDumpVideoFrameRateRange(metadataEntry, dumpString);
        }
    }
    if (args.size() == 0 || argSets.count(arg3) != 0) {
        dumpString += "-------- Clientwise Info -------\n";
        HCaptureSession::dumpSessions(dumpString);
    }

    if (dumpString.size() == 0) {
        MEDIA_ERR_LOG("Dump string empty!");
        return CAMERA_INVALID_STATE;
    }

    (void)write(fd, dumpString.c_str(), dumpString.size());
    return CAMERA_OK;
}
} // namespace CameraStandard
} // namespace OHOS
