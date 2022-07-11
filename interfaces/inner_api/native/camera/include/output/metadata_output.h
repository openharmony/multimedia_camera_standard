/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_METADATA_OUTPUT_H
#define OHOS_CAMERA_METADATA_OUTPUT_H

#include <iostream>
#include "capture_output.h"
#include "istream_metadata.h"

#include "camera_metadata_operator.h"
#include "surface.h"

namespace OHOS {
namespace CameraStandard {
enum class MetadataObjectType : int32_t {
    FACE = 0,
};

enum MetadataOutputErrorCode : int32_t {
    ERROR_UNKNOWN = 1,
    ERROR_INSUFFICIENT_RESOURCES,
};

typedef struct {
    double topLeftX;
    double topLeftY;
    double width;
    double height;
} Rect;

class MetadataObject : public RefBase {
public:
    MetadataObject(MetadataObjectType type, double timestamp, Rect rect);
    virtual ~MetadataObject() = default;
    MetadataObjectType GetType();
    double GetTimestamp();
    Rect GetBoundingBox();

private:
    MetadataObjectType type_;
    double timestamp_;
    Rect box_;
};

class MetadataFaceObject : public MetadataObject {
public:
    MetadataFaceObject(double timestamp, Rect rect);
    ~MetadataFaceObject() = default;
};

class MetadataObjectCallback {
public:
    MetadataObjectCallback() = default;
    virtual ~MetadataObjectCallback() = default;
    virtual void OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const = 0;
};

class MetadataStateCallback {
public:
    MetadataStateCallback() = default;
    virtual ~MetadataStateCallback() = default;
    virtual void OnError(int32_t errorCode) const = 0;
};

class MetadataOutput : public CaptureOutput {
public:
    MetadataOutput(sptr<Surface> surface, sptr<IStreamMetadata> &streamMetadata);

    /**
     * @brief Get the supported metadata object types.
     *
     * @return Returns vector of MetadataObjectType.
     */
    std::vector<MetadataObjectType> GetSupportedMetadataObjectTypes();

    /**
     * @brief Set the metadata object types
     *
     * @param Vector of MetadataObjectType
     */
    void SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> objectTypes);

    /**
     * @brief Set the metadata object callback for the metadata output.
     *
     * @param MetadataObjectCallback pointer to be triggered.
     */
    void SetCallback(std::shared_ptr<MetadataObjectCallback> metadataObjectCallback);

    /**
     * @brief Set the metadata state callback for the metadata output.
     *
     * @param MetadataStateCallback pointer to be triggered.
     */
    void SetCallback(std::shared_ptr<MetadataStateCallback> metadataStateCallback);

    /**
     * @brief Start the metadata capture.
     */
    int32_t Start();

    /**
     * @brief Stop the metadata capture.
     */
    int32_t Stop();

    /**
     * @brief Releases a instance of the MetadataOutput.
     */
    void Release() override;

    friend class MetadataObjectListener;
private:
    sptr<Surface> surface_;
    std::shared_ptr<MetadataObjectCallback> appObjectCallback_;
    std::shared_ptr<MetadataStateCallback> appStateCallback_;
};

class MetadataObjectListener : public IBufferConsumerListener {
public:
    MetadataObjectListener(sptr<MetadataOutput> metadata);
    virtual ~MetadataObjectListener() = default;
    void OnBufferAvailable() override;

private:
    int32_t ProcessMetadataBuffer(void *buffer, int64_t timestamp);
    int32_t ProcessFaceRectangles(int64_t timestamp, const camera_metadata_item_t &metadataItem,
                                  std::vector<sptr<MetadataObject>> &metaObjects);
    sptr<MetadataOutput> metadata_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_METADATA_OUTPUT_H
