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

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "input/camera_manager.h"
#include "input/camera_input.h"
#include "media_log.h"
#include "surface.h"
#include "display_type.h"
#include <securec.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::CameraStandard;

static sptr<Surface> captureConSurface;

int32_t SaveYUVPreview(char *buffer, int32_t size)
{
    MEDIA_DEBUG_LOG("HStreamCapture::SaveYUVPreview is called");
    std::ostringstream ss("Preview_raw.yuv");
    std::ofstream preview;
    preview.open("/data/" + ss.str(), std::ofstream::out | std::ofstream::app);
    MEDIA_DEBUG_LOG("saving file to /data/%{public}s", ss.str().c_str());
    preview.write(buffer, size);
    MEDIA_DEBUG_LOG("Saving preview end");
    preview.close();
    return 0;
}
class SurfaceListener : public IBufferConsumerListener {
public:
    void OnBufferAvailable() override
    {
        int32_t flushFence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        MEDIA_DEBUG_LOG("SurfaceListener OnBufferAvailable");
        OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        captureConSurface->AcquireBuffer(buffer, flushFence, timestamp, damage);
        if (buffer != nullptr) {
            char *addr = static_cast<char *>(buffer->GetVirAddr());
            uint32_t size = buffer->GetSize();
            MEDIA_DEBUG_LOG("Calling SaveYUV");
            SaveYUVPreview(addr, size);
            captureConSurface->ReleaseBuffer(buffer, -1);
        } else {
            MEDIA_DEBUG_LOG("AcquireBuffer failed!");
        }
    }
};

int main()
{
    int32_t intResult = -1;
    MEDIA_DEBUG_LOG("Camera new sample begin.");

    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    MEDIA_DEBUG_LOG("Camera ID count: %{public}d", cameraObjList.size());
    for (auto& it : cameraObjList) {
        MEDIA_DEBUG_LOG("Camera ID: %{public}s", it->GetID().c_str());
    }
    if (cameraObjList.size() > 0) {
        sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
        if (captureSession == nullptr) {
            MEDIA_DEBUG_LOG("Failed to create capture session");
        }
        captureSession->BeginConfig();
        sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
        if (cameraInput != nullptr) {
            intResult = captureSession->AddInput(cameraInput);
            if (0 == intResult) {
                captureConSurface = Surface::CreateSurfaceAsConsumer();
                sptr<IBufferConsumerListener> listener = new SurfaceListener();
                captureConSurface->RegisterConsumerListener(listener);
                sptr<CaptureOutput> previewOutput = camManagerObj->CreatePreviewOutput(captureConSurface);
                if (previewOutput == nullptr) {
                    MEDIA_DEBUG_LOG("Failed to create previewOutput");
                    return 0;
                }
                intResult = captureSession->AddOutput(previewOutput);
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to Add output to session, intResult: %{public}d", intResult);
                    return 0;
                }
                intResult = captureSession->CommitConfig();
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to Commit config, intResult: %{public}d", intResult);
                    return 0;
                }
                intResult = captureSession->Start();
                if (intResult != 0){
                    MEDIA_DEBUG_LOG("Failed to start, intResult: %{public}d", intResult);
                    return 0;
                }
                MEDIA_DEBUG_LOG("Waiting for 10 seconds begin");
                sleep(10);
                MEDIA_DEBUG_LOG("Waiting for 10 seconds end");
                captureSession->Stop();
                captureSession->Release();
            } else {
                MEDIA_DEBUG_LOG("Add input to session is failed, intResult: %{public}d", intResult);
            }
        } else {
            MEDIA_DEBUG_LOG("Failed to create Camera Input");
        }
    }
    MEDIA_DEBUG_LOG("Camera new sample end.");
    return 0;
}
