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

#include "camera_capture_video.h"
#include <securec.h>
#include <unistd.h>
#include "camera_util.h"
#include "media_log.h"
#include "test_common.h"

using namespace OHOS;
using namespace OHOS::CameraStandard;

namespace {
    static const int32_t PREVIEW_WIDTH = 640;
    static const int32_t PREVIEW_HEIGHT = 480;
    static const int32_t SECOND_PREVIEW_WIDTH = 832;
    static const int32_t SECOND_PREVIEW_HEIGHT = 480;
    static const int32_t PHOTO_WIDTH = 1280;
    static const int32_t PHOTO_HEIGHT = 960;
    static const int32_t VIDEO_WIDTH = 640;
    static const int32_t VIDEO_HEIGHT = 360;
    static const int32_t GAP_AFTER_STOP = 1;
    static const char *TEST_NAME = "Camera_capture_video";
}

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
    photoWidth_ = PHOTO_WIDTH;
    photoHeight_ = PHOTO_HEIGHT;
    videoWidth_ = VIDEO_WIDTH;
    videoHeight_ = VIDEO_HEIGHT;
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
    sleep(gapAfterCapture_);
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
    sleep(videoCaptureDuration_);
    result = ((sptr<VideoOutput> &)videoOutput_)->Stop();
    if (result != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to stop recording, result: %{public}d", result);
        return result;
    }
    sleep(gapAfterCapture_);
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
    cameraInput_ = nullptr;
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
        cameraMngrCallback_ = std::make_shared<TestCameraMngerCallback>(TEST_NAME);
        cameraManager_->SetCallback(cameraMngrCallback_);
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::InitCameraInput()
{
    int32_t result = -1;

    if (cameraInput_ == nullptr) {
        std::vector<sptr<CameraInfo>> cameraObjList = cameraManager_->GetCameras();
        if (cameraObjList.size() <= 0) {
            MEDIA_ERR_LOG("No cameras are availble!!!");
            return result;
        }
        cameraInput_ = cameraManager_->CreateCameraInput(cameraObjList[0]);
        if (cameraInput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create CameraInput");
            return result;
        }
        cameraInputCallback_ = std::make_shared<TestDeviceCallback>(TEST_NAME);
        ((sptr<CameraInput> &)cameraInput_)->SetErrorCallback(cameraInputCallback_);
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
        previewSurfaceListener_ = new SurfaceListener(TEST_NAME, SurfaceType::PREVIEW, fd_, previewSurface_);
        previewSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)previewSurfaceListener_);
        previewOutput_ = cameraManager_->CreatePreviewOutput(previewSurface_);
        if (previewOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create previewOutput");
            return result;
        }
        previewOutputCallback_ = std::make_shared<TestPreviewOutputCallback>(TEST_NAME);
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
        secondPreviewSurfaceListener_ = new SurfaceListener(TEST_NAME,
            SurfaceType::SECOND_PREVIEW, fd_, secondPreviewSurface_);
        secondPreviewSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)secondPreviewSurfaceListener_);
        secondPreviewOutput_ = cameraManager_->CreateCustomPreviewOutput(secondPreviewSurface_->GetProducer(),
                                                                         SECOND_PREVIEW_WIDTH, SECOND_PREVIEW_HEIGHT);
        if (secondPreviewOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create second previewOutput");
            return result;
        }
        secondPreviewOutputCallback_ = std::make_shared<TestPreviewOutputCallback>(TEST_NAME);
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
        photoSurfaceListener_ = new SurfaceListener(TEST_NAME, SurfaceType::PHOTO, fd_, photoSurface_);
        photoSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)photoSurfaceListener_);
        photoOutput_ = cameraManager_->CreatePhotoOutput(photoSurface_);
        if (photoOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create PhotoOutput");
            return result;
        }
        photoOutputCallback_ = std::make_shared<TestPhotoOutputCallback>(TEST_NAME);
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
        videoSurfaceListener_ = new SurfaceListener(TEST_NAME, SurfaceType::VIDEO, fd_, videoSurface_);
        videoSurface_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)videoSurfaceListener_);
        videoOutput_ = cameraManager_->CreateVideoOutput(videoSurface_);
        if (videoOutput_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create VideoOutput");
            return result;
        }
        videoOutputCallback_ = std::make_shared<TestVideoOutputCallback>(TEST_NAME);
        ((sptr<VideoOutput> &)videoOutput_)->SetCallback(videoOutputCallback_);
    }
    result = CAMERA_OK;
    return result;
}

int32_t CameraCaptureVideo::AddOutputbyState()
{
    int32_t result = -1;

    switch (currentState_) {
        case State::PHOTO_CAPTURE :
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