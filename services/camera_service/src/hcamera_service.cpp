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
#include "hcamera_service.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"

#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

namespace OHOS {
namespace CameraStandard {
REGISTER_SYSTEM_ABILITY_BY_ID(HCameraService, CAMERA_SERVICE_ID, true)

void HCameraService::InitMapInfo()
{
    cameraPos_.insert(std::make_pair(0, "Front"));
    cameraPos_.insert(std::make_pair(1, "Back"));
    cameraPos_.insert(std::make_pair(2, "Other"));

    cameraType_.insert(std::make_pair(0, "Wide-Angle"));
    cameraType_.insert(std::make_pair(1, "Ultra-Wide"));
    cameraType_.insert(std::make_pair(2, "TelePhoto"));
    cameraType_.insert(std::make_pair(3, "TrueDepth"));
    cameraType_.insert(std::make_pair(4, "Logical"));
    cameraType_.insert(std::make_pair(5, "Unspecified"));

    cameraConType_.insert(std::make_pair(0, "Builtin"));
    cameraConType_.insert(std::make_pair(1, "USB-Plugin"));
    cameraConType_.insert(std::make_pair(2, "Remote"));

    cameraFormat_.insert(std::make_pair(1, "RGBA_8888"));
    cameraFormat_.insert(std::make_pair(2, "YCBCR_420_888"));
    cameraFormat_.insert(std::make_pair(3, "YCRCB_420_SP"));
    cameraFormat_.insert(std::make_pair(4, "JPEG"));

    cameraFocusMode_.insert(std::make_pair(0, "Manual"));
    cameraFocusMode_.insert(std::make_pair(1, "Continuous-Auto"));
    cameraFocusMode_.insert(std::make_pair(2, "Auto"));
    cameraFocusMode_.insert(std::make_pair(3, "Locked"));

    cameraFlashMode_.insert(std::make_pair(0, "Close"));
    cameraFlashMode_.insert(std::make_pair(1, "Open"));
    cameraFlashMode_.insert(std::make_pair(2, "Auto"));
    cameraFlashMode_.insert(std::make_pair(3, "Always-Open"));
}

HCameraService::HCameraService(int32_t systemAbilityId, bool runOnCreate) : SystemAbility(systemAbilityId, runOnCreate)
{
    cameraHostManager_ = new HCameraHostManager();
    if (cameraHostManager_ == nullptr) {
        MEDIA_ERR_LOG("HCameraService:HCameraHostManager creation is failed");
        return;
    }
    InitMapInfo();
}

HCameraService::~HCameraService()
{}

void HCameraService::OnStart()
{
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
}

int32_t HCameraService::GetCameras(std::vector<std::string> &cameraIds,
    std::vector<std::shared_ptr<CameraMetadata>> &cameraAbilityList)
{
    int32_t ret = cameraHostManager_->GetCameras(cameraIds);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("HCameraService::GetCameras failed");
        return ret;
    }

    std::shared_ptr<CameraMetadata> cameraAbility;
    for (auto id : cameraIds) {
        ret = cameraHostManager_->GetCameraAbility(id, cameraAbility);
        if (ret != CAMERA_OK) {
            MEDIA_ERR_LOG("HCameraService::GetCameraAbility failed");
            return ret;
        }
        cameraAbilityList.emplace_back(cameraAbility);
    }

    return ret;
}

int32_t HCameraService::CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService> &device)
{
    int32_t rc;
    sptr<HCameraDevice> cameraDevice;
    sptr<Camera::IStreamOperator> streamOperator;

    if (streamOperatorCallback_ == nullptr) {
        streamOperatorCallback_ = new StreamOperatorCallback();
    }
    if (cameraDeviceCallback_ == nullptr) {
        cameraDeviceCallback_ = new CameraDeviceCallback();
    }
    cameraDevice = new HCameraDevice(cameraHostManager_, cameraDeviceCallback_, cameraId);
    if (cameraDevice == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateCameraDevice HCameraDevice allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    rc = cameraDevice->Open();
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::CreateCameraDevice Failed to open camera with return: %{public}d", rc);
        delete cameraDevice;
        return rc;
    }
    rc = cameraDevice->GetStreamOperator(streamOperatorCallback_, streamOperator);
    if (rc != CAMERA_OK) {
        MEDIA_ERR_LOG("HCaptureSession::CreateCameraDevice Failed to get stream operator with return: %{public}d", rc);
        cameraDevice->Close();
        delete cameraDevice;
        return rc;
    }
    device = cameraDevice;
    return CAMERA_OK;
}

int32_t HCameraService::CreateCaptureSession(sptr<ICaptureSession> &session)
{
    sptr<HCaptureSession> captureSession;

    if (streamOperatorCallback_ == nullptr) {
        streamOperatorCallback_ = new StreamOperatorCallback();
    }

    captureSession = new HCaptureSession(cameraHostManager_, streamOperatorCallback_);
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
    sptr<HStreamCapture> streamCapture;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePhotoOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamCapture = new HStreamCapture(producer, format);
    if (streamCapture == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePhotoOutput HStreamCapture allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    photoOutput = streamCapture;
    return CAMERA_OK;
}

int32_t HCameraService::CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                            sptr<IStreamRepeat> &previewOutput)
{
    sptr<HStreamRepeat> streamRepeatPreview;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePreviewOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatPreview = new HStreamRepeat(producer, format);
    if (streamRepeatPreview == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreatePreviewOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    previewOutput = streamRepeatPreview;
    return CAMERA_OK;
}

int32_t HCameraService::CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                                  int32_t width, int32_t height, sptr<IStreamRepeat> &previewOutput)
{
    sptr<HStreamRepeat> streamRepeatPreview;

    if (producer == nullptr || width == 0 || height == 0) {
        MEDIA_ERR_LOG("HCameraService::CreateCustomPreviewOutput producer is null or invalid custom size is set");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatPreview = new HStreamRepeat(producer, format, width, height);
    if (streamRepeatPreview == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateCustomPreviewOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    previewOutput = streamRepeatPreview;
    return CAMERA_OK;
}

int32_t HCameraService::CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                          sptr<IStreamRepeat> &videoOutput)
{
    sptr<HStreamRepeat> streamRepeatVideo;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateVideoOutput producer is null");
        return CAMERA_INVALID_ARG;
    }
    streamRepeatVideo = new HStreamRepeat(producer, format, true);
    if (streamRepeatVideo == nullptr) {
        MEDIA_ERR_LOG("HCameraService::CreateVideoOutput HStreamRepeat allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    videoOutput = streamRepeatVideo;
    return CAMERA_OK;
}

CameraHostCallback::CameraHostCallback(sptr<ICameraServiceCallback> &callback)
{
    cameraServiceCallback = callback;
}

void CameraHostCallback::OnCameraStatus(const std::string &cameraId, Camera::CameraStatus status)
{
    CameraStatus svcStatus = CAMERA_STATUS_UNAVAILABLE;

    if (cameraServiceCallback != nullptr) {
        switch (status) {
            case Camera::UN_AVAILABLE:
                svcStatus = CAMERA_STATUS_UNAVAILABLE;
                break;

            case Camera::AVAILABLE:
                svcStatus = CAMERA_STATUS_AVAILABLE;
                break;

            default:
                MEDIA_ERR_LOG("Unknown camera status: %{public}d", status);
                return;
        }
        cameraServiceCallback->OnCameraStatusChanged(cameraId, svcStatus);
    }
}

void CameraHostCallback::OnFlashlightStatus(const std::string &cameraId, Camera::FlashlightStatus status)
{
    FlashStatus flashStaus = FLASH_STATUS_OFF;

    if (cameraServiceCallback != nullptr) {
        switch (status) {
            case Camera::FLASHLIGHT_OFF:
                flashStaus = FLASH_STATUS_OFF;
                break;

            case Camera::FLASHLIGHT_ON:
                flashStaus = FLASH_STATUS_ON;
                break;

            case Camera::FLASHLIGHT_UNAVAILABLE:
                flashStaus = FLASH_STATUS_UNAVAILABLE;
                break;

            default:
                MEDIA_ERR_LOG("Unknown flashlight status: %{public}d", status);
                return;
        }
        cameraServiceCallback->OnFlashlightStatusChanged(cameraId, flashStaus);
    }
}

#ifdef BALTIMORE_CAMERA
void CameraHostCallback::OnCameraEvent(const std::string &cameraId, Camera::CameraEvent event)
{
    MEDIA_INFO_LOG("CameraHostCallback::OnCameraEvent called");
    switch (event) {
        case Camera::CAMERA_EVENT_DEVICE_ADD:
            MEDIA_INFO_LOG("CAMERA_EVENT_DEVICE_ADD event with cameraID:%{public}s", cameraId.c_str());
            break;

        case Camera::CAMERA_EVENT_DEVICE_RMV:
            MEDIA_INFO_LOG("CAMERA_EVENT_DEVICE_RMV event with cameraID:%{public}s", cameraId.c_str());
            break;

        default:
            MEDIA_ERR_LOG("Unknown Camera event: %{public}d", event);
            return;
    }
}
#endif

int32_t HCameraService::SetCallback(sptr<ICameraServiceCallback> &callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("HCameraService::SetCallback callback is null");
        return CAMERA_INVALID_ARG;
    }
    cameraHostCallback_ = new CameraHostCallback(callback);
    if (cameraHostCallback_ == nullptr) {
        MEDIA_ERR_LOG("HCameraService::SetCallback CameraHostCallback allocation failed");
        return CAMERA_ALLOC_ERROR;
    }
    int ret = cameraHostManager_->SetCallback(cameraHostCallback_);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("Setting cameraHostCallback failed");
        return ret;
    }
    return CAMERA_OK;
}

void HCameraService::CameraDumpAbility(common_metadata_header_t *metadataEntry,
    std::string& dumpString)
{
    camera_metadata_item_t item;
    int ret;
    dumpString += "    ## Camera Ability List: \n";

    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_CAMERA_POSITION, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            cameraPos_.find(item.data.u8[0]);
        if (iter != cameraPos_.end()) {
            dumpString += "        Camera Position:["
                + iter->second
                + "]:    ";
        }
    }

    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_CAMERA_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            cameraType_.find(item.data.u8[0]);
        if (iter != cameraType_.end()) {
            dumpString += "Camera Type:["
                + iter->second
                + "]:    ";
        }
    }

    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            cameraConType_.find(item.data.u8[0]);
        if (iter != cameraConType_.end()) {
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
    int32_t unitLen = 3;
    int32_t widthOffset = 1;
    int32_t heightOffset = 2;
    dumpString += "        ### Camera Available stream configuration List: \n";
    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    if (ret == CAM_META_SUCCESS) {
        dumpString += "            Number Stream Info: "
            + std::to_string(item.count/unitLen) + "\n";
        for (uint32_t index = 0; index < item.count; index += unitLen) {
            std::map<int, std::string>::const_iterator iter =
            cameraFormat_.find(item.data.i32[index]);
            if (iter != cameraFormat_.end()) {
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
    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_ZOOM_CAP, &item);
    if ((ret == CAM_META_SUCCESS) && (item.count == zoomRangeCount)) {
        dumpString += "        Available Zoom Capability:["
            + std::to_string(item.data.i32[minIndex]) + "  "
            + std::to_string(item.data.i32[maxIndex])
            + "]:\n";
    }

    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_SCENE_ZOOM_CAP, &item);
    if ((ret == CAM_META_SUCCESS) && (item.count == zoomRangeCount)) {
        dumpString += "        Available scene Zoom Capability:["
            + std::to_string(item.data.i32[minIndex]) + "  "
            + std::to_string(item.data.i32[maxIndex])
            + "]:\n";
    }

    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_ZOOM_RATIO_RANGE, &item);
    if ((ret == CAM_META_SUCCESS) && (item.count == zoomRangeCount)) {
        dumpString += "        Available Zoom Ratio Range:["
            + std::to_string(item.data.f[minIndex])
            + std::to_string(item.data.f[maxIndex])
            + "]:\n";
    }

    ret = FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_ZOOM_RATIO, &item);
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
    ret = FindCameraMetadataItem(metadataEntry, OHOS_ABILITY_DEVICE_AVAILABLE_FLASHMODES, &item);
    if (ret == CAM_META_SUCCESS) {
        for (uint32_t i = 0; i < item.count; i++) {
            std::map<int, std::string>::const_iterator iter =
                cameraFlashMode_.find(item.data.u8[i]);
            if (iter != cameraFlashMode_.end()) {
                dumpString += " " + iter->second;
            }
        }
        dumpString += "]:\n";
    }

    ret = FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_FLASHMODE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            cameraFlashMode_.find(item.data.u8[0]);
        if (iter != cameraFlashMode_.end()) {
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

    ret = FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_AF_AVAILABLE_MODES, &item);
    if (ret == CAM_META_SUCCESS) {
        for (uint32_t i = 0; i < item.count; i++) {
            std::map<int, std::string>::const_iterator iter =
                cameraFocusMode_.find(item.data.u8[i]);
            if (iter != cameraFocusMode_.end()) {
                dumpString += " " + iter->second;
            }
        }
        dumpString += "]:\n";
    }

    ret = FindCameraMetadataItem(metadataEntry, OHOS_CONTROL_AF_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        std::map<int, std::string>::const_iterator iter =
            cameraFocusMode_.find(item.data.u8[0]);
        if (iter != cameraFocusMode_.end()) {
            dumpString += "        Set Focus Mode:["
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
    ret = FindCameraMetadataItem(metadataEntry, OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &item);
    if (ret == CAM_META_SUCCESS) {
        dumpString += "        Array:["
            + std::to_string(item.data.i32[leftIndex]) + " "
            + std::to_string(item.data.i32[topIndex]) + " "
            + std::to_string(item.data.i32[rightIndex]) + " "
            + std::to_string(item.data.i32[bottomIndex])
            + "]:\n";
    }
}

int32_t HCameraService::Dump(int fd, const std::vector<std::u16string>& args)
{
    std::unordered_set<std::u16string> argSets;
    std::u16string arg1(u"ability");
    std::u16string arg2(u"streaminfo");
    std::u16string arg3(u"zoom");
    std::u16string arg4(u"flash");
    std::u16string arg5(u"af");
    std::u16string arg6(u"sensorinfo");
    for (decltype(args.size()) index = 0; index < args.size(); ++index) {
        argSets.insert(args[index]);
    }
    std::string dumpString;

    dumpString += "------- CameraInfo ------- \n";

    std::vector<std::string> cameraIds;
    std::vector<std::shared_ptr<CameraMetadata>> cameraAbilityList;
    int32_t capIdx = 0;
    std::shared_ptr<CameraMetadata> metadata;
    int ret;

    ret = GetCameras(cameraIds, cameraAbilityList);
    if ((ret != CAMERA_OK) || cameraIds.empty()
        || (cameraAbilityList.empty())) {
        return CAMERA_INVALID_STATE;
    }

    for (auto& it : cameraIds) {
        metadata = cameraAbilityList[capIdx++];
        common_metadata_header_t *metadataEntry = metadata->get();

        dumpString += "# Camera ID:[" + it + "]: \n";

        if (args.size() == 0 || argSets.count(arg1) != 0) {
            CameraDumpAbility(metadataEntry, dumpString);
        }

        if (args.size() == 0 || argSets.count(arg2) != 0) {
            CameraDumpStreaminfo(metadataEntry, dumpString);
        }

        if (args.size() == 0 || argSets.count(arg3) != 0) {
            CameraDumpZoom(metadataEntry, dumpString);
        }

        if (args.size() == 0 || argSets.count(arg4) != 0) {
            CameraDumpFlash(metadataEntry, dumpString);
        }

        if (args.size() == 0 || argSets.count(arg5) != 0) {
            CameraDumpAF(metadataEntry, dumpString);
        }

        if (args.size() == 0 || argSets.count(arg6) != 0) {
            CameraDumpSensorInfo(metadataEntry, dumpString);
        }
    }
    if (dumpString.size() == 0) {
        MEDIA_ERR_LOG("Dump string empty!");
        return CAMERA_INVALID_STATE;
    }

    write(fd, dumpString.c_str(), dumpString.size());
    return CAMERA_OK;
}
} // namespace CameraStandard
} // namespace OHOS
