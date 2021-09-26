# Camera组件<a name="ZH-CN_TOPIC_0000001101564782"></a>

-   [简介](#section11660541593)
-   [目录](#section176641621345)
-   [使用说明](#section45377346241)
    -   [拍照](#section2071191842718)
    -   [开始和停止预览](#section2094314213271)
    -   [视频录像](#section1983913118271)

-   [相关仓](#section16511040154318)

## 简介<a name="section11660541593"></a>

相机组件支持相机业务的开发，开发者可以通过已开放的接口实现相机硬件的访问、操作和新功能开发，最常见的操作如：预览、拍照和录像等。

**图 1**  相机组件架构图<a name="fig310889397"></a>

![](figures/camera-architecture-zh.png "camera-architecture-zh")

## 目录<a name="section176641621345"></a>

仓目录结构如下：

```
/foundation/multimedia/camera_standard   # 相机组件业务代码
├── frameworks                           # 框架代码
│   ├── innerkitsimpl                    # 内部接口实现
│   │   ├── camera                       # 相机框架实现
│   │   └── metadata                     # 元数据实现
│   └── kitsimpl                         # 外部接口实现
│   └── camera_napi                      # 相机NAPI实现
├── interfaces                           # 接口代码
│   ├── innerkits                        # 内部接口
│   └── kits                             # 外部接口
├── LICENSE                              # 许可证文件
├── ohos.build                           # 构建文件
├── sa_profile                           # 服务配置文件
└── services                             # 服务代码
    ├── camera_service                   # 相机服务实现
    └── etc                              # 相机服务配置
```

## 使用说明<a name="section45377346241"></a>

### 拍照<a name="section2071191842718"></a>

1.  创建缓冲区消费者端监听器（CaptureSurfaceListener）以保存图像。

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

2.  获取相机管理器实例并获取相机对象列表。

    ```
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    ```

3.  创建采集会话。

    ```
    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    ```

4.  开始配置采集会话。

    ```
    int32_t result = captureSession->BeginConfig();
    ```

5.  使用相机对象创建相机输入。

    ```
    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    ```

6.  将相机输入添加到采集会话。

    ```
    result = captureSession->AddInput(cameraInput);
    ```

7.  创建消费者Surface并注册监听器以监听缓冲区更新。

    ```
    sptr<Surface> photoSurface = Surface::CreateSurfaceAsConsumer(); 
    sptr<CaptureSurfaceListener> capturelistener = new CaptureSurfaceListener(); 
    capturelistener->mode_ = MODE_PHOTO; 
    capturelistener->surface_ = photoSurface; 
    photoSurface->RegisterConsumerListener((sptr<IBufferConsumerListener> &)capturelistener);
    ```

8.  使用上面创建的 Surface 创建拍照输出。

    ```
    sptr<CaptureOutput> photoOutput = camManagerObj->CreatePhotoOutput(photoSurface);
    ```

9.  将拍照输出添加到采集会话。

    ```
    result = captureSession->AddOutput(photoOutput);
    ```

10. 将配置提交到采集会话。

    ```
    result = captureSession->CommitConfig();
    ```

11. 拍摄照片。

    ```
    result = ((sptr<PhotoOutput> &)photoOutput)->Capture();
    ```

12. 释放采集会话资源。

    ```
    captureSession->Release();
    ```


### 开始和停止预览<a name="section2094314213271"></a>

1.  获取相机管理器实例并获取相机对象列表。

    ```
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance(); 
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    ```

2.  创建采集会话。

    ```
    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    ```

3.  开始配置采集会话。

    ```
    int32_t result = captureSession->BeginConfig();
    ```

4.  使用相机对象创建相机输入。

    ```
    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    ```

5.  将相机输入添加到采集会话。

    ```
    result = captureSession->AddInput(cameraInput); 
    ```

6.  使用从窗口管理器获得的Surface创建预览输出用以在显示上渲染。如果想保存到文件，可以按照拍照流程提到步骤，创建 Surface，注册监听器以监听缓冲区更新。

    ```
    sptr<CaptureOutput> previewOutput = camManagerObj->CreatePreviewOutput(previewSurface); 
    ```

7.  将预览输出添加到采集会话。

    ```
    result = captureSession->AddOutput(previewOutput);
    ```

8.  将配置提交到采集会话。

    ```
    result = captureSession->CommitConfig();
    ```

9.  开始预览。

    ```
    result = captureSession->Start();
    ```

10. 需要时停止预览。

    ```
    result = captureSession->Stop();
    ```

11. 释放采集会话资源。

    ```
    captureSession->Release();
    ```


### 视频录像<a name="section1983913118271"></a>

1.  获取相机管理器实例并获取相机对象列表。

    ```
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraInfo>> cameraObjList = camManagerObj->GetCameras();
    ```

2.  创建采集会话。

    ```
    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    ```

3.  开始配置采集会话。

    ```
    int32_t result = captureSession->BeginConfig();
    ```

4.  使用相机对象创建相机输入。

    ```
    sptr<CaptureInput> cameraInput = camManagerObj->CreateCameraInput(cameraObjList[0]);
    ```

5.  将相机输入添加到采集会话。

    ```
    result = captureSession->AddInput(cameraInput);
    ```

6.  通过Surface创建一个视频输出，来与音频合成并保存到文件，Surface通过Recoder获取。如果想仅保存视频缓冲数据到文件里，可以按照拍照流程提到步骤，创建 Surface，注册监听器以监听缓冲区更新。

    ```
    sptr<CaptureOutput> videoOutput = camManagerObj->CreateVideoOutput(videoSurface);
    ```

7.  将视频输出添加到采集会话。

    ```
    result = captureSession->AddOutput(videoOutput);
    ```

8.  将配置提交到采集会话。

    ```
    result = captureSession->CommitConfig();
    ```

9.  开始视频录制。

    ```
    result = ((sptr<VideoOutput> &)videoOutput)->Start();
    ```

10. 需要时停止录制。

    ```
    result = ((sptr<VideoOutput> &)videoOutput)->Stop();
    ```

11. 释放采集会话的资源。

    ```
    captureSession->Release();
    ```


## 相关仓<a name="section16511040154318"></a>

[multimedia\_camera\_standard](https://gitee.com/openharmony/multimedia_camera_standard)

