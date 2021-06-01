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
#include "camera_manager.h"

using namespace std;
namespace OHOS {
namespace Media {
CameraManager *g_cameraManager = nullptr;
CameraKit::CameraKit()
{
    g_cameraManager = CameraManager::GetInstance();
}

CameraKit::~CameraKit() {}

CameraKit *CameraKit::GetInstance()
{
    static CameraKit kit;
    return &kit;
}

list<string> CameraKit::GetCameraIds()
{
    return g_cameraManager->GetCameraIds();
}

const CameraAbility *CameraKit::GetCameraAbility(const string cameraId)
{
    return g_cameraManager->GetCameraAbility(cameraId);
}

void CameraKit::RegisterCameraDeviceCallback(CameraDeviceCallback &callback, EventHandler &handler)
{
    g_cameraManager->RegisterCameraDeviceCallback(callback, handler);
}

void CameraKit::UnregisterCameraDeviceCallback(CameraDeviceCallback &callback)
{
    g_cameraManager->UnregisterCameraDeviceCallback(callback);
}

void CameraKit::CreateCamera(const string &cameraId, CameraStateCallback &callback, EventHandler &handler)
{
    g_cameraManager->CreateCamera(cameraId, callback, handler);
}
} // namespace Media
} // namespace OHOS
