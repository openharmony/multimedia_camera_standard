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

#include "input/camera_input.h"
#include "input/camera_manager.h"
#include "media_errors.h"
#include "media_log.h"
#include "recorder.h"
#include "surface.h"
#include "test_common.h"

#include <unistd.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::CameraStandard;
using namespace OHOS::Media;

namespace {
    int32_t g_videoFd = -1;
    class TestVideoRecorderCallback : public RecorderCallback {
    public:
        void OnError(RecorderErrorType errorType, int32_t errorCode)
        {
            MEDIA_DEBUG_LOG("OnError errorType is %{public}d, errorCode is  %{public}d", errorType, errorCode);
        }
        void OnInfo(int32_t type, int32_t extra)
        {
            MEDIA_DEBUG_LOG("OnInfo Type is %{public}d, extra is  %{public}d", type, extra);
        }
    };

    bool ConfigureVideoParams(const std::shared_ptr<Recorder> &recorder,
                              int32_t videoSourceId, int32_t width, int32_t height)
    {
        int32_t encodingBitRate  = 48000;
        int32_t frameRate = 30;
        int32_t captureRate = 30;

        if (recorder->SetVideoEncoder(videoSourceId, Media::H264)) {
            MEDIA_DEBUG_LOG("Set Video Encoder Failed");
            return false;
        }

        if (recorder->SetVideoSize(videoSourceId, width, height)) {
            MEDIA_DEBUG_LOG("Set Video Size Failed");
            return false;
        }

        if (recorder->SetVideoFrameRate(videoSourceId, frameRate)) {
            MEDIA_DEBUG_LOG("Set Video Frame Rate Failed");
            return false;
        }

        if (recorder->SetVideoEncodingBitRate(videoSourceId, encodingBitRate)) {
            MEDIA_DEBUG_LOG("Set Video Encoding Bit Rate Failed");
            return false;
        }

        if (recorder->SetCaptureRate(videoSourceId, captureRate)) {
            MEDIA_DEBUG_LOG("Set Capture Rate Failed");
            return false;
        }

        return true;
    }

    bool ConfigureAudioParams(const std::shared_ptr<Recorder> &recorder, int32_t audioSourceId)
    {
        int32_t channelCount = 2;
        int32_t sampleRate = 48000;
        int32_t encodingBitRate = 48000;

        if (recorder->SetAudioEncoder(audioSourceId, Media::AAC_LC)) {
            MEDIA_DEBUG_LOG("Set Audio Encoder Failed");
            return false;
        }

        if (recorder->SetAudioSampleRate(audioSourceId, sampleRate)) {
            MEDIA_DEBUG_LOG("Set Audio Sample Rate Failed");
            return false;
        }

        if (recorder->SetAudioChannels(audioSourceId, channelCount)) {
            MEDIA_DEBUG_LOG("Set Audio Channels Failed");
            return false;
        }

        if (recorder->SetAudioEncodingBitRate(audioSourceId, encodingBitRate)) {
            MEDIA_DEBUG_LOG("Set Audio Encoding Bit Rate Failed");
            return false;
        }

        return true;
    }

    bool CreateAndConfigureRecorder(std::shared_ptr<Recorder> &recorder,
                                    int32_t &videoSourceId, int32_t width, int32_t height)
    {
        int32_t maxDuration = 36000;

        recorder = RecorderFactory::CreateRecorder();
        if (recorder == nullptr) {
            MEDIA_DEBUG_LOG("Create Recorder Failed");
            return false;
        }

        int32_t audioSourceId = 0;
        if (recorder->SetVideoSource(Media::VIDEO_SOURCE_SURFACE_ES, videoSourceId)) {
            MEDIA_DEBUG_LOG("Set Video Source Failed");
            return false;
        }

        if (recorder->SetAudioSource(Media::AUDIO_MIC, audioSourceId)) {
            MEDIA_DEBUG_LOG("Set Audio Source Failed");
            return false;
        }

        if (recorder->SetOutputFormat(Media::FORMAT_MPEG_4)) {
            MEDIA_DEBUG_LOG("Set Output Format Failed");
            return false;
        }

        if (!ConfigureVideoParams(recorder, videoSourceId, width, height)) {
            MEDIA_DEBUG_LOG("Failed to configure video for recorder");
            return false;
        }

        if (!ConfigureAudioParams(recorder, audioSourceId)) {
            MEDIA_DEBUG_LOG("Failed to configure audio for recorder");
            return false;
        }

        if (recorder->SetMaxDuration(maxDuration)) {
            MEDIA_DEBUG_LOG("Set Max Duration Failed");
            return false;
        }

        if (recorder->SetOutputPath("/data/media")) {
            MEDIA_DEBUG_LOG("Set output Path Failed");
            return false;
        }

        if (recorder->SetRecorderCallback(std::make_shared<TestVideoRecorderCallback>())) {
            MEDIA_DEBUG_LOG("Set Recorder Callback Failed");
            return false;
        }
        return true;
    }
}

int main(int argc, char **argv)
{
    const int32_t previewFormatIndex = 1;
    const int32_t previewWidthIndex = 2;
    const int32_t previewHeightIndex = 3;
    const int32_t videoFormatIndex = 4;
    const int32_t videoWidthIndex = 5;
    const int32_t videoHeightIndex = 6;
    const int32_t validArgCount = 7;
    const int32_t videoCaptureDuration = 5; // Sleep for 5 sec
    const int32_t videoPauseDuration = 2; // Sleep for 2 sec
    const int32_t previewVideoGap = 2; // Sleep for 2 sec
    const char *testName = "camera_video";
    int32_t ret = -1;
    int32_t previewFd = -1;
    int32_t previewFormat = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    int32_t previewWidth = 640;
    int32_t previewHeight = 480;
    int32_t videoFormat = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    int32_t videoWidth = 640;
    int32_t videoHeight = 360;
    bool isResolutionConfigured = false;
    bool isRecorder = false;

    MEDIA_DEBUG_LOG("Camera new sample begin without recorder");
    // Update sizes if enough number of valid arguments are passed
    if (argc == validArgCount) {
        // Validate arguments and consider if valid
        for (int counter = 1; counter < argc; counter++) {
            if (!TestUtils::IsNumber(argv[counter])) {
                cout << "Invalid argument: " << argv[counter] << endl;
                cout << "Retry by giving proper sizes" << endl;
                return 0;
            }
        }
        previewFormat = atoi(argv[previewFormatIndex]);
        previewWidth = atoi(argv[previewWidthIndex]);
        previewHeight = atoi(argv[previewHeightIndex]);
        videoFormat = atoi(argv[videoFormatIndex]);
        videoWidth = atoi(argv[videoWidthIndex]);
        videoHeight = atoi(argv[videoHeightIndex]);
        isResolutionConfigured = true;
    } else if (argc != 1) {
        cout << "Pass " << (validArgCount - 1) << "arguments" << endl;
        cout << "PreviewFormat, PreviewWidth, PreviewHeight, VideoFormat, VideoWidth, VideoHeight" << endl;
        return 0;
    }

    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    MEDIA_DEBUG_LOG("Setting callback to listen camera status and flash status");
    camManagerObj->SetCallback(std::make_shared<TestCameraMngerCallback>(testName));
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    if (cameraObjList.size() == 0) {
        MEDIA_DEBUG_LOG("No camera devices");
        return 0;
    }

    MEDIA_DEBUG_LOG("Camera ID count: %{public}zu", cameraObjList.size());
    for (auto& it : cameraObjList) {
        MEDIA_DEBUG_LOG("Camera ID: %{public}s", it->GetID().c_str());
    }

    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    if (captureSession == nullptr) {
        MEDIA_DEBUG_LOG("Failed to create capture session");
        return 0;
    }

    captureSession->BeginConfig();

    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    if (cameraInput == nullptr) {
        MEDIA_DEBUG_LOG("Failed to create camera input");
        return 0;
    }

    if (!isResolutionConfigured) {
        std::vector<camera_format_t> previewFormats = ((sptr<CameraInput> &)cameraInput)->GetSupportedPreviewFormats();
        if (previewFormats.empty()) {
            MEDIA_DEBUG_LOG("No preview formats supported");
        }
        MEDIA_DEBUG_LOG("Supported preview formats:");
        camera_format_t format;
        for (auto item = previewFormats.begin(); item != previewFormats.end(); ++item) {
            format = *item;
            MEDIA_DEBUG_LOG("format : %{public}d", format);
        }
        if (std::find(previewFormats.begin(), previewFormats.end(), OHOS_CAMERA_FORMAT_YCRCB_420_SP)
            != previewFormats.end()) {
            previewFormat = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
            MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is present in supported preview formats");
        } else if(!previewFormats.empty()) {
            previewFormat = previewFormats[0];
            MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is not present in supported preview formats");
        }
        std::vector<camera_format_t> videoFormats = ((sptr<CameraInput> &)cameraInput)->GetSupportedVideoFormats();
        if (videoFormats.empty()) {
            MEDIA_DEBUG_LOG("No video formats supported");
        }
        MEDIA_DEBUG_LOG("Supported video formats:");
        for (auto item = videoFormats.begin(); item != videoFormats.end(); ++item) {
            format = *item;
            MEDIA_DEBUG_LOG("format : %{public}d", format);
        }
        if (std::find(videoFormats.begin(), videoFormats.end(), OHOS_CAMERA_FORMAT_YCRCB_420_SP)
            != videoFormats.end()) {
            videoFormat = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
            MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is present in supported video formats");
        } else if(!videoFormats.empty()) {
            videoFormat = videoFormats[0];
            MEDIA_DEBUG_LOG("OHOS_CAMERA_FORMAT_YCRCB_420_SP format is not present in supported video formats");
        }
        std::vector<CameraPicSize> previewSizes
            = ((sptr<CameraInput> &)cameraInput)->getSupportedSizes(static_cast<camera_format_t>(previewFormat));
        if (previewSizes.empty()) {
            MEDIA_DEBUG_LOG("No preview sizes supported");
        }
        MEDIA_DEBUG_LOG("Supported sizes for preview:");
        CameraPicSize size;
        for (auto item = previewSizes.begin(); item != previewSizes.end(); ++item) {
            size = *item;
            MEDIA_DEBUG_LOG("width: %{public}d, height: %{public}d", size.width, size.height);
        }
        std::vector<CameraPicSize> videoSizes
            = ((sptr<CameraInput> &)cameraInput)->getSupportedSizes(static_cast<camera_format_t>(videoFormat));
        if (videoSizes.empty()) {
            MEDIA_DEBUG_LOG("No video sizes supported");
        }
        MEDIA_DEBUG_LOG("Supported sizes for video:");
        for (auto item = videoSizes.begin(); item != videoSizes.end(); ++item) {
            size = *item;
            MEDIA_DEBUG_LOG("width: %{public}d, height: %{public}d", size.width, size.height);
        }
        if (!previewSizes.empty()) {
            previewWidth = previewSizes[0].width;
            previewHeight = previewSizes[0].height;
        }
        if (!videoSizes.empty()) {
            videoWidth = videoSizes[0].width;
            videoHeight = videoSizes[0].height;
        }
    }

    MEDIA_DEBUG_LOG("previewFormat: %{public}d, previewWidth: %{public}d, previewHeight: %{public}d",
                    previewFormat, previewWidth, previewHeight);
    MEDIA_DEBUG_LOG("videoFormat: %{public}d, videoWidth: %{public}d, videoHeight: %{public}d",
                    videoFormat, videoWidth, videoHeight);

    ((sptr<CameraInput> &)cameraInput)->SetErrorCallback(std::make_shared<TestDeviceCallback>(testName));
    ret = captureSession->AddInput(cameraInput);
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Add input to session is failed, ret: %{public}d", ret);
        return 0;
    }

    sptr<Surface> previewSurface = Surface::CreateSurfaceAsConsumer();
    previewSurface->SetDefaultWidthAndHeight(previewWidth, previewHeight);
    previewSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(previewFormat));
    sptr<SurfaceListener> listener = new SurfaceListener("Preview", SurfaceType::PREVIEW, previewFd, previewSurface);
    previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)listener);

    sptr<CaptureOutput> previewOutput = camManagerObj->CreatePreviewOutput(previewSurface);
    if (previewOutput == nullptr) {
        MEDIA_DEBUG_LOG("Failed to create preview output");
        return 0;
    }

    MEDIA_DEBUG_LOG("Setting preview callback");
    ((sptr<PreviewOutput> &)previewOutput)->SetCallback(std::make_shared<TestPreviewOutputCallback>(testName));
    ret = captureSession->AddOutput(previewOutput);
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to Add output to session, ret: %{public}d", ret);
        return 0;
    }

    sptr<Surface> videoSurface = nullptr;
    std::shared_ptr<Recorder> recorder = nullptr;
    if (isRecorder) {
        int32_t videoSourceId = 0;
        if (!CreateAndConfigureRecorder(recorder, videoSourceId, videoWidth, videoHeight)) {
            MEDIA_DEBUG_LOG("Failed to create and configure recorder");
            return 0;
        }

        if (recorder->Prepare()) {
            MEDIA_DEBUG_LOG("Failed to prepare recorder");
            return 0;
        }

        videoSurface = recorder->GetSurface(videoSourceId);
        if (videoSurface == nullptr) {
            MEDIA_DEBUG_LOG("Failed to get surface from recorder");
            return 0;
        }
        videoSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(videoFormat));
    } else {
        videoSurface = Surface::CreateSurfaceAsConsumer();
        videoSurface->SetDefaultWidthAndHeight(videoWidth, videoHeight);
        videoSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(videoFormat));
        sptr<SurfaceListener> videoListener = new SurfaceListener("Video", SurfaceType::VIDEO, g_videoFd, videoSurface);
        videoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)videoListener);
    }

    sptr<CaptureOutput> videoOutput = camManagerObj->CreateVideoOutput(videoSurface);
    if (videoOutput == nullptr) {
        MEDIA_DEBUG_LOG("Failed to create video output");
        return 0;
    }

    MEDIA_DEBUG_LOG("Setting video callback");
    ((sptr<VideoOutput> &)videoOutput)->SetCallback(std::make_shared<TestVideoOutputCallback>(testName));
    ret = captureSession->AddOutput(videoOutput);
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to Add output to session, ret: %{public}d", ret);
        return 0;
    }

    ret = captureSession->CommitConfig();
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to commit session config, ret: %{public}d", ret);
        return 0;
    }

    ret = captureSession->Start();
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to start session, ret: %{public}d", ret);
        return 0;
    }

    MEDIA_DEBUG_LOG("Preview started");
    sleep(previewVideoGap);
    MEDIA_DEBUG_LOG("Start video recording");

    ret = ((sptr<VideoOutput> &)videoOutput)->Start();
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to start video output, ret: %{public}d", ret);
        return 0;
    }

    if (recorder != nullptr) {
        ret = recorder->Start();
        if (ret != 0) {
            MEDIA_DEBUG_LOG("Failed to start recorder, return: %{public}s",
                OHOS::Media::MSErrorToString(static_cast<OHOS::Media::MediaServiceErrCode>(ret)).c_str());
            return 0;
        }
    }

    MEDIA_DEBUG_LOG("Wait for 5 seconds after start");
    sleep(videoCaptureDuration);
    MEDIA_DEBUG_LOG("Pause video recording for 2 sec");
    ret = ((sptr<VideoOutput> &)videoOutput)->Pause();
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to pause video output, ret: %{public}d", ret);
        return 0;
    }

    if (recorder != nullptr) {
        ret = recorder->Pause();
        if (ret != 0) {
            MEDIA_DEBUG_LOG("Failed to pause recorder, return: %{public}s",
                OHOS::Media::MSErrorToString(static_cast<OHOS::Media::MediaServiceErrCode>(ret)).c_str());
        }
    }
    sleep(videoPauseDuration);
    MEDIA_DEBUG_LOG("Resume video recording");
    ret = ((sptr<VideoOutput> &)videoOutput)->Resume();
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to resume video output, ret: %{public}d", ret);
        return 0;
    }

    if (recorder != nullptr) {
        ret = recorder->Resume();
        if (ret != 0) {
            MEDIA_DEBUG_LOG("Failed to resume recorder, return: %{public}s",
                OHOS::Media::MSErrorToString(static_cast<OHOS::Media::MediaServiceErrCode>(ret)).c_str());
        }
    }
    MEDIA_DEBUG_LOG("Wait for 5 seconds before stop");
    sleep(videoCaptureDuration);
    MEDIA_DEBUG_LOG("Stop video recording");
    ret = ((sptr<VideoOutput> &)videoOutput)->Stop();
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to stop video output, ret: %{public}d", ret);
        return 0;
    }

    if (recorder != nullptr) {
        ret = recorder->Stop(false);
        if (ret != 0) {
            MEDIA_DEBUG_LOG("Failed to stop recorder, return: %{public}s",
                OHOS::Media::MSErrorToString(static_cast<OHOS::Media::MediaServiceErrCode>(ret)).c_str());
        }

        ret = recorder->Reset();
        if (ret != 0) {
            MEDIA_DEBUG_LOG("Failed to reset recorder, return: %{public}s",
                OHOS::Media::MSErrorToString(static_cast<OHOS::Media::MediaServiceErrCode>(ret)).c_str());
        }

        ret = recorder->Release();
        if (ret != 0) {
            MEDIA_DEBUG_LOG("Failed to release recorder, return: %{public}s",
                OHOS::Media::MSErrorToString(static_cast<OHOS::Media::MediaServiceErrCode>(ret)).c_str());
        }
    }

    MEDIA_DEBUG_LOG("Closing the session");
    ret = captureSession->Stop();
    if (ret != 0) {
        MEDIA_DEBUG_LOG("Failed to stop session, ret: %{public}d", ret);
        return 0;
    }

    MEDIA_DEBUG_LOG("Releasing the session");
    captureSession->Release();

    // Close video file
    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);
    camManagerObj->SetCallback(nullptr);

    MEDIA_DEBUG_LOG("Camera new sample end.");
    return 0;
}
