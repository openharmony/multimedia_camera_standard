/*
* Copyright (C) 2021 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
import {Callback, ErrorCallback, AsyncCallback} from './basic';

/**
 * @name camera
 * @since 6
 * @sysCap SystemCapability.Multimedia.Camera
 * @import import camera from '@ohos.Multimedia.Camera';
 * @permission
 */
declare namespace camera {
  /**
   * Create the Camera instance to manage camera.
   * @sysCap SystemCapability.Multimedia.Camera
   * @devices
   */
  function createCamera(id: string): Camera;

  /**
   * Gets the list of available Camera IDs.
   * @sysCap SystemCapability.Multimedia.Camera
   * @devices
   */
  function getCameraIds(callback: AsyncCallback<CameraIdList>): void;

  /**
   * Gets the list of available Camera IDs.
   * @sysCap SystemCapability.Multimedia.Camera
   * @devices
   */
  function getCameraIds(): Promise<CameraIdList>;

  enum CameraPosition {
    /**
     * 未指定位置
     */
    UNSPECIFIED = 1,

    /**
     * 后置摄像头
     */
    BACK_CAMERA = 2,

    /**
     * 前置摄像头
     */
    FRONT_CAMERA = 3,
  }

  enum CameraType {
    /**
     * 未指定类型
     */
    UNSPECIFIED = 1,

    /**
     * 广角摄像头
     */
    WIDE_ANGLE = 2,

    /**
     * 超广角摄像头
     */
    ULTRA_WIDE = 3,

    /**
     * 长焦摄像头
     */
    TELEPHOTO = 4,

    /**
     * 深度摄像头
     */
    TRUE_DEPTH = 5,

    /**
     * 逻辑摄像头
     */
    LOGICAL = 6,
  }

  enum ExposureMode {
    /**
     * 手动曝光，设备仅应根据用户提供的曝光时长调整曝光
     */
    MANUAL = 1,

    /**
     * 设备会连续监控曝光水平，并在必要时自动曝光
     */
    CONTINUOUS_AUTO_EXPOSURE = 2,
  }

  enum FocusMode {
    /**
     * 手动对焦
     */
    MANUAL = 1,

    /**
     * 设备会连续监视焦点并在必要时自动聚焦
     */
    CONTINUOUS_AUTO_FOCUS = 2,

    /**
     * 设备会自动调整一次对焦
     */
    AUTO_FOCUS = 3,

    /**
     * 锁定对焦
     */
    LOCKED = 4,
  }

  enum FlashMode {
    /**
     * 关闭
     */
    CLOSE = 1,

    /**
     * 打开
     */
    OPEN = 2,
  }

  enum FileFormat {
    /**
     * MPEG4 format
     */
    MP4 = 1,
  }

  enum VideoEncoder {
    /**
     * H.264/MPEG-4 AVC
     */
    H264 = 1,
  }

  enum AudioEncoder {
    /**
     * Advanced Audio Coding Low Complexity(AAC-LC)
     */
    AAC_LC = 1,
  }

  interface RecorderConfig {
    /**
     * Output video file path.
     * @devices
     */
    videoPath: string;

    /**
     * Video file thumbnail path.
     * @devices
     */
    thumbPath: string;

    /**
     * Mute the voice.
     * @devices
     */
    muted?: boolean;

    /**
     * RecorderProfile.
     * @devices
     */
    profile: RecorderProfile;
  }

  interface PhotoConfig {
    /**
     * 生成照片的路径。
     * @devices
     */
    photoPath: string;

    /**
     * 镜像模式。
     * @devices
     */
    mirror?: boolean;
  }

  /**
   * the List of Camera IDs
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  type CameraIdList = Array<string>;

  /**
   * the List of Supported Exposure Modes
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  type SupportedExposureModesList = Array<ExposureMode>;

  /**
   * the List of Supported Focus Modes
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  type SupportedFocusModesList = Array<FocusMode>; 

  /**
   * the List of Supported Flash Modes
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  type SupportedFlashModesList = Array<FlashMode>;

  /**
   * the List of Supported Zoom Ranges
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  type SupportedZoomRangeList = Array<number>;

  enum AudioSourceType {
    /**
     * Microphone
     */
     MIC = 1,
  }

  enum VideoSourceType {
    /**
     * Camera
     */
     CAMERA = 1,
  }

  interface RecorderProfile {
    /**
     * Audio source type.
     * @devices
     */
     readonly audioSourceType?: AudioSourceType;

     /**
     * Video source type.
     * @devices
     */
      readonly videoSourceType?: VideoSourceType;

    /**
     * Indicates the audio bit rate.
     * @devices
     */
    readonly aBitRate: number;

    /**
     * Indicates the number of audio channels.
     * @devices
     */
    readonly aChannels: number;

    /**
     * Indicates the audio encoding format.
     * @devices
     */
    readonly aCodec: AudioEncoder;

    /**
     * Indicates the audio sampling rate.
     * @devices
     */
    readonly aSampleRate: number;

    /**
     * Indicates the default recording duration.
     * @devices
     */
    readonly durationTime: number;

    /**
     * Indicates the output file format.
     * @devices
     */
    readonly fileFormat: FileFormat;

    /**
     * Indicates the video bit rate.
     * @devices
     */
    readonly vBitRate: number;

    /**
     * Indicates the video encoding format.
     * @devices
     */
    readonly vCodec: VideoEncoder;

    /**
     * Indicates the video height.
     * @devices
     */
    readonly vFrameHeight: number;

    /**
     * Indicates the video frame rate.
     * @devices
     */
    readonly vFrameRate: number;

    /**
     * Indicates the video width.
     * @devices
     */
    readonly vFrameWidth: number;
  }

  interface Camera {
    /**
     * Camera position.
     * @devices
     */
    readonly position: CameraPosition;

    /**
     * Camera type.
     * @devices
     */
    readonly type: CameraType;

    /**
     * Exposure Mmode.
     * @devices
     */
    readonly exposure: ExposureMode;

    /**
     * Focus mode.
     * @devices
     */
    readonly focus: FocusMode;

    /**
     * Flash mode.
     * @devices
     */
    readonly flash: FlashMode;

    /**
     * Zoom value.
     * @devices
     */
    readonly zoom: number;

    /**
     * Preapre recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    prepare(flowTypeFlag: number, config: RecorderConfig): void;

    /**
     * Start recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    startVideoRecording(): void;

    /**
     * Pause recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    pauseVideoRecording(): void;

    /**
     * Resume recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    resumeVideoRecording(): void;

    /**
     * Stop recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    stopVideoRecording(): void;

    /**
     * Reset recorder.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    resetVideoRecording(): void;

    /**
     * Starts the preview
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    startPreview(): void;

    /**
     * Stops the preview
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    stopPreview(): void;

    /**
     * Takes a photo.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    takePhoto(config: PhotoConfig, callback: AsyncCallback<void>): void;

    /**
     * Takes a photo.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    takePhoto(config: PhotoConfig): Promise<void>;

    /**
     * Gets the supported exposure mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedExposureMode(callback: AsyncCallback<SupportedExposureModesList>): void;

    /**
     * Gets the supported exposure mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedExposureMode(): Promise<SupportedExposureModesList>;

    /**
     * Sets the exposure mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setExposureMode(mode: ExposureMode, callback: AsyncCallback<void>): void;

    /**
     * Sets the exposure mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setExposureMode(mode: ExposureMode): Promise<void>;

    /**
     * Gets the supported focus mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedFocusMode(callback: AsyncCallback<SupportedFocusModesList>): void;

    /**
     * Gets the supported focus mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedFocusMode(): Promise<SupportedFocusModesList>;

    /**
     * Sets the focus mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setFocusMode(mode: FocusMode, callback: AsyncCallback<void>): void;

    /**
     * Sets the focus mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setFocusMode(mode: FocusMode): Promise<void>;

    /**
     * Gets the supported flash mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedFlashMode(callback: AsyncCallback<SupportedFlashModesList>): void;

    /**
     * Gets the supported flash mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedFlashMode(): Promise<SupportedFlashModesList>;

    /**
     * Sets the flash mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setFlashMode(mode: FlashMode, callback: AsyncCallback<void>): void;

    /**
     * Sets the flash mode.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setFlashMode(mode: FlashMode): Promise<void>;

    /**
     * Gets the zoom range supported by the camera.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedZoomRange(callback: AsyncCallback<SupportedZoomRangeList>): void;

    /**
     * Gets the zoom range supported by the camera.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedZoomRange(): Promise<SupportedZoomRangeList>;

    /**
     * Sets a zoom value.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setZoom(value: number, callback: AsyncCallback<void>): void;

    /**
     * Sets a zoom value.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    setZoom(value: number): Promise<void>;

    /**
     * Called when prepare, start, pause, resume, stop or reset event complete.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'prepare' | 'start_video' | 'pause_video' | 'resume_video' | 'stop_video' | 'reset_video' | 'start_preview' | 'stop_preview', callback: ()=>{}): void;

    /**
     * Called when an error has occurred.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'error', callback: ErrorCallback): void;
  }
}

export default camera;

