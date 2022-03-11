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

#include "camera_capture_video.h"
#include <securec.h>
#include <unistd.h>
#include "camera_util.h"
#include "media_log.h"
#include "test_common.h"

using namespace OHOS;
using namespace OHOS::CameraStandard;

static void PhotoModeUsage(FILE *fp)
{
    int32_t result = 0;

    result = fprintf(fp,
        "---------------------\n"
        "Running in Photo mode\n"
        "---------------------\n"
        "Options:\n"
        "h      Print this message\n"
        "c      Capture one picture\n"
        "v      Switch to video mode\n"
        "d      Double preview mode\n"
        "q      Quit this app\n");
    if (result < 0) {
        MEDIA_ERR_LOG("Failed to display menu, %{public}d", result);
    }
}

static void VideoModeUsage(FILE *fp)
{
    int32_t result = 0;

    result = fprintf(fp,
        "---------------------\n"
        "Running in Video mode\n"
        "---------------------\n"
        "Options:\n"
        "h      Print this message\n"
        "r      Record video\n"
        "p      Switch to Photo mode\n"
        "d      Switch to Double preview mode\n"
        "q      Quit this app\n");
    if (result < 0) {
        MEDIA_ERR_LOG("Failed to display menu, %{public}d", result);
    }
}

static void DoublePreviewModeUsage(FILE *fp)
{
    int32_t result = 0;

    result = fprintf(fp,
        "---------------------\n"
        "Running in Double preview mode\n"
        "---------------------\n"
        "Options:\n"
        "h      Print this message\n"
        "p      Switch to Photo mode\n"
        "v      Switch to Video mode\n"
        "q      Quit this app\n");
    if (result < 0) {
        MEDIA_ERR_LOG("Failed to display menu, %{public}d", result);
    }
}

static void Usage(std::shared_ptr<CameraCaptureVideo> testObj)
{
    if (testObj->currentState_ == State::PHOTO_CAPTURE) {
        PhotoModeUsage(stdout);
    } else if (testObj->currentState_ == State::VIDEO_RECORDING) {
        VideoModeUsage(stdout);
    } else if (testObj->currentState_ == State::DOUBLE_PREVIEW) {
        DoublePreviewModeUsage(stdout);
    }
    return;
}

static char PutMenuAndGetChr(std::shared_ptr<CameraCaptureVideo> &testObj)
{
    int32_t result = 0;
    char userInput[1];

    Usage(testObj);
    result = scanf_s(" %c", &userInput, 1);
    if (result == 0) {
        return 'h';
    }
    return userInput[0];
}

static int32_t SwitchMode(std::shared_ptr<CameraCaptureVideo> testObj, State state)
{
    int32_t result = CAMERA_OK;

    if (testObj->currentState_ != state) {
        testObj->Release();
        testObj->currentState_ = state;
        result = testObj->StartPreview();
    }
    return result;
}

static void DisplayMenu(std::shared_ptr<CameraCaptureVideo> testObj)
{
    char c = 'h';
    int32_t result = CAMERA_OK;

    while (1  && (result == CAMERA_OK)) {
        switch (c) {
            case 'h':
                c = PutMenuAndGetChr(testObj);
                break;

            case 'c':
                if (testObj->currentState_ == State::PHOTO_CAPTURE) {
                    result = testObj->TakePhoto();
                }
                if (result == CAMERA_OK) {
                    c = PutMenuAndGetChr(testObj);
                }
                break;

            case 'r':
                if (testObj->currentState_ == State::VIDEO_RECORDING) {
                    result = testObj->RecordVideo();
                }
                if (result == CAMERA_OK) {
                    c = PutMenuAndGetChr(testObj);
                }
                break;

            case 'v':
                if (testObj->currentState_ != State::VIDEO_RECORDING) {
                    result = SwitchMode(testObj, State::VIDEO_RECORDING);
                }
                if (result == CAMERA_OK) {
                    c = PutMenuAndGetChr(testObj);
                }
                break;

            case 'p':
                if (testObj->currentState_ != State::PHOTO_CAPTURE) {
                    result = SwitchMode(testObj, State::PHOTO_CAPTURE);
                }
                if (result == CAMERA_OK) {
                    c = PutMenuAndGetChr(testObj);
                }
                break;

            case 'd':
                if (testObj->currentState_ != State::DOUBLE_PREVIEW) {
                    result = SwitchMode(testObj, State::DOUBLE_PREVIEW);
                }
                if (result == CAMERA_OK) {
                    c = PutMenuAndGetChr(testObj);
                }
                break;

            case 'q':
                testObj->Release();
                exit(EXIT_SUCCESS);

            default:
                c = PutMenuAndGetChr(testObj);
                break;
        }
    }
    if (result != CAMERA_OK) {
        std::cout << "Operation Failed!, Check logs for more details!, result: " << result << std::endl;
        testObj->Release();
        exit(EXIT_SUCCESS);
    }
}

CameraCaptureVideo::CameraCaptureVideo()
{
    previewWidth_ = PREVIEW_WIDTH;
    previewHeight_ = PREVIEW_HEIGHT;
    previewWidth2_ = SECOND_PREVIEW_WIDTH;
    previewHeight2_ = SECOND_PREVIEW_HEIGHT;
    photoWidth_ = PHOTO_WIDTH;
    photoHeight_ = PHOTO_HEIGHT;
    videoWidth_ = VIDEO_WIDTH;
    videoHeight_ = VIDEO_HEIGHT;
    previewFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    photoFormat_ = OHOS_CAMERA_FORMAT_JPEG;
    videoFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    currentState_ = State::PHOTO_CAPTURE;
    fd_ = -1;
}

int32_t CameraCaptureVideo::TakePhoto()
{
    int32_t result = -1;

    result = ((sptr<PhotoOutput> &)photoOutput_)->Capture();
    if (result != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to capture, result: %{public}d", result);
        return result;
    }
    sleep(GAP_AFTER_CAPTURE);
    return result;
}

int32_t CameraCaptureVideo::RecordVideo()
{
    int32_t result = -1;

    result = ((sptr<VideoOutput> &)videoOutput_)->Start();
    if (result != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to start recording, result: %{public}d", result);
        return result;
    }
    sleep(VIDEO_CAPTURE_DURATION);
    result = ((sptr<VideoOutput> &)videoOutput_)->Stop();
    if (result != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to stop recording, result: %{public}d", result);
        return result;
    }
    sleep(GAP_AFTER_CAPTURE);
    result = TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, fd_);
    fd_ = -1;
    return result;
}

void CameraCaptureVideo::Release()
{
    if (captureSession_ != nullptr) {
        captureSession_->Stop();
        sleep(GAP_AFTER_STOP);
        captureSession_->Release();
        captureSession_ = nullptr;
    }
    if (cameraInput_ != nullptr) {
        cameraInput_->Release();
        cameraInput_ = nullptr;
    }
    cameraInputCallback_ = nullptr;
    previewSurface_ = nullptr;
    previewSurfaceListener_ = nullptr;
    previewOutput_ = nullptr;
    previewOutputCallback_ = nullptr;
    photoSurface_ = nullptr;
    photoSurfaceListener_ = nullptr;
    sptr<CaptureOutput> photoOutput_ = nullptr;
    photoOutputCallback_ = nullptr;
    videoSurface_ = nullptr;
    videoSurfaceListener_ = nullptr;
    videoOutput_ = nullptr;
    videoOutputCallback_ = nullptr;
    secondPreviewSurface_ = nullptr;
    secondPreviewSurfaceListener_ = nullptr;
    secondPreviewOutput_ = nullptr;
    secondPreviewOutputCallback_ = nullptr;
}

int32_t CameraCaptureVideo::InitCameraManager()
{
    int32_t result = -1;

    if (cameraManager_ == nullptr) {
        cameraManager_ = CameraManager::GetInstance();
        if (cameraManager_ == nullptr) {
            MEDIA_ERR_LOG("Failed to get camera manager!");
            return result;
        }
        cameraMngrCallback_ = std::make_shared<TestCameraMngerCallback>(testName_);
        cameraManager_->SetCallback(cameraMngrCallback_);
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::InitCameraFormatAndResolution(sptr<CameraInput> &cameraInput)
{
    std::vector<camera_format_t> previewFormats = cameraInput->GetSupportedPreviewFormats();
    MEDIA_DEBUG_LOG("Supported preview formats:");
    for (auto &format : previewFormats) {
        MEDIA_DEBUG_LOG("format : %{public}d", format);
    }
    if (std::find(previewFormats.begin(), previewFormats.end(), OHOS_CAMERA_FORMAT_YCRCB_420_SP)
        != previewFormats.end()) {
        previewFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
        MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is present in supported preview formats");
    } else if (!previewFormats.empty()) {
        previewFormat_ = previewFormats[0];
        MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is not present in supported preview formats");
    }
    std::vector<camera_format_t> photoFormats = cameraInput->GetSupportedPhotoFormats();
    MEDIA_DEBUG_LOG("Supported photo formats:");
    for (auto &format : photoFormats) {
        MEDIA_DEBUG_LOG("format : %{public}d", format);
    }
    if (!photoFormats.empty()) {
        photoFormat_ = photoFormats[0];
    }
    std::vector<camera_format_t> videoFormats = cameraInput->GetSupportedVideoFormats();
    MEDIA_DEBUG_LOG("Supported video formats:");
    for (auto &format : videoFormats) {
        MEDIA_DEBUG_LOG("format : %{public}d", format);
    }
    if (std::find(videoFormats.begin(), videoFormats.end(), OHOS_CAMERA_FORMAT_YCRCB_420_SP) != videoFormats.end()) {
        videoFormat_ = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
        MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is present in supported video formats");
    } else if (!videoFormats.empty()) {
        videoFormat_ = videoFormats[0];
        MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is not present in supported video formats");
    }
    std::vector<CameraPicSize> previewSizes
        = cameraInput->getSupportedSizes(static_cast<camera_format_t>(previewFormat_));
    MEDIA_DEBUG_LOG("Supported sizes for preview:");
    for (auto &size : previewSizes) {
        MEDIA_DEBUG_LOG("width: %{public}d, height: %{public}d", size.width, size.height);
    }
    std::vector<CameraPicSize> photoSizes = cameraInput->getSupportedSizes(static_cast<camera_format_t>(photoFormat_));
    MEDIA_DEBUG_LOG("Supported sizes for photo:");
    for (auto &size : photoSizes) {
        MEDIA_DEBUG_LOG("width: %{public}d, height: %{public}d", size.width, size.height);
    }
    std::vector<CameraPicSize> videoSizes = cameraInput->getSupportedSizes(static_cast<camera_format_t>(videoFormat_));
    MEDIA_DEBUG_LOG("Supported sizes for video:");
    for (auto &size : videoSizes) {
        MEDIA_DEBUG_LOG("width: %{public}d, height: %{public}d", size.width, size.height);
    }

    if (!photoSizes.empty()) {
        photoWidth_ = photoSizes[0].width;
        photoHeight_ = photoSizes[0].height;
    }
    if (!videoSizes.empty()) {
        videoWidth_ = videoSizes[0].width;
        videoHeight_ = videoSizes[0].height;
    }
    if (!previewSizes.empty()) {
        previewWidth_ = previewSizes[0].width;
        previewHeight_ = previewSizes[0].height;
    }
    if (previewSizes.size() > 1) {
        previewWidth2_ = previewSizes[1].width;
        previewHeight2_ = previewSizes[1].height;
    }

    MEDIA_DEBUG_LOG("previewFormat: %{public}d, previewWidth: %{public}d, previewHeight: %{public}d",
                    previewFormat_, previewWidth_, previewHeight_);
    MEDIA_DEBUG_LOG("previewFormat: %{public}d, previewWidth2: %{public}d, previewHeight2: %{public}d",
                    previewFormat_, previewWidth2_, previewHeight2_);
    MEDIA_DEBUG_LOG("photoFormat: %{public}d, photoWidth: %{public}d, photoHeight: %{public}d",
                    photoFormat_, photoWidth_, photoHeight_);
    MEDIA_DEBUG_LOG("videoFormat: %{public}d, videoWidth: %{public}d, videoHeight: %{public}d",
                    videoFormat_, videoWidth_, videoHeight_);
    return CAMERA_OK;
}

int32_t CameraCaptureVideo::InitCameraInput()
{
    int32_t result = -1;

    if (cameraInput_ == nullptr) {
        std::vector<sptr<CameraInfo>> cameraObjList = cameraManager_->GetCameras();
        if (cameraObjList.size() <= 0) {
            MEDIA_ERR_LOG("No cameras are available!!!");
            return result;
        }
        cameraInput_ = cameraManager_->CreateCameraInput(cameraObjList[0]);
        if (cameraInput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create CameraInput");
            return result;
        }
        cameraInputCallback_ = std::make_shared<TestDeviceCallback>(testName_);
        ((sptr<CameraInput> &)cameraInput_)->SetErrorCallback(cameraInputCallback_);
        result = InitCameraFormatAndResolution((sptr<CameraInput> &)cameraInput_);
        if (result != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to initialize format and resolution for preview, photo and video");
            return result;
        }
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::InitPreviewOutput()
{
    int32_t result = -1;

    if (previewOutput_ == nullptr) {
        previewSurface_ = Surface::CreateSurfaceAsConsumer();
        previewSurface_->SetDefaultWidthAndHeight(previewWidth_, previewHeight_);
        previewSurface_->SetUserData(CameraManager::surfaceFormat, std::to_string(previewFormat_));
        previewSurfaceListener_ = new SurfaceListener(testName_, SurfaceType::PREVIEW, fd_, previewSurface_);
        previewSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)previewSurfaceListener_);
        previewOutput_ = cameraManager_->CreatePreviewOutput(previewSurface_);
        if (previewOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create previewOutput");
            return result;
        }
        previewOutputCallback_ = std::make_shared<TestPreviewOutputCallback>(testName_);
        ((sptr<PreviewOutput> &)previewOutput_)->SetCallback(previewOutputCallback_);
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::InitSecondPreviewOutput()
{
    int32_t result = -1;

    if (secondPreviewOutput_ == nullptr) {
        secondPreviewSurface_ = Surface::CreateSurfaceAsConsumer();
        secondPreviewSurface_->SetDefaultWidthAndHeight(previewWidth_, previewHeight_);
        secondPreviewSurface_->SetUserData(CameraManager::surfaceFormat, std::to_string(previewFormat_));
        secondPreviewSurfaceListener_ = new SurfaceListener(testName_,
            SurfaceType::SECOND_PREVIEW, fd_, secondPreviewSurface_);
        secondPreviewSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)secondPreviewSurfaceListener_);
        secondPreviewOutput_ = cameraManager_->CreateCustomPreviewOutput(secondPreviewSurface_->GetProducer(),
                                                                         previewFormat_, previewWidth2_,
                                                                         previewHeight2_);
        if (secondPreviewOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create second previewOutput");
            return result;
        }
        secondPreviewOutputCallback_ = std::make_shared<TestPreviewOutputCallback>(testName_);
        ((sptr<PreviewOutput> &)secondPreviewOutput_)->SetCallback(secondPreviewOutputCallback_);
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::InitPhotoOutput()
{
    int32_t result = -1;

    if (photoOutput_ == nullptr) {
        photoSurface_ = Surface::CreateSurfaceAsConsumer();
        photoSurface_->SetDefaultWidthAndHeight(photoWidth_, photoHeight_);
        photoSurface_->SetUserData(CameraManager::surfaceFormat, std::to_string(photoFormat_));
        photoSurfaceListener_ = new SurfaceListener(testName_, SurfaceType::PHOTO, fd_, photoSurface_);
        photoSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)photoSurfaceListener_);
        photoOutput_ = cameraManager_->CreatePhotoOutput(photoSurface_);
        if (photoOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create PhotoOutput");
            return result;
        }
        photoOutputCallback_ = std::make_shared<TestPhotoOutputCallback>(testName_);
        ((sptr<PhotoOutput> &)photoOutput_)->SetCallback(photoOutputCallback_);
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::InitVideoOutput()
{
    int32_t result = -1;

    if (videoOutput_ == nullptr) {
        videoSurface_ = Surface::CreateSurfaceAsConsumer();
        videoSurface_->SetDefaultWidthAndHeight(videoWidth_, videoHeight_);
        videoSurface_->SetUserData(CameraManager::surfaceFormat, std::to_string(videoFormat_));
        videoSurfaceListener_ = new SurfaceListener(testName_, SurfaceType::VIDEO, fd_, videoSurface_);
        videoSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)videoSurfaceListener_);
        videoOutput_ = cameraManager_->CreateVideoOutput(videoSurface_);
        if (videoOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create VideoOutput");
            return result;
        }
        videoOutputCallback_ = std::make_shared<TestVideoOutputCallback>(testName_);
        ((sptr<VideoOutput> &)videoOutput_)->SetCallback(videoOutputCallback_);
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::AddOutputbyState()
{
    int32_t result = -1;

    switch (currentState_) {
        case State::PHOTO_CAPTURE:
            result = InitPhotoOutput();
            if (result == CAMERA_OK) {
                result = captureSession_->AddOutput(photoOutput_);
            }
            break;
        case State::VIDEO_RECORDING:
            result = InitVideoOutput();
            if (result == CAMERA_OK) {
                result = captureSession_->AddOutput(videoOutput_);
            }
            break;
        case State::DOUBLE_PREVIEW:
            result = InitSecondPreviewOutput();
            if (result == CAMERA_OK) {
                result = captureSession_->AddOutput(secondPreviewOutput_);
            }
            break;
        default:
            break;
    }
    return result;
}

int32_t CameraCaptureVideo::StartPreview()
{
    int32_t result = -1;

    result = InitCameraManager();
    if (result != CAMERA_OK) {
        return result;
    }
    result = InitCameraInput();
    if (result != CAMERA_OK) {
        return result;
    }
    captureSession_ = cameraManager_->CreateCaptureSession();
    if (captureSession_ == nullptr) {
        return result;
    }
    captureSession_->BeginConfig();
    result = captureSession_->AddInput(cameraInput_);
    if (CAMERA_OK != result) {
        return result;
    }
    result = AddOutputbyState();
    if (result != CAMERA_OK) {
        return result;
    }
    result = InitPreviewOutput();
    if (result != CAMERA_OK) {
        return result;
    }
    result = captureSession_->AddOutput(previewOutput_);
    if (CAMERA_OK != result) {
        return result;
    }
    result = captureSession_->CommitConfig();
    if (CAMERA_OK != result) {
        MEDIA_ERR_LOG("Failed to Commit config");
        return result;
    }
    result = captureSession_->Start();
    MEDIA_DEBUG_LOG("Preview started, result: %{public}d", result);
    return result;
}

int32_t main(int32_t argc, char **argv)
{
    int32_t result = 0; // Default result
    std::shared_ptr<CameraCaptureVideo> testObj =
        std::make_shared<CameraCaptureVideo>();
    result = testObj->StartPreview();
    DisplayMenu(testObj);
    return 0;
}
