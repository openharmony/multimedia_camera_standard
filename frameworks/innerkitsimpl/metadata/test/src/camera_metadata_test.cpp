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

#include "camera_metadata_operator.h"
#include "camera_metadata_test.h"

using namespace testing::ext;

void CameraMetadataTest::SetUpTestCase(void) {}
void CameraMetadataTest::TearDownTestCase(void) {}

void CameraMetadataTest::SetUp() {}
void CameraMetadataTest::TearDown() {}

/*
 * Feature: Metadata
 * Function: allocate_camera_metadata_buffer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test allocation of camera metadata
 */
HWTEST_F(CameraMetadataTest, media_camera_metadata_test_001, TestSize.Level1)
{
    uint32_t item_capacity = 1;
    uint32_t data_capacity = 1;
    common_metadata_header_t *metadata = allocate_camera_metadata_buffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_EQ(metadata->item_count, 0U);
    EXPECT_EQ(metadata->item_capacity, 1U);
    EXPECT_EQ(metadata->data_count, 0U);
    EXPECT_EQ(metadata->data_capacity, 1U);
    free_camera_metadata_buffer(metadata);
}

/*
 * Feature: Metadata
 * Function: add_camera_metadata_item
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type
 */
HWTEST_F(CameraMetadataTest, media_camera_metadata_test_002, TestSize.Level1)
{
    uint32_t item_capacity = 1;
    uint32_t data_capacity = 0;
    common_metadata_header_t *metadata = allocate_camera_metadata_buffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 1);
    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = add_camera_metadata_item(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 1);
    camera_metadata_item_entry_t *metadata_item = get_metadata_items(metadata);
    EXPECT_TRUE(metadata_item->item == OHOS_ABILITY_CAMERA_TYPE);
    EXPECT_TRUE(metadata_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(metadata_item->count == 1);
    EXPECT_TRUE(metadata_item->data.value[0] == OHOS_CAMERA_TYPE_ULTRA_WIDE);
    free_camera_metadata_buffer(metadata);
}

/*
 * Feature: Metadata
 * Function: add_camera_metadata_item
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type and camera position
 */
HWTEST_F(CameraMetadataTest, media_camera_metadata_test_003, TestSize.Level1)
{
    uint32_t item_capacity = 2;
    uint32_t data_capacity = 0;
    common_metadata_header_t *metadata = allocate_camera_metadata_buffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 2);
    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = add_camera_metadata_item(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);
    uint8_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    result = add_camera_metadata_item(metadata, OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 2);
    camera_metadata_item_entry_t *position_item = get_metadata_items(metadata) 
        + metadata->item_count - 1;
    EXPECT_TRUE(position_item->item == OHOS_ABILITY_CAMERA_POSITION);
    EXPECT_TRUE(position_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(position_item->count == 1);
    EXPECT_TRUE(position_item->data.value[0] == OHOS_CAMERA_POSITION_BACK);
    free_camera_metadata_buffer(metadata);
}

/*
 * Feature: Metadata
 * Function: add_camera_metadata_item
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type, camera position and available 
 * focus modes
 */
HWTEST_F(CameraMetadataTest, media_camera_metadata_test_004, TestSize.Level1)
{
    uint32_t item_capacity = 3;
    uint32_t data_capacity = 0;
    common_metadata_header_t *metadata = allocate_camera_metadata_buffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 3);
    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = add_camera_metadata_item(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    result = add_camera_metadata_item(metadata, OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);

    uint8_t focusModes[4] = { OHOS_CAMERA_FOCUS_MODE_MANUAL, 
        OHOS_CAMERA_FOCUS_MODE_CONTINUOUS_AUTO, OHOS_CAMERA_FOCUS_MODE_AUTO, 
        OHOS_CAMERA_FOCUS_MODE_LOCKED };
    result = add_camera_metadata_item(metadata, OHOS_ABILITY_DEVICE_AVAILABLE_FOCUSMODES, focusModes, 4);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 3);
    camera_metadata_item_entry_t *avail_focusmodes_item = get_metadata_items(metadata) 
        + metadata->item_count - 1;
    EXPECT_TRUE(OHOS_ABILITY_DEVICE_AVAILABLE_FOCUSMODES == avail_focusmodes_item->item);
    EXPECT_TRUE(META_TYPE_BYTE == avail_focusmodes_item->data_type);
    EXPECT_TRUE(4 == avail_focusmodes_item->count);
    EXPECT_TRUE(avail_focusmodes_item->data.value[0] == OHOS_CAMERA_FOCUS_MODE_MANUAL);
    EXPECT_TRUE(avail_focusmodes_item->data.value[1] == OHOS_CAMERA_FOCUS_MODE_CONTINUOUS_AUTO);
    EXPECT_TRUE(avail_focusmodes_item->data.value[2] == OHOS_CAMERA_FOCUS_MODE_AUTO);
    EXPECT_TRUE(avail_focusmodes_item->data.value[3] == OHOS_CAMERA_FOCUS_MODE_LOCKED);
    free_camera_metadata_buffer(metadata);
}

/*
 * Feature: Metadata
 * Function: add_camera_metadata_item
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type, camera position, 
 *                  focus mode and zoom ratio range
 */
HWTEST_F(CameraMetadataTest, media_camera_metadata_test_005, TestSize.Level1)
{
    uint32_t item_capacity = 4;
    uint32_t data_capacity = ALIGN_TO(5 * sizeof(float), DATA_ALIGNMENT);
    common_metadata_header_t *metadata = allocate_camera_metadata_buffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 4);
    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = add_camera_metadata_item(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    result = add_camera_metadata_item(metadata, OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);

    uint8_t focusMode = OHOS_CAMERA_FOCUS_MODE_MANUAL;
    result = add_camera_metadata_item(metadata, OHOS_CONTROL_FOCUSMODE, &focusMode, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 3);

    float zoomRatioRange[5] = {1.0, 2.0, 4.0, 8.0, 16.1};
    result = add_camera_metadata_item(metadata, OHOS_ABILITY_ZOOM_RATIO_RANGE, &zoomRatioRange, 5);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 4);

    camera_metadata_item_entry_t *zoom_ratio_range_item = get_metadata_items(metadata) 
        + metadata->item_count - 1;
    EXPECT_TRUE(zoom_ratio_range_item->item == OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(zoom_ratio_range_item->data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(zoom_ratio_range_item->count == 5);
    uint8_t *zoom_ratios = get_metadata_data(metadata) + zoom_ratio_range_item->data.offset;
    EXPECT_TRUE(memcmp(zoomRatioRange, zoom_ratios, 5 * sizeof(float)) == 0);
    free_camera_metadata_buffer(metadata);
}
