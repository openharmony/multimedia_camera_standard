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

#include "hcamera_service_proxy.h"
#include "media_log.h"
#include "metadata_utils.h"
#include "remote_request_code.h"

namespace OHOS {
namespace CameraStandard {
HCameraServiceProxy::HCameraServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ICameraService>(impl) { }

int32_t HCameraServiceProxy::GetCameras(std::vector<std::string> &cameraIds,
    std::vector<std::shared_ptr<CameraMetadata>> &cameraAbilityList)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    constexpr int32_t MAX_SUPPORTED_CAMERAS = 100;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy GetCameras Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_SERVICE_GET_CAMERAS, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy GetCameras failed, error: %{public}d", error);
        return error;
    }

    if (!reply.ReadStringVector(&cameraIds)) {
        MEDIA_ERR_LOG("HCameraServiceProxy GetCameras ReadStringVector failed");
        error = IPC_PROXY_ERR;
    }

    int32_t count = reply.ReadInt32();
    if ((cameraIds.size() != static_cast<uint32_t>(count)) || (count > MAX_SUPPORTED_CAMERAS)) {
        MEDIA_ERR_LOG("HCameraServiceProxy GetCameras Malformed camera count value");
        return IPC_PROXY_ERR;
    }

    std::shared_ptr<CameraMetadata> cameraAbility;
    for (int i = 0; i < count; i++) {
        MetadataUtils::DecodeCameraMetadata(reply, cameraAbility);
        cameraAbilityList.emplace_back(cameraAbility);
    }

    return error;
}

int32_t HCameraServiceProxy::CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService> &device)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCameraDevice Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteString(cameraId)) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCameraDevice Write CameraId failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_SERVICE_CREATE_DEVICE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCameraDevice failed, error: %{public}d", error);
        return error;
    }

    auto remoteObject = reply.ReadRemoteObject();
    if (remoteObject != nullptr) {
        device = iface_cast<ICameraDeviceService>(remoteObject);
    } else {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCameraDevice CameraDevice is null");
        error = IPC_PROXY_ERR;
    }

    return error;
}

int32_t HCameraServiceProxy::SetCallback(sptr<ICameraServiceCallback>& callback)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (callback == nullptr) {
        MEDIA_ERR_LOG("HCameraServiceProxy SetCallback callback is null");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy SetCallback Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteRemoteObject(callback->AsObject())) {
        MEDIA_ERR_LOG("HCameraServiceProxy SetCallback write CameraServiceCallback obj failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_SERVICE_SET_CALLBACK, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy SetCallback failed, error: %{public}d", error);
    }

    return error;
}

int32_t HCameraServiceProxy::CreateCaptureSession(sptr<ICaptureSession>& session)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCaptureSession Write interface token failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_SERVICE_CREATE_CAPTURE_SESSION, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCaptureSession failed, error: %{public}d", error);
        return error;
    }

    auto remoteObject = reply.ReadRemoteObject();
    if (remoteObject != nullptr) {
        session = iface_cast<ICaptureSession>(remoteObject);
    } else {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCaptureSession CaptureSession is null");
        error = IPC_PROXY_ERR;
    }

    return error;
}

int32_t HCameraServiceProxy::CreatePhotoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
    sptr<IStreamCapture>& photoOutput)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePhotoOutput producer is null");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePhotoOutput Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteRemoteObject(producer->AsObject())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePhotoOutput write producer obj failed");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInt32(format)) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePhotoOutput Write format failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_SERVICE_CREATE_PHOTO_OUTPUT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePhotoOutput failed, error: %{public}d", error);
        return error;
    }

    auto remoteObject = reply.ReadRemoteObject();
    if (remoteObject != nullptr) {
        photoOutput = iface_cast<IStreamCapture>(remoteObject);
    } else {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePhotoOutput photoOutput is null");
        error = IPC_PROXY_ERR;
    }

    return error;
}

int32_t HCameraServiceProxy::CreatePreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
    sptr<IStreamRepeat>& previewOutput)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePreviewOutput producer is null");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePreviewOutput Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteRemoteObject(producer->AsObject())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePreviewOutput write producer obj failed");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInt32(format)) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePreviewOutput Write format failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_SERVICE_CREATE_PREVIEW_OUTPUT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePreviewOutput failed, error: %{public}d", error);
        return error;
    }

    auto remoteObject = reply.ReadRemoteObject();
    if (remoteObject != nullptr) {
        previewOutput = iface_cast<IStreamRepeat>(remoteObject);
    } else {
        MEDIA_ERR_LOG("HCameraServiceProxy CreatePreviewOutput previewOutput is null");
        error = IPC_PROXY_ERR;
    }

    return error;
}

int32_t HCameraServiceProxy::CreateCustomPreviewOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                                       int32_t width, int32_t height,
                                                       sptr<IStreamRepeat>& previewOutput)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if ((producer == nullptr) || (width == 0) || (height == 0)) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCustomPreviewOutput producer is null or invalid size is set");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCustomPreviewOutput Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteRemoteObject(producer->AsObject())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCustomPreviewOutput write producer obj failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(format)) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCustomPreviewOutput Write format failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(width)) {
        MEDIA_ERR_LOG("HCameraServiceProxy Write width failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteInt32(height)) {
        MEDIA_ERR_LOG("HCameraServiceProxy Write height failed");
        return IPC_PROXY_ERR;
    }
    int error = Remote()->SendRequest(CAMERA_SERVICE_CREATE_PREVIEW_OUTPUT_CUSTOM_SIZE, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCustomPreviewOutput failed, error: %{public}d", error);
        return error;
    }
    auto remoteObject = reply.ReadRemoteObject();
    if (remoteObject != nullptr) {
        previewOutput = iface_cast<IStreamRepeat>(remoteObject);
    } else {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateCustomPreviewOutput previewOutput is null");
        error = IPC_PROXY_ERR;
    }
    return error;
}

int32_t HCameraServiceProxy::CreateVideoOutput(const sptr<OHOS::IBufferProducer> &producer, int32_t format,
                                               sptr<IStreamRepeat>& videoOutput)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateVideoOutput producer is null");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateVideoOutput Write interface token failed");
        return IPC_PROXY_ERR;
    }
    if (!data.WriteRemoteObject(producer->AsObject())) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateVideoOutput write producer obj failed");
        return IPC_PROXY_ERR;
    }

    if (!data.WriteInt32(format)) {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateVideoOutput Write format failed");
        return IPC_PROXY_ERR;
    }

    int error = Remote()->SendRequest(CAMERA_SERVICE_CREATE_VIDEO_OUTPUT, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy::CreateVideoOutput failed, error: %{public}d", error);
        return error;
    }

    auto remoteObject = reply.ReadRemoteObject();
    if (remoteObject != nullptr) {
        videoOutput = iface_cast<IStreamRepeat>(remoteObject);
    } else {
        MEDIA_ERR_LOG("HCameraServiceProxy CreateVideoOutput videoOutput is null");
        error = IPC_PROXY_ERR;
    }

    return error;
}

int32_t HCameraServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        MEDIA_ERR_LOG("HCameraServiceProxy::SetListenerObject Failed to write descriptor");
        return IPC_PROXY_ERR;
    }

    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(CAMERA_SERVICE_SET_LISTENER_OBJ, data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HCameraServiceProxy::SetListenerObject Set listener obj failed, error: %{public}d", error);
        return IPC_PROXY_ERR;
    }

    return reply.ReadInt32();
}
} // namespace CameraStandard
} // namespace OHOS
