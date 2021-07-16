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

#include "camera_kit.h"
#include "display_type.h"
#include "meta_data.h"
#include "recorder.h"
#include "window_manager.h"

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <securec.h>
#include <memory>

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
static void SampleSaveCapture(const char &buffer, uint32_t size)
{
    cout << "Start saving picture" << endl;
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    struct tm *ltm = localtime(&tv.tv_sec);
    if (ltm != nullptr) {
        ostringstream ss("Capture_");
        ss << "Capture" << ltm->tm_hour << "-" << ltm->tm_min << "-" << ltm->tm_sec << ".jpg";

        ofstream pic("/data/" + ss.str(), ofstream::out | ofstream::trunc);
        cout << "write " << size << " bytes" << endl;
        pic.write(&buffer, size);
        cout << "Saving picture end" << endl;
        pic.close();
    }
}

class SampleRecorderCallback : public RecorderCallback {
public:
    void OnError(int32_t errorType, int32_t errorCode) {}
    void OnInfo(int32_t type, int32_t extra) {}
    virtual ~SampleRecorderCallback() {}
};

class SampleFrameStateCallback : public FrameStateCallback {
    void OnFrameFinished(const Camera &camera, const FrameConfig &fc, const FrameResult &result) override
    {
        cout << "Receive frame complete inform." << endl;
        if (((FrameConfig &) fc).GetFrameConfigType() == FRAME_CONFIG_CAPTURE) {
            cout << "Capture frame received." << endl;
            list<Surface *> surfaceList = ((FrameConfig &) fc).GetSurfaces();
            for (Surface *surface : surfaceList) {
                sptr<SurfaceBuffer> buffer;
                int32_t flushFence;
                int64_t timestamp;
                Rect damage;
                SurfaceError ret = surface->AcquireBuffer(buffer, flushFence, timestamp, damage);
                if (ret == SURFACE_ERROR_OK) {
                    char *virtAddr = static_cast<char *>(buffer->GetVirAddr());
                    if (virtAddr != nullptr) {
                        int32_t bufferSize = stoi(surface->GetUserData("surface_buffer_size"));
                        SampleSaveCapture(*virtAddr, bufferSize);
                    }
                    surface->ReleaseBuffer(buffer, -1);
                }
            }
            delete &fc;
        }
    }
};

class SampleCameraStateMng : public CameraStateCallback {
public:
    SampleCameraStateMng() = delete;
    explicit SampleCameraStateMng(EventHandler &eventHdlr) : eventHdlr_(eventHdlr) {}
    ~SampleCameraStateMng()
    {
        CloseRecorder();
        if (cam_) {
            cam_->Release();
        }
    }
    void OnCreated(const Camera &c) override
    {
        cout << "Sample recv OnCreate camera." << endl;
        if (((Camera &)c).GetCameraConfig() == nullptr) {
            CameraConfig *config = CameraConfig::CreateCameraConfig();
            config->SetFrameStateCallback(fsCb_, eventHdlr_);
            ((Camera &)c).Configure(*config);
        }
        cam_ = (Camera *) &c;
    }
    void OnCreateFailed(const std::string cameraId, int32_t errorCode) override {}
    void OnReleased(const Camera &c) override {}

    std::shared_ptr<Recorder> SampleCreateRecorder()
    {
        int ret = 0;
        int32_t sampleRate = 48000;
        int32_t channelCount = 1;
        AudioCodecFormat audioFormat = AAC_LC;
        AudioSourceType inputSource = AUDIO_MIC;
        int32_t audioEncodingBitRate = sampleRate;
        VideoSourceType source = VIDEO_SOURCE_SURFACE_ES;
        int32_t frameRate = 30;
        double fps = 30;
        int32_t rate = 4096;
        int32_t sourceId = 0;
        int32_t audioSourceId = 0;
        int32_t width = 1920;
        int32_t height = 1080;
        VideoCodecFormat encoder = HEVC;

        std::shared_ptr<Recorder> recorder = RecorderFactory::CreateRecorder();
        if ((ret = recorder->SetVideoSource(source, sourceId)) != ERR_OK) {
            cout << "SetVideoSource failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetAudioSource(inputSource, audioSourceId)) != ERR_OK) {
            cout << "SetAudioSource failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetOutputFormat(FORMAT_MPEG_4)) != ERR_OK) {
            cout << "SetOutputFormat failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetVideoEncoder(sourceId, encoder)) != ERR_OK) {
            cout << "SetVideoEncoder failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetVideoSize(sourceId, width, height)) != ERR_OK) {
            cout << "SetVideoSize failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetVideoFrameRate(sourceId, frameRate)) != ERR_OK) {
            cout << "SetVideoFrameRate failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetVideoEncodingBitRate(sourceId, rate)) != ERR_OK) {
            cout << "SetVideoEncodingBitRate failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetCaptureRate(sourceId, fps)) != ERR_OK) {
            cout << "SetCaptureRate failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetAudioEncoder(audioSourceId, audioFormat)) != ERR_OK) {
            cout << "SetAudioEncoder failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetAudioSampleRate(audioSourceId, sampleRate)) != ERR_OK) {
            cout << "SetAudioSampleRate failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetAudioChannels(audioSourceId, channelCount)) != ERR_OK) {
            cout << "SetAudioChannels failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetAudioEncodingBitRate(audioSourceId, audioEncodingBitRate)) != ERR_OK) {
            cout << "SetAudioEncodingBitRate failed." << ret << endl;
            return nullptr;
        }
        if ((ret = recorder->SetMaxDuration(36000)) != ERR_OK) { // 36000s=10h
            cout << "SetAudioEncodingBitRate failed." << ret << endl;
            return nullptr;
        }
        std::shared_ptr<SampleRecorderCallback> recCB = std::make_shared<SampleRecorderCallback>();
        if ((ret = recorder->SetRecorderCallback(recCB)) != ERR_OK) {
            return nullptr;
        }
        videoSourceId = sourceId;
        return recorder;
    }

    void CloseRecorder()
    {
        if (recorder_ != nullptr) {
            recorder_->Stop(true);
            recorder_ = nullptr;
        }
    }

    int PrepareRecorder()
    {
        if (cam_ == nullptr) {
            cout << "Camera is not ready." << endl;
            return -1;
        }
        if (recorder_ == nullptr) {
            recorder_ = SampleCreateRecorder();
        }
        if (recorder_ == nullptr) {
            cout << "Recorder not available." << endl;
            return -1;
        }
        return ERR_OK;
    }

    void StartRecord()
    {
        if (recordState_ == STATE_RUNNING) {
            cout << "Camera is already recording." << endl;
            return;
        }
        int ret = PrepareRecorder();
        if (ret != ERR_OK) {
            cout << "PrepareRecorder failed." << endl;
            CloseRecorder();
            return;
        }
        const string path = "/data";
        ret = recorder_->SetOutputPath(path);
        if (ret != ERR_OK) {
            cout << "SetOutputPath failed. ret=" << ret << endl;
            CloseRecorder();
            return;
        }
        ret = recorder_->Prepare();
        if (ret != ERR_OK) {
            cout << "Prepare failed. ret=" << ret << endl;
            CloseRecorder();
            return;
        }
        ret = recorder_->Start();
        if (ret != ERR_OK) {
            cout << "recorder start failed. ret=" << ret << endl;
            CloseRecorder();
            return;
        }
        FrameConfig *fc = new FrameConfig(FRAME_CONFIG_RECORD);
        Surface *surface = (recorder_->GetSurface(videoSourceId)).GetRefPtr();

        int queueSize = 10;
        int oneKilobytes = 1024;
        int surfaceSize = oneKilobytes * oneKilobytes;
        int usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
        surface->SetUserData("surface_width", "1920");
        surface->SetUserData("surface_height", "1080");
        surface->SetQueueSize(queueSize);
        surface->SetUserData("surface_size", to_string(surfaceSize));
        surface->SetUserData("surface_stride_Alignment", "8");
        surface->SetUserData("surface_format", to_string(PIXEL_FMT_RGBA_8888));
        surface->SetUserData("surface_usage", to_string(usage));
        surface->SetUserData("surface_timeout", "0");

        fc->AddSurface(*surface);
        ret = cam_->TriggerLoopingCapture(*fc);
        if (ret != 0) {
            delete fc;
            CloseRecorder();
            cout << "camera start recording failed. ret=" << ret << endl;
            return;
        }
        recordConfig = fc;
        recordState_ = STATE_RUNNING;
        cout << "camera start recording succeed." << endl;
    }

    void StartPreview()
    {
        if (cam_ == nullptr) {
            cout << "Camera is not ready." << endl;
            return;
        }
        if (previewState_ == STATE_RUNNING) {
            cout << "Camera is already previewing." << endl;
            return;
        }

        WindowConfig config;
        memset_s(&config, sizeof(WindowConfig), 0, sizeof(WindowConfig));
        config.height = 480;
        config.width = 480;
        config.format = PIXEL_FMT_RGBA_8888;
        config.pos_x = 0;
        config.pos_y = 0;

        WindowConfig subConfig = config;
        subConfig.type = WINDOW_TYPE_VIDEO;

        int queueSize = 10;
        window = WindowManager::GetInstance()->CreateWindow(&config);
        sptr<SurfaceBuffer> buffer;
        int32_t releaseFence;
        BufferRequestConfig requestConfig;
        window->GetRequestConfig(requestConfig);
        SurfaceError error = window->GetSurface()->RequestBuffer(buffer, releaseFence, requestConfig);
        if (error != SURFACE_ERROR_OK) {
            cout << "camera request buffer fail, ret=" << error << endl;
            return;
        }
        uint32_t buffSize = buffer->GetSize();
        void *bufferVirAddr = buffer->GetVirAddr();
        if (bufferVirAddr == nullptr) {
            cout << "bufferVirAddr is nullptr";
            return;
        }
        memset_s(bufferVirAddr, buffSize, 0, buffSize);
        BufferFlushConfig flushConfig = {
            .damage = {
                .x = 0,
                .y = 0,
                .w = requestConfig.width,
                .h = requestConfig.height,
            },
            .timestamp = 0,
        };
        error = window->GetSurface()->FlushBuffer(buffer, -1, flushConfig);
        if (error != SURFACE_ERROR_OK) {
            cout << "camera flush buffer fail, ret=" << error << endl;
            return;
        }

        subWindow = WindowManager::GetInstance()->CreateSubWindow(window->GetWindowID(), &subConfig);
        subWindow->GetSurface()->SetQueueSize(queueSize);
        sptr<Surface> consumerSurface = subWindow->GetSurface();
        consumerSurface->SetUserData("surface_stride_Alignment", "8");
        consumerSurface->SetUserData("surface_format", to_string(PIXEL_FMT_YCBCR_422_SP));
        consumerSurface->SetUserData("surface_width", "720");
        consumerSurface->SetUserData("surface_height", "480");
        consumerSurface->SetUserData("region_position_x", "0");
        consumerSurface->SetUserData("region_position_y", "0");
        consumerSurface->SetUserData("region_width", "720");
        consumerSurface->SetUserData("region_height", "480");

        FrameConfig *fc = new FrameConfig(FRAME_CONFIG_PREVIEW);
        fc->AddSurface(*consumerSurface);
        int32_t ret = cam_->TriggerLoopingCapture(*fc);
        if (ret != 0) {
            delete fc;
            cout << "camera start preview failed. ret=" << ret << endl;
            return;
        }
        previewConfig = fc;
        previewState_ = STATE_RUNNING;
        cout << "camera start preview succeed." << endl;
    }

    class SurfaceListener : public IBufferConsumerListener {
    public:
        void OnBufferAvailable() override
        {
            cout << "SurfaceListener OnBufferAvailable";
        }
    };

    void Capture()
    {
        if (cam_ == nullptr) {
            cout << "Camera is not ready." << endl;
            return;
        }
        FrameConfig *fc = new FrameConfig(FRAME_CONFIG_CAPTURE);

        captureConSurface = Surface::CreateSurfaceAsConsumer();
        if (captureConSurface == nullptr) {
            delete fc;
            cout << "CreateSurface failed" << endl;
            return;
        }

        sptr<IBufferConsumerListener> listener = new SurfaceListener();
        captureConSurface->RegisterConsumerListener(listener);
        auto refSurface = captureConSurface; // to keep the reference count

        int usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
        captureConSurface->SetUserData("surface_width", "1920");
        captureConSurface->SetUserData("surface_height", "1080");
        captureConSurface->SetUserData("surface_stride_Alignment", "8");
        captureConSurface->SetUserData("surface_format", to_string(PIXEL_FMT_RGBA_8888));
        captureConSurface->SetUserData("surface_usage", to_string(usage));
        captureConSurface->SetUserData("surface_timeout", "0");
        fc->AddSurface(*refSurface);
        int32_t ret = cam_->TriggerSingleCapture(*fc);
        if (ret != 0) {
            cout << "TriggerSingleCapture failed" << endl;
            delete fc;
        }
    }

    void Stop()
    {
        if (cam_ == nullptr) {
            cout << "Camera is not ready." << endl;
            return;
        }
        cam_->StopLoopingCapture();
        if (recordState_ == STATE_RUNNING) {
            CloseRecorder();
        }
        if (recordConfig) {
            delete recordConfig;
            recordConfig = nullptr;
        }
        if (previewConfig) {
            delete previewConfig;
            previewConfig = nullptr;
        }
        recordState_ = STATE_IDLE;
        previewState_ = STATE_IDLE;
    }

private:
    enum State : int32_t { STATE_IDLE, STATE_RUNNING, STATE_BUTT };
    State previewState_ = STATE_IDLE;
    State recordState_ = STATE_IDLE;
    EventHandler &eventHdlr_;
    Camera *cam_ = nullptr;
    std::shared_ptr<Recorder> recorder_ = nullptr;
    int32_t videoSourceId = -1;
    SampleFrameStateCallback fsCb_;
    sptr<Surface> captureConSurface;
    std::unique_ptr<Window> window;
    std::unique_ptr<SubWindow> subWindow;
    FrameConfig *recordConfig = nullptr;
    FrameConfig *previewConfig = nullptr;
};

class SampleCameraDeviceCallback : public CameraDeviceCallback {
};

inline void SampleHelp()
{
    cout << "*******************************************" << endl;
    cout << "Select the behavior of avrecorder." << endl;
    cout << "1: Capture" << endl;
    cout << "2: Record(Press s to stop)" << endl;
    cout << "3: Preview(Press s to stop)" << endl;
    cout << "q: quit the sample." << endl;
    cout << "*******************************************" << endl;
}

int main()
{
    cout << "Camera sample begin." << endl;
    const int supportedPixWidth = 1920;
    const int supportedPixHeight = 1080;

    SampleHelp();
    CameraKit *camKit = CameraKit::GetInstance();
    if (camKit == nullptr) {
        cout << "Can not get CameraKit instance" << endl;
        return 0;
    }
    list<string> camList = camKit->GetCameraIds();
    string camId;
    for (auto &cam : camList) {
        cout << "camera name:" << cam << endl;
        const CameraAbility *ability = camKit->GetCameraAbility(cam);
        /* find camera which fits user's ability */
        list<CameraPicSize> sizeList = ability->GetSupportedSizes();
        for (CameraPicSize picSize: sizeList) {
            cout<< "\npicSize.width= "<< picSize.width;
            cout<< "\npicSize.height= "<< picSize.height;
            if ((picSize.width == supportedPixWidth) &&
                (picSize.height ==  supportedPixHeight)) {
                cout << "\nGot the 1080p camera";
                camId = cam;
                break;
            }
        }
    }

    if (camId.empty()) {
        cout << "No available camera.(1080p wanted)" << endl;
        return 0;
    }

    EventHandler eventHdlr; // Create a thread to handle callback events
    SampleCameraStateMng camStateMng(eventHdlr);

    camKit->CreateCamera(camId, camStateMng, eventHdlr);

    char input;
    while (cin >> input) {
        switch (input) {
            case '1':
                camStateMng.Capture();
                break;
            case '2':
                camStateMng.StartRecord();
                break;
            case '3':
                camStateMng.StartPreview();
                break;
            case 's':
                camStateMng.Stop();
                break;
            case 'q':
                camStateMng.Stop();
                cout << "Camera sample end." << endl;
                return 0;
            default:
                SampleHelp();
                break;
        }
    }
}
