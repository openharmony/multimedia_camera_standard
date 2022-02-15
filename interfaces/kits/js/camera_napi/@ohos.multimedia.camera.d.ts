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

import {ErrorCallback, AsyncCallback} from './basic';
import Context from './@ohos.ability';

/**
 * @name camera
 * @SysCap SystemCapability.Multimedia.Camera
 * @devices phone, tablet, wearable, car
 * @since 8
 */
declare namespace camera {

  /**
   * Creates a CameraManager instance.
   * @param context Current application context.
   * @return CameraManager instance.
   * @since 8
   */
  function getCameraManager(context: Context, callback: AsyncCallback<CameraManager>): void;
  function getCameraManager(context: Context): Promise<CameraManager>;

  /**
   * Enum for camera status.
   * @since 8
   */
  enum CameraStatus {
    /**
     * Appear status.
     * @since 8
     */
    CAMERA_STATUS_APPEAR = 0,
    /**
     * Disappear status.
     * @since 8
     */
    CAMERA_STATUS_DISAPPEAR,
    /**
     * Available status.
     * @since 8
     */
    CAMERA_STATUS_AVAILABLE,
    /**
     * Unavailable status.
     * @since 8
     */
    CAMERA_STATUS_UNAVAILABLE
  }

  /**
   * Camera manager object.
   * @since 8
   */
  interface CameraManager  {
    /**
     * Gets all camera descriptions.
     * @return All camera descriptions.
     * @since 8
     */
    getCameras(callback: AsyncCallback<Array<Camera>>): void;
    getCameras(): Promise<Array<Camera>>;

    /**
     * Creates a CameraInput instance by camera id.
     * @param cameraId Target camera id.
     * @return CameraInput instance.
     * @since 8
     */
    createCameraInput(cameraId: string, callback: AsyncCallback<CameraInput>): void;
    createCameraInput(cameraId: string): Promise<CameraInput>;

    /**
     * Creates a CameraInput instance by camera position and type.
     * @param position Target camera position.
     * @param type Target camera type.
     * @return CameraInput instance.
     * @since 8
     */
    createCameraInput(position: CameraPosition, type: CameraType, callback: AsyncCallback<CameraInput>): void;
    createCameraInput(position: CameraPosition, type: CameraType): Promise<CameraInput>;

    /**
     * Subscribes camera status change event callback.
     * @param type Event type.
     * @return CameraStatusInfo event callback.
     * @since 8
     */
    on(type: "cameraStatus", callback: AsyncCallback<CameraStatusInfo>): void;
  }

  /**
   * Camera status info.
   * @since 8
   */
  interface CameraStatusInfo {
    /**
     * Camera instance.
     * @since 8
     */
    camera: Camera;
    /**
     * Current camera status.
     * @since 8
     */
    status: CameraStatus;
  }

  /**
   * Enum for camera position.
   * @since 8
   */
  enum CameraPosition {
    /**
     * Unspecified position.
     * @since 8
     */
    CAMERA_POSITION_UNSPECIFIED = 0,
    /**
     * Back position.
     * @since 8
     */
    CAMERA_POSITION_BACK,
    /**
     * Front position.
     * @since 8
     */
    CAMERA_POSITION_FRONT
  }

  /**
   * Enum for camera type.
   * @since 8
   */
  enum CameraType {
    CAMERA_TYPE_UNSPECIFIED = 0,
    CAMERA_TYPE_WIDE_ANGLE,
    CAMERA_TYPE_ULTRA_WIDE,
    CAMERA_TYPE_TELEPHOTO,
    CAMERA_TYPE_TRUE_DEAPTH
  }

  /**
   * Enum for camera connection type.
   * @since 8
   */
  enum ConnectionType {
    CAMERA_CONNECTION_BUILT_IN = 0,
    CAMERA_CONNECTION_USB_PLUGIN,
    CAMERA_CONNECTION_REMOTE
  }

  /**
   * Camera object.
   * @since 8
   */
  interface Camera {
    /**
     * Camera id attribute.
     * @since 8
     */
    readonly cameraId: string;
    /**
     * Camera position attribute.
     * @since 8
     */
    readonly cameraPosition: CameraPosition;
    /**
     * Camera type attribute.
     * @since 8
     */
    readonly cameraType: CameraType;
    /**
     * Camera connection type attribute.
     * @since 8
     */
    readonly connectionType: ConnectionType;
  }

  /**
   * Size parameter.
   * @since 8
   */
  interface Size {
    /**
     * Height.
     * @since 8
     */
    height: number;
    /**
     * Width.
     * @since 8
     */
    width: number;
  }

  /**
   * Enum for camera data format. Align to pixel format and image format value.
   * @since 8
   */
  enum CameraFormat {
    /**
     * YCRCb 420 SP format.
     * @since 8
     */
    CAMERA_FORMAT_YCRCb_420_SP = 1003,

    /**
     * JPEG format.
     * @since 8
     */
    CAMERA_FORMAT_JPEG = 2000,
  }

  /**
   * Camera input object.
   * @since 8
   */
  interface CameraInput {
    /**
     * Gets camera id.
     * @return Camera id.
     * @since 8
     */
    getCameraId(callback: AsyncCallback<string>): void;
    getCameraId(): Promise<string>;

    /**
     * Gets all supported sizes for current camera input.
     * @return Supported size array.
     * @since 8
     */
    getSupportedSizes(format: CameraFormat, callback: AsyncCallback<Array<Size>>): void;
    getSupportedSizes(format: CameraFormat): Promise<Array<Size>>;

    /**
     * Gets all supported formats for current camera input.
     * @return Supported format array.
     * @since 8
     */
    getSupportedPreviewFormats(callback: AsyncCallback<Array<CameraFormat>>): void;
    getSupportedPreviewFormats(): Promise<Array<CameraFormat>>;
    getSupportedPhotoFormats(callback: AsyncCallback<Array<CameraFormat>>): void;
    getSupportedPhotoFormats(): Promise<Array<CameraFormat>>;
    getSupportedVideoFormats(callback: AsyncCallback<Array<CameraFormat>>): void;
    getSupportedVideoFormats(): Promise<Array<CameraFormat>>;

    /**
     * Check if device has flash light.
     * @return Flash light has or not.
     * @since 8
     */
    hasFlash(callback: AsyncCallback<boolean>): void;
    hasFlash(): Promise<boolean>;

    /**
     * Gets all supported flash modes for current camera input.
     * @return Supported flash mode array.
     * @since 8
     */
    isFlashModeSupported(flashMode: FlashMode, callback: AsyncCallback<boolean>): void;
    isFlashModeSupported(flashMode: FlashMode): Promise<boolean>;

    /**
     * Gets current flash mode.
     * @return Current flash mode.
     * @since 8
     */
    getFlashMode(callback: AsyncCallback<FlashMode>): void;
    getFlashMode(): Promise<FlashMode>;

    /**
     * Sets flash mode.
     * @param flashMode Target flash mode.
     * @since 8
     */
    setFlashMode(flashMode: FlashMode, callback: AsyncCallback<void>): void;
    setFlashMode(flashMode: FlashMode): Promise<void>;

    /**
     * Gets all supported exposure modes for current camera input.
     * @return Supported exposure mode array.
     * @since 8
     */
    isExposureModeSupported(aeMode: ExposureMode, callback: AsyncCallback<boolean>): void;
    isExposureModeSupported(aeMode: ExposureMode): Promise<boolean>;

    /**
     * Gets current exposure mode.
     * @return Current exposure mode.
     * @since 8
     */
    getExposureMode(callback: AsyncCallback<ExposureMode>): void;
    getExposureMode(): Promise<ExposureMode>;

    /**
     * Sets exposure mode.
     * @param aeMode Target exposure mode.
     * @since 8
     */
    setExposureMode(aeMode: ExposureMode, callback: AsyncCallback<void>): void;
    setExposureMode(aeMode: ExposureMode): Promise<void>;

    /**
     * Gets all supported focus modes for current camera input.
     * @return Supported focus mode array.
     * @since 8
     */
    isFocusModeSupported(afMode: FocusMode, callback: AsyncCallback<boolean>): void;
    isFocusModeSupported(afMode: FocusMode): Promise<boolean>;

    /**
     * Gets current focus mode.
     * @return Current focus mode.
     * @since 8
     */
    getFocusMode(callback: AsyncCallback<FocusMode>): void;
    getFocusMode(): Promise<FocusMode>;

    /**
     * Sets focus mode.
     * @param afMode Target focus mode.
     * @since 8
     */
    setFocusMode(afMode: FocusMode, callback: AsyncCallback<void>): void;
    setFocusMode(afMode: FocusMode): Promise<void>;

    /**
     * Gets all supported zoom ratio range.
     * @param afMode Target focus mode.
     * @since 8
     */
    getZoomRatioRange(callback: AsyncCallback<Array<number>>): void;
    getZoomRatioRange(): Promise<Array<number>>;

    /**
     * Gets zoom ratio.
     * @return Current zoom ratio.
     * @since 8
     */
    getZoomRatio(callback: AsyncCallback<number>): void;
    getZoomRatio(): Promise<number>;

    /**
     * Sets zoom ratio.
     * @param zoomRatio Target zoom ratio.
     * @since 8
     */
    setZoomRatio(zoomRatio: number, callback: AsyncCallback<void>): void;
    setZoomRatio(zoomRatio: number): Promise<void>;

    /**
     * Releases instance.
     * @since 8
     */
    release(callback: AsyncCallback<void>): void;
    release(): Promise<void>;

    /**
     * Subscribes focus status change event callback.
     * @param type Event type.
     * @return FocusState event callback.
     * @since 8
     */
    on(type: "focusStateChange", callback: AsyncCallback<FocusState>): void;

    /**
     * Subscribes exposure status change event callback.
     * @param type Event type.
     * @return ExposureState event callback.
     * @since 8
     */
    on(type: "exposureStateChange", callback: AsyncCallback<ExposureState>): void;

    /**
     * Subscribes error event callback.
     * @param type Event type.
     * @return Error event callback.
     * @since 8
     */
    on(type: "error", callback: ErrorCallback<CameraInputError>): void;
  }

  enum CameraInputErrorCode {
    ERROR_UNKNOWN = -1
  }

  interface CameraInputError extends Error {
    code: CameraInputErrorCode;
  }

  /**
   * Enum for flash mode.
   * @since 8
   */
  enum FlashMode {
    /**
     * Close mode.
     * @since 8
     */
    FLASH_MODE_CLOSE = 0,
    /**
     * Open mode.
     * @since 8
     */
    FLASH_MODE_OPEN,
    /**
     * Auto mode.
     * @since 8
     */
    FLASH_MODE_AUTO,
    /**
     * Always open mode.
     * @since 8
     */
    FLASH_MODE_ALWAYS_OPEN
  }

  /**
   * Enum for exposure mode.
   * @since 8
   */
  enum ExposureMode {
    /**
     * Manual mode.
     * @since 8
     */
    EXPOSURE_MODE_MANUAL = 0,
    /**
     * Continuous auto mode.
     * @since 8
     */
    EXPOSURE_MODE_CONTINUOUS_AUTO
  }

  /**
   * Enum for focus mode.
   * @since 8
   */
  enum FocusMode {
    /**
     * Manual mode.
     * @since 8
     */
    FOCUS_MODE_MANUAL = 0,
    /**
     * Continuous auto mode.
     * @since 8
     */
    FOCUS_MODE_CONTINUOUS_AUTO,
    /**
     * Auto mode.
     * @since 8
     */
    FOCUS_MODE_AUTO,
    /**
     * Locked mode.
     * @since 8
     */
    FOCUS_MODE_LOCKED
  }

  /**
   * Enum for focus state.
   * @since 8
   */
  enum FocusState {
    /**
     * Scan state.
     * @since 8
     */
    FOCUS_STATE_SCAN = 0,
    /**
     * Focused state.
     * @since 8
     */
    FOCUS_STATE_FOCUSED,
    /**
     * Unfocused state.
     * @since 8
     */
    FOCUS_STATE_UNFOCUSED
  }

  /**
   * Enum for exposure state.
   * @since 8
   */
  enum ExposureState {
    /**
     * Scan state.
     * @since 8
     */
    EXPOSURE_STATE_SCAN = 0,
    /**
     * Converged state.
     * @since 8
     */
    EXPOSURE_STATE_CONVERGED
  }

  /**
   * Gets a CaptureSession instance.
   * @param context Current application context.
   * @return CaptureSession instance.
   * @since 8
   */
  function createCaptureSession(context: Context, callback: AsyncCallback<CaptureSession>): void;
  function createCaptureSession(context: Context): Promise<CaptureSession>;

  /**
   * Capture session object.
   * @since 8
   */
  interface CaptureSession {
    /**
     * Begin capture session config.
     * @since 8
     */
    beginConfig(callback: AsyncCallback<void>): void;
    beginConfig(): Promise<void>;

    /**
     * Commit capture session config.
     * @since 8
     */
    commitConfig(callback: AsyncCallback<void>): void;
    commitConfig(): Promise<void>;

    /**
     * Adds a camera input.
     * @param cameraInput Target camera input to add.
     * @since 8
     */
    addInput(cameraInput: CameraInput, callback: AsyncCallback<void>): void;
    addInput(cameraInput: CameraInput): Promise<void>;

    /**
     * Adds a camera preview output.
     * @param previewOutput Target camera preview output to add.
     * @since 8
     */
    addOutput(previewOutput: PreviewOutput, callback: AsyncCallback<void>): void;
    addOutput(previewOutput: PreviewOutput): Promise<void>;

    /**
     * Adds a camera photo output.
     * @param photoOutput Target camera photo output to add.
     * @since 8
     */
    addOutput(photoOutput: PhotoOutput, callback: AsyncCallback<void>): void;
    addOutput(photoOutput: PhotoOutput): Promise<void>;

    /**
     * Adds a camera video output.
     * @param videoOutput Target camera video output to add.
     * @since 8
     */
    addOutput(videoOutput: VideoOutput, callback: AsyncCallback<void>): void;
    addOutput(videoOutput: VideoOutput): Promise<void>;

    /**
     * Removes a camera input.
     * @param cameraInput Target camera input to remove.
     * @since 8
     */
    removeInput(cameraInput: CameraInput, callback: AsyncCallback<void>): void;
    removeInput(cameraInput: CameraInput): Promise<void>;

    /**
     * Removes a camera preview output.
     * @param previewOutput Target camera preview output to remove.
     * @since 8
     */
    removeOutput(previewOutput: PreviewOutput, callback: AsyncCallback<void>): void;
    removeOutput(previewOutput: PreviewOutput): Promise<void>;

    /**
     * Removes a camera photo output.
     * @param photoOutput Target camera photo output to remove.
     * @since 8
     */
    removeOutput(photoOutput: PhotoOutput, callback: AsyncCallback<void>): void;
    removeOutput(photoOutput: PhotoOutput): Promise<void>;

    /**
     * Removes a camera video output.
     * @param videoOutput Target camera video output to remove.
     * @since 8
     */
    removeOutput(videoOutput: VideoOutput, callback: AsyncCallback<void>): void;
    removeOutput(videoOutput: VideoOutput): Promise<void>;

    /**
     * Starts capture session.
     * @since 8
     */
    start(callback: AsyncCallback<void>): void;
    start(): Promise<void>;

    /**
     * Stops capture session.
     * @since 8
     */
    stop(callback: AsyncCallback<void>): void;
    stop(): Promise<void>;

    /**
     * Release capture session instance.
     * @since 8
     */
    release(callback: AsyncCallback<void>): void;
    release(): Promise<void>;

    /**
     * Subscribes error event callback.
     * @param type Event type.
     * @return Error event callback.
     * @since 8
     */
    on(type: "error", callback: ErrorCallback<CaptureSessionError>): void;
  }

  enum CaptureSessionErrorCode {
    ERROR_UNKNOWN = -1
  }

  interface CaptureSessionError extends Error {
    code: CaptureSessionErrorCode;
  }

  /**
   * Creates a PreviewOutput instance.
   * @param surfaceId Surface object id used in camera preview output.
   * @return PreviewOutput instance.
   * @since 8
   */
  function createPreviewOutput(surfaceId: string, callback: AsyncCallback<PreviewOutput>): void;
  function createPreviewOutput(surfaceId: string): Promise<PreviewOutput>;

  /**
   * Preview output object.
   * @since 8
   */
  interface PreviewOutput {
    /**
     * Release output instance.
     * @since 8
     */
    release(callback: AsyncCallback<void>): void;
    release(): Promise<void>;

    /**
     * Subscribes frame start event callback.
     * @param type Event type.
     * @return Frame start event callback.
     * @since 8
     */
    on(type: "frameStart", callback: AsyncCallback<void>): void;

    /**
     * Subscribes frame end event callback.
     * @param type Event type.
     * @return Frame end event callback.
     * @since 8
     */
    on(type: "frameEnd", callback: AsyncCallback<void>): void;

    /**
     * Subscribes error event callback.
     * @param type Event type.
     * @return Error event callback.
     * @since 8
     */
    on(type: "error", callback: ErrorCallback<PreviewOutputError>): void;
  }

  enum PreviewOutputErrorCode {
    ERROR_UNKNOWN = -1
  }

  interface PreviewOutputError extends Error {
    code: PreviewOutputErrorCode;
  }

  /**
   * Creates a PhotoOutput instance.
   * @param surfaceId Surface object id used in camera photo output.
   * @return PhotoOutput instance.
   * @since 8
   */
  function createPhotoOutput(surfaceId: string, callback: AsyncCallback<PhotoOutput>): void;
  function createPhotoOutput(surfaceId: string): Promise<PhotoOutput>;

  enum ImageRotation {
    ROTATION_0 = 0,
    ROTATION_90 = 90,
    ROTATION_180 = 180,
    ROTATION_270 = 270
  }

  interface Location {
    /**
     * Latitude.
     * @since 8
     */
    latitude: number;

    /**
     * Longitude.
     * @since 8
     */
    longitude: number;
  }

  /**
   * Enum for photo quality.
   * @since 8
   */
  enum QualityLevel {
    QUALITY_LEVEL_HIGH = 0,
    QUALITY_LEVEL_MEDIUM,
    QUALITY_LEVEL_LOW
  }

  /**
   * Photo capture options to set.
   * @since 8
   */
  interface PhotoCaptureSetting {
    /**
     * Photo image quality.
     * @since 8
     */
    quality?: QualityLevel;
    /**
     * Photo rotation.
     * @since 8
     */
    rotation?: ImageRotation;
    /**
     * Photo location.
     * @since 8
     */
    location?: Location;
    /**
     * Mirror mode.
     * @since 8
     */
    mirror?: boolean;
  }

  /**
   * Photo output object.
   * @since 8
   */
  interface PhotoOutput {
    /**
     * Start capture output.
     * @since 8
     */
    capture(callback: AsyncCallback<void>): void;
    capture(setting: PhotoCaptureSetting, callback: AsyncCallback<void>): void;
    capture(setting?: PhotoCaptureSetting): Promise<void>;

    /**
     * Release output instance.
     * @since 8
     */
    release(callback: AsyncCallback<void>): void;
    release(): Promise<void>;

    /**
     * Check if mirror mode supported.
     * @since 8
     */
    isMirrorSupported(callback: AsyncCallback<boolean>): void;
    isMirrorSupported(): Promise<boolean>;

    /**
     * Subscribes capture start event callback.
     * @param type Event type.
     * @return Capture start event callback.
     * @since 8
     */
    on(type: "captureStart", callback: AsyncCallback<number>): void;

    /**
     * Subscribes frame shutter event callback.
     * @param type Event type.
     * @return Frame shutter event callback.
     * @since 8
     */
    on(type: "frameShutter", callback: AsyncCallback<FrameShutterInfo>): void;

    /**
     * Subscribes capture end event callback.
     * @param type Event type.
     * @return Capture end event callback.
     * @since 8
     */
    on(type: "captureEnd", callback: AsyncCallback<CaptureEndInfo>): void;

    /**
     * Subscribes error event callback.
     * @param type Event type.
     * @return Error event callback.
     * @since 8
     */
    on(type: "error", callback: ErrorCallback<PhotoOutputError>): void;
  }

  /**
   * Frame shutter callback info.
   * @since 8
   */
  interface FrameShutterInfo {
    /**
     * Capture id.
     * @since 8
     */
    captureId: number;
    /**
     * Timestamp for frame.
     * @since 8
     */
    timestamp: number;
  }

  /**
   * Capture end info.
   * @since 8
   */
  interface CaptureEndInfo {
    /**
     * Capture id.
     * @since 8
     */
    captureId: number;
    /**
     * Frame count.
     * @since 8
     */
    frameCount: number;
  }

  enum PhotoOutputErrorCode {
    ERROR_UNKNOWN = -1
  }

  interface PhotoOutputError extends Error {
    code: PhotoOutputErrorCode;
  }

  /**
   * Creates a VideoOutput instance.
   * @param surfaceId Surface object id used in camera video output.
   * @return VideoOutput instance.
   * @since 8
   */
  function createVideoOutput(surfaceId: string, callback: AsyncCallback<VideoOutput>): void;
  function createVideoOutput(surfaceId: string): Promise<VideoOutput>;

  /**
   * Video output object.
   * @since 8
   */
  interface VideoOutput {
    /**
     * Start video output.
     * @since 8
     */
    start(callback: AsyncCallback<void>): void;
    start(): Promise<void>;

    /**
     * Stop video output.
     * @since 8
     */
    stop(callback: AsyncCallback<void>): void;
    stop(): Promise<void>;

    /**
     * Release output instance.
     * @since 8
     */
    release(callback: AsyncCallback<void>): void;
    release(): Promise<void>;

    /**
     * Subscribes frame start event callback.
     * @param type Event type.
     * @return Frame start event callback.
     * @since 8
     */
    on(type: "frameStart", callback: AsyncCallback<void>): void;

    /**
     * Subscribes frame end event callback.
     * @param type Event type.
     * @return Frame end event callback.
     * @since 8
     */
    on(type: "frameEnd", callback: AsyncCallback<void>): void;

    /**
     * Subscribes error event callback.
     * @param type Event type.
     * @return Error event callback.
     * @since 8
     */
    on(type: "error", callback: ErrorCallback<VideoOutputError>): void;
  }

  enum VideoOutputErrorCode {
    ERROR_UNKNOWN = -1
  }

  interface VideoOutputError extends Error {
    code: VideoOutputErrorCode;
  }
}

export default camera;