# Camera<a name="EN-US_TOPIC_0000001101564782"></a>

- [Camera<a name="EN-US_TOPIC_0000001101564782"></a>](#camera)
  - [Introduction<a name="section11660541593"></a>](#introduction)
    - [Basic Concepts](#basic-concepts)
  - [Directory Structure<a name="section176641621345"></a>](#directory-structure)
  - [Usage Guidelines<a name="usage-guidelines"></a>](#usage-guidelines)
    - [To Capture a Photo](#to-capture-a-photo)
    - [To Start and Stop Preview](#to-start-and-stop-preview)
    - [To Record Video](#to-record-video)
  - [Repositories Involved<a name="section16511040154318"></a>](#repositories-involved)

## Introduction<a name="section11660541593"></a>

The  **camera\_standard**  repository enables your application to provide camera functions. You can access and operate the camera device and develop new functions through open APIs. Common operations include preview, photographing and video recording.

### Basic Concepts

-   Capture Photo

    This function is used to capture a photo.

-   Preview

    This function repeatedly captures multiple frames in the buffer once the camera is started. It is used to preview the subject before capturing a photo or start recording a video.

-   Record Video

    This function repeatedly captures multiple frames in the buffer once the recording is started and continues till the recording is stopped. It is used for video recording.

**Figure  1**  Position in the subsystem architecture<a name="fig310889397"></a>
![](figures/position-in-the-subsystem-architecture.png "position-in-the-subsystem-architecture")

## Directory Structure<a name="section176641621345"></a>

The structure of the repository directory is as follows:

```
├── frameworks                          # Framework code
│   ├── innerkitsimpl                   # Internal Native API Implementation
│   │   ├── camera                      # Camera Framework Implementation
│   │   └── metadata                    # Metadata Implementation
│   └── kitsimpl                        # External JS API Implementation
│       └── camera_napi                 # Camera NAPI Implementation
├── interfaces                          # Interfaces
│   ├── innerkits                       # Internal Native APIs
│   └── kits                            # External JS APIs
├── LICENSE                             # License file
├── ohos.build                          # Build file
├── sa_profile                          # Service configuration profile
└── services                            # Service code
    ├── camera_service                  # Camera Service Implementation
    └── etc                             # Camera Service Init Configuration
```


## Usage Guidelines<a name="usage-guidelines"></a>
### To Capture a Photo
Steps to capture a photo:
1. Create the buffer consumer listener(**CaptureSurfaceListener**) to save the image.
```
    class CaptureSurfaceListener : public IBufferConsumerListener {
        public:
        int32_t mode_;
        sptr<Surface> surface_;

        void OnBufferAvailable() override
        {
            int32_t flushFence = 0;
            int64_t timestamp = 0;
            OHOS::Rect damage; // initialize the damage

            OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
            surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
            if (buffer != nullptr) {
                void *addr = buffer->GetVirAddr();
                int32_t size = buffer->GetSize();

                // Save the buffer(addr) to a file.

                surface_->ReleaseBuffer(buffer, -1);
            }
        }
    };
```

2. Get the Camera Manager Instance and retrieve the camera object list.
```
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
```

3. Create the Capture Session
```
    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    if (captureSession == nullptr) {
        MEDIA_ERR_LOG("Failed to create capture session");
        return captureSession;
    }
```

4. Begin configuring the Capture Session.
```
    int32_t result = captureSession->BeginConfig();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to begin configuration");
        return result;
    }
```

5. Create the Camera Input using the Camera Object.
```
    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    if (cameraInput == nullptr) {
        MEDIA_ERR_LOG("Failed to create camera input");
        return cameraInput;
    }
```

6. Add the Camera Input to the Capture Session.
```
    result = captureSession->AddInput(cameraInput);
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to add input to session");
        return result;
    }
```

7. Create the Consumer Surface and register listerner for buffer updates. The photo width and height can be configured to the supported resolution which is 1290x960.
```
    sptr<Surface> photoSurface = Surface::CreateSurfaceAsConsumer();
    int32_t photoWidth = 1290;
    int32_t photoHeight = 960;
    photoSurface->SetDefaultWidthAndHeight(photoWidth, photoHeight);
    sptr<CaptureSurfaceListener> capturelistener = new CaptureSurfaceListener();
    capturelistener->mode_ = MODE_PHOTO;
    capturelistener->surface_ = photoSurface;
    photoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)capturelistener);
```

8. Create a Photo Output with Surface created above.
```
    sptr<CaptureOutput> photoOutput = camManagerObj->CreatePhotoOutput(photoSurface);
    if (photoOutput == nullptr) {
        MEDIA_ERR_LOG("Failed to create photo output");
        return photoOutput;
    }
```

9. Add the Photo Output to the Capture Session.
```
    result = captureSession->AddOutput(photoOutput);
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to add output to session");
        return result;
    }
```

10. Commit the configuration to Capture Session.
```
    result = captureSession->CommitConfig();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to commit configuration");
        return result;
    }
```

11. Capture the Photo.
```
    result = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to capture");
        return result;
    }
```

12. Release the Capture Session resources.
```
    captureSession->Release();
```
### To Start and Stop Preview
Steps to start and stop preview:
1. Get the Camera Manager Instance and retrieve the camera object list.
```
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
```

2. Create the Capture Session
```
    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    if (captureSession == nullptr) {
        MEDIA_ERR_LOG("Failed to create capture session");
        return captureSession;
    }
```

3. Begin configuring the Capture Session.
```
    int32_t result = captureSession->BeginConfig();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to begin configuration");
        return result;
    }
```

4. Create the Camera Input using the Camera Object.
```
    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    if (cameraInput == nullptr) {
        MEDIA_ERR_LOG("Failed to create camera input");
        return cameraInput;
    }
```

5. Add the Camera Input to the Capture Session.
```
    result = captureSession->AddInput(cameraInput);
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to add input to session");
        return result;
    }
```

6. Create a Preview Output with Surface obtained from Window Manager to render on display. The preview width and Height can be configured as per the supported resolutions which are 640x480 or 832x480. And to save to a file, can follow the step to create Surface and register listerner for buffer updates mentioned in capture photo.
```
    int32_t previewWidth = 640;
    int32_t previewHeight = 480;
    sptr<CaptureOutput> previewOutput = camManagerObj->CreateCustomPreviewOutput(previewSurface, previewWidth, previewHeight);
    if (previewOutput == nullptr) {
        MEDIA_ERR_LOG("Failed to create preview output");
        return previewOutput;
    }
```

7. Add the Preview Output to the Capture Session.
```
    result = captureSession->AddOutput(previewOutput);
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to add output to session");
        return result;
    }
```

8. Commit the configuration to Capture Session.
```
    result = captureSession->CommitConfig();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to commit configuration");
        return result;
    }
```

9. Start the Preview
```
    result = captureSession->Start();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to start");
        return result;
    }
```

10. Stop the Preview when needed.
```
    result = captureSession->Stop();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to stop");
        return result;
    }
```


11. Release the Capture Session resources.
```
    captureSession->Release();
```

### To Record Video
Steps to record Video:
1. Get the Camera Manager Instance and retrieve the camera object list.
```
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
```

2. Create the Capture Session
```
    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    if (captureSession == nullptr) {
        MEDIA_ERR_LOG("Failed to create capture session");
        return captureSession;
    }
```

3. Begin configuring the Capture Session.
```
    int32_t result = captureSession->BeginConfig();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to begin configuration");
        return result;
    }
```

4. Create the Camera Input using the Camera Object.
```
    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    if (cameraInput == nullptr) {
        MEDIA_ERR_LOG("Failed to create camera input");
        return cameraInput;
    }
```

5. Add the Camera Input to the Capture Session.
```
    result = captureSession->AddInput(cameraInput);
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to add input to session");
        return result;
    }
```

6. Create a Video Output with Surface obtained from Recoder to MUX with audio and save the file. And to save just Video buffer to a file, can follow the step to create Surface and register listerner for buffer updates mentioned in capture photo. The video resolution can be configured from supported resolutions which are 1280x720 or 640x360 while setting the recorder configurations.
```
    sptr<CaptureOutput> videoOutput = camManagerObj->CreateVideoOutput(videoSurface);
    if (videoOutput == nullptr) {
        MEDIA_ERR_LOG("Failed to create video output");
        return videoOutput;
    }
```

7. Add the Video Output to the Capture Session.
```
    result = captureSession->AddOutput(videoOutput);
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to add output to session");
        return result;
    }
```

8. Commit the configuration to Capture Session.
```
    result = captureSession->CommitConfig();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to commit configuration");
        return result;
    }
```

9. Start the Video Recording.
```
    result = ((sptr<VideoOutput> &)videoOutput)->Start();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to start");
        return result;
    }
```

10. Stop the recording when needed.
```
    result = ((sptr<VideoOutput> &)videoOutput)->Stop();
    if (result != 0) {
        MEDIA_ERR_LOG("Failed to stop");
        return result;
    }
```

11. Release the Capture Session resources.
```
    captureSession->Release();
```

## Repositories Involved<a name="section16511040154318"></a>

multimedia\_camera\_standard

