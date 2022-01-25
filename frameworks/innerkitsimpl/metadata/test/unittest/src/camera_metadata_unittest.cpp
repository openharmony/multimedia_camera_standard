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

#include "camera_metadata_unittest.h"
#include "metadata_utils.h"

using namespace testing::ext;

namespace OHOS {
namespace CameraStandard {
void CameraMetadataUnitTest::SetUpTestCase(void) {}
void CameraMetadataUnitTest::TearDownTestCase(void) {}

void CameraMetadataUnitTest::SetUp() {}
void CameraMetadataUnitTest::TearDown() {}

/*
 * Feature: Metadata
 * Function: AllocateCameraMetadataBuffer
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test allocation of camera metadata
 */
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_001, TestSize.Level0)
{
    uint32_t item_capacity = 1;
    uint32_t data_capacity = 1;
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_EQ(metadata->item_count, 0U);
    EXPECT_EQ(metadata->item_capacity, 1U);
    EXPECT_EQ(metadata->data_count, 0U);
    EXPECT_EQ(metadata->data_capacity, 1U);

    FreeCameraMetadataBuffer(metadata);
}

/*
 * Feature: Metadata
 * Function: AddCameraMetadataItem
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type
 */
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_002, TestSize.Level0)
{
    uint32_t item_capacity = 1;
    uint32_t data_capacity = 0;
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 1);

    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 1);

    camera_metadata_item_entry_t *metadata_item = GetMetadataItems(metadata);
    EXPECT_TRUE(metadata_item->item == OHOS_ABILITY_CAMERA_TYPE);
    EXPECT_TRUE(metadata_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(metadata_item->count == 1);
    EXPECT_TRUE(metadata_item->data.value[0] == OHOS_CAMERA_TYPE_ULTRA_WIDE);

    FreeCameraMetadataBuffer(metadata);
}

/*
 * Feature: Metadata
 * Function: AddCameraMetadataItem
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type and camera position
 */
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_003, TestSize.Level0)
{
    uint32_t item_capacity = 2;
    uint32_t data_capacity = 0;
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 2);

    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 2);

    camera_metadata_item_entry_t *position_item = GetMetadataItems(metadata)
        + metadata->item_count - 1;
    EXPECT_TRUE(position_item->item == OHOS_ABILITY_CAMERA_POSITION);
    EXPECT_TRUE(position_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(position_item->count == 1);
    EXPECT_TRUE(position_item->data.value[0] == OHOS_CAMERA_POSITION_BACK);

    FreeCameraMetadataBuffer(metadata);
}

/*
 * Feature: Metadata
 * Function: AddCameraMetadataItem
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type, camera position and available
 * focus modes
 */
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_004, TestSize.Level0)
{
    uint32_t item_capacity = 3;
    uint32_t data_capacity = 0;
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 3);

    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);

    uint8_t focusModes[4] = { OHOS_CAMERA_AF_MODE_AUTO,
        OHOS_CAMERA_AF_MODE_MACRO,
        OHOS_CAMERA_AF_MODE_CONTINUOUS_VIDEO,
        OHOS_CAMERA_AF_MODE_CONTINUOUS_PICTURE};
    result = AddCameraMetadataItem(metadata, OHOS_CONTROL_AF_AVAILABLE_MODES, focusModes, 4);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 3);

    camera_metadata_item_entry_t *avail_focusmodes_item = GetMetadataItems(metadata)
        + metadata->item_count - 1;
    EXPECT_TRUE(OHOS_CONTROL_AF_AVAILABLE_MODES == avail_focusmodes_item->item);
    EXPECT_TRUE(META_TYPE_BYTE == avail_focusmodes_item->data_type);
    EXPECT_TRUE(4 == avail_focusmodes_item->count);
    EXPECT_TRUE(avail_focusmodes_item->data.value[0] == OHOS_CAMERA_AF_MODE_AUTO);
    EXPECT_TRUE(avail_focusmodes_item->data.value[1] == OHOS_CAMERA_AF_MODE_MACRO);
    EXPECT_TRUE(avail_focusmodes_item->data.value[2] == OHOS_CAMERA_AF_MODE_CONTINUOUS_VIDEO);
    EXPECT_TRUE(avail_focusmodes_item->data.value[3] == OHOS_CAMERA_AF_MODE_CONTINUOUS_PICTURE);

    FreeCameraMetadataBuffer(metadata);
}

/*
 * Feature: Metadata
 * Function: AddCameraMetadataItem
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add camera metadata item with camera type, camera position,
 *                  focus mode and zoom ratio range
 */
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_005, TestSize.Level0)
{
    uint32_t item_capacity = 4;
    uint32_t data_capacity = AlignTo(5 * sizeof(float), DATA_ALIGNMENT);
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);
    EXPECT_TRUE(metadata->item_count == 0);
    EXPECT_TRUE(metadata->item_capacity == 4);

    uint8_t cameraType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
    int result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);

    uint8_t focusMode = OHOS_CAMERA_AF_MODE_AUTO;
    result = AddCameraMetadataItem(metadata, OHOS_CONTROL_AF_MODE, &focusMode, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 3);

    float zoomRatioRange[5] = {1.0, 2.0, 4.0, 8.0, 16.1};
    result = AddCameraMetadataItem(metadata, OHOS_ABILITY_ZOOM_RATIO_RANGE, zoomRatioRange,
                                    sizeof(zoomRatioRange)/sizeof(zoomRatioRange[0]));
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 4);

    camera_metadata_item_entry_t *zoom_ratio_range_item = GetMetadataItems(metadata)
        + metadata->item_count - 1;
    EXPECT_TRUE(zoom_ratio_range_item->item == OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(zoom_ratio_range_item->data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(zoom_ratio_range_item->count == sizeof(zoomRatioRange)/sizeof(zoomRatioRange[0]));
    uint8_t *zoom_ratios = GetMetadataData(metadata) + zoom_ratio_range_item->data.offset;
    EXPECT_TRUE(memcmp(zoomRatioRange, zoom_ratios, sizeof(zoomRatioRange)) == 0);

    FreeCameraMetadataBuffer(metadata);
}

/*
* Feature: Metadata
* Function: FindCameraMetadataItem and UpdateCameraMetadataItem
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test find metadata and update metadata items with camera type, camera position,
*                  flash mode, exposure mode and zoom ratio range.
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_006, TestSize.Level0)
{
    uint32_t item_capacity = 5;
    uint32_t data_capacity = AlignTo(6 * sizeof(float), DATA_ALIGNMENT);
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);

    uint8_t cameraType = OHOS_CAMERA_TYPE_WIDE_ANGLE;
    int result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    float zoomRatioRange[4] = {1.0, 2.0, 4.0, 8.0};
    result = AddCameraMetadataItem(metadata, OHOS_ABILITY_ZOOM_RATIO_RANGE, zoomRatioRange,
                                    sizeof(zoomRatioRange)/sizeof(zoomRatioRange[0]));
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    uint8_t flashMode = OHOS_CAMERA_FLASH_MODE_AUTO;
    result = AddCameraMetadataItem(metadata, OHOS_CONTROL_FLASHMODE, &flashMode, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    uint8_t exposureMode = OHOS_CAMERA_AE_MODE_ON_AUTO_FLASH;
    result = AddCameraMetadataItem(metadata, OHOS_CONTROL_AE_MODE, &exposureMode, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata->item_count == 5);

    camera_metadata_item_t metadata_item;

    // Find the focus mode. It should return item not found error
    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_AF_MODE, &metadata_item);
    EXPECT_TRUE(result == CAM_META_ITEM_NOT_FOUND);

    // Find the flash mode and verify the values returned
    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_FLASHMODE, &metadata_item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata_item.item == OHOS_CONTROL_FLASHMODE);
    EXPECT_TRUE(metadata_item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(metadata_item.index == 3);
    EXPECT_TRUE(metadata_item.count == 1);
    EXPECT_TRUE(metadata_item.data.u8[0] == OHOS_CAMERA_FLASH_MODE_AUTO);

    // Update focus mode should fail as it is not present and return item not found error
    uint8_t focusMode = OHOS_CAMERA_AF_MODE_AUTO;
    result = UpdateCameraMetadataItem(metadata, OHOS_CONTROL_AF_MODE, &focusMode, 1, &metadata_item);
    EXPECT_TRUE(result == CAM_META_ITEM_NOT_FOUND);

    // Find the current exposure mode
    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_AE_MODE, &metadata_item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata_item.data.u8[0] == OHOS_CAMERA_AE_MODE_ON_AUTO_FLASH);

    // Update exposure mode
    exposureMode = OHOS_CAMERA_AE_MODE_ON;
    result = UpdateCameraMetadataItem(metadata, OHOS_CONTROL_AE_MODE, &exposureMode, 1, &metadata_item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata_item.item == OHOS_CONTROL_AE_MODE);
    EXPECT_TRUE(metadata_item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(metadata_item.index == 4);
    EXPECT_TRUE(metadata_item.count == 1);
    EXPECT_TRUE(metadata_item.data.u8[0] == OHOS_CAMERA_AE_MODE_ON);

    // Update zoom ratio range
    float updatedZoomRatioRange[6] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0};
    result = UpdateCameraMetadataItem(metadata, OHOS_ABILITY_ZOOM_RATIO_RANGE, updatedZoomRatioRange,
                sizeof(updatedZoomRatioRange)/sizeof(updatedZoomRatioRange[0]), &metadata_item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata_item.item == OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(metadata_item.data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(metadata_item.index == 2);
    EXPECT_TRUE(metadata_item.count == sizeof(updatedZoomRatioRange)/sizeof(updatedZoomRatioRange[0]));
    EXPECT_TRUE(memcmp(updatedZoomRatioRange, metadata_item.data.f, sizeof(updatedZoomRatioRange)) == 0);

    // Find to check if updated zoom ratio range is returned
    camera_metadata_item_t updated_item;
    result = FindCameraMetadataItem(metadata, OHOS_ABILITY_ZOOM_RATIO_RANGE, &updated_item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(memcmp(&updated_item, &metadata_item, sizeof(updated_item)) == 0);
    EXPECT_TRUE(GetCameraMetadataItemName(OHOS_ABILITY_ZOOM_RATIO_RANGE) != nullptr);

    // Free metadata
    FreeCameraMetadataBuffer(metadata);
}

/*
* Feature: Metadata
* Function: DeleteCameraMetadataItem
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test delete metadata item
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_007, TestSize.Level0)
{
    uint32_t item_capacity = 3;
    uint32_t data_capacity = 0;
    common_metadata_header_t *metadata = AllocateCameraMetadataBuffer(item_capacity, data_capacity);
    ASSERT_NE(metadata, nullptr);

    uint8_t cameraType = OHOS_CAMERA_TYPE_TELTPHOTO;
    int result = AddCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    uint8_t flashMode = OHOS_CAMERA_FLASH_MODE_OPEN;
    result = AddCameraMetadataItem(metadata, OHOS_CONTROL_FLASHMODE, &flashMode, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    uint8_t focusMode = OHOS_CAMERA_AF_MODE_OFF;
    result = AddCameraMetadataItem(metadata, OHOS_CONTROL_AF_MODE, &focusMode, 1);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    camera_metadata_item_t metadata_item;
    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_FLASHMODE, &metadata_item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(metadata_item.item == OHOS_CONTROL_FLASHMODE);
    EXPECT_TRUE(metadata_item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(metadata_item.index == 1);
    EXPECT_TRUE(metadata_item.count == 1);
    EXPECT_TRUE(metadata_item.data.u8[0] == OHOS_CAMERA_FLASH_MODE_OPEN);

    // Delete exposure mode should return item not found error
    result = DeleteCameraMetadataItem(metadata, OHOS_CONTROL_AE_MODE);
    EXPECT_TRUE(result == CAM_META_ITEM_NOT_FOUND);

    // delete flash mode
    result = DeleteCameraMetadataItem(metadata, OHOS_CONTROL_FLASHMODE);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    // Verify if flash mode is deleted from metadata
    camera_metadata_item_entry_t *base_item = GetMetadataItems(metadata);
    uint32_t items[2] = {OHOS_ABILITY_CAMERA_TYPE, OHOS_CONTROL_AF_MODE};
    uint8_t values[2] = {OHOS_CAMERA_TYPE_TELTPHOTO, OHOS_CAMERA_AF_MODE_OFF};
    for (int i = 0; i < 2; i++, base_item++) {
        EXPECT_TRUE(base_item->item == items[i]);
        EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
        EXPECT_TRUE(base_item->count == 1);
        EXPECT_TRUE(base_item->data.value[0] == values[i]);
    }

    // Free metadata
    FreeCameraMetadataBuffer(metadata);
}

/*
* Feature: Metadata
* Function: addEntry
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test addEntry
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_008, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> meta = std::make_shared<CameraMetadata>(2, 0);
    uint8_t cameraType = OHOS_CAMERA_TYPE_TRUE_DEAPTH;
    bool ret = meta->addEntry(OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);
    EXPECT_TRUE(ret == true);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    ret = meta->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    EXPECT_TRUE(ret == true);

    // Verify if both the added metadata items are present in buffer
    camera_metadata_item_entry_t *base_item = GetMetadataItems(meta->get());
    uint32_t items[2] = {OHOS_ABILITY_CAMERA_TYPE, OHOS_ABILITY_CAMERA_POSITION};
    uint8_t values[2] = {OHOS_CAMERA_TYPE_TRUE_DEAPTH, OHOS_CAMERA_POSITION_FRONT};
    for (int i = 0; i < 2; i++, base_item++) {
        EXPECT_TRUE(base_item->item == items[i]);
        EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
        EXPECT_TRUE(base_item->count == 1);
        EXPECT_TRUE(base_item->data.value[0] == values[i]);
    }
}

/*
* Feature: Metadata
* Function: updateEntry and DeleteCameraMetadataItem
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test updateEntry with metadata item data size more than 4 bytes and then delete item
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_009, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> meta = std::make_shared<CameraMetadata>(3, AlignTo(7 * sizeof(float),
                                                                            DATA_ALIGNMENT));
    uint8_t cameraType = OHOS_CAMERA_TYPE_TRUE_DEAPTH;
    bool ret = meta->addEntry(OHOS_ABILITY_CAMERA_TYPE, &cameraType, 1);
    EXPECT_TRUE(ret == true);

    float zoomRatioRange[5] = {1.0, 2.0, 4.0, 8.0, 16.0};
    ret = meta->addEntry(OHOS_ABILITY_ZOOM_RATIO_RANGE, zoomRatioRange,
                         sizeof(zoomRatioRange)/sizeof(zoomRatioRange[0]));
    EXPECT_TRUE(ret == true);

    uint8_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    ret = meta->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    EXPECT_TRUE(ret == true);

    // Verify all 3 added metadata items are present in buffer
    camera_metadata_item_entry_t *base_item = GetMetadataItems(meta->get());
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_CAMERA_TYPE);
    EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(base_item->count == 1);
    EXPECT_TRUE(base_item->data.value[0] == OHOS_CAMERA_TYPE_TRUE_DEAPTH);

    base_item++;
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(base_item->data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(base_item->count == sizeof(zoomRatioRange)/sizeof(zoomRatioRange[0]));
    EXPECT_TRUE(memcmp(zoomRatioRange, GetMetadataData(meta->get()) + base_item->data.offset,
        sizeof(zoomRatioRange)) == 0);

    base_item++;
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_CAMERA_POSITION);
    EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(base_item->count == 1);
    EXPECT_TRUE(base_item->data.value[0] == OHOS_CAMERA_POSITION_FRONT);

    // update the zoom ration range
    float newZoomRatioRange[7] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0};
    ret = meta->updateEntry(OHOS_ABILITY_ZOOM_RATIO_RANGE, newZoomRatioRange,
    		                sizeof(newZoomRatioRange)/sizeof(newZoomRatioRange[0]));
    EXPECT_TRUE(ret == true);

    // Verify metadata items in buffer
    base_item = GetMetadataItems(meta->get());
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_CAMERA_TYPE);
    EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(base_item->count == 1);
    EXPECT_TRUE(base_item->data.value[0] == OHOS_CAMERA_TYPE_TRUE_DEAPTH);

    base_item++;
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(base_item->data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(base_item->count == sizeof(newZoomRatioRange)/sizeof(newZoomRatioRange[0]));
    EXPECT_TRUE(memcmp(newZoomRatioRange, GetMetadataData(meta->get()) + base_item->data.offset,
                       sizeof(newZoomRatioRange)) == 0);

    base_item++;
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_CAMERA_POSITION);
    EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(base_item->count == 1);
    EXPECT_TRUE(base_item->data.value[0] == OHOS_CAMERA_POSITION_FRONT);

    // delete the zoom ratio range
    int result = DeleteCameraMetadataItem(meta->get(), OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    // Verify metadata items in buffer
    base_item = GetMetadataItems(meta->get());
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_CAMERA_TYPE);
    EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(base_item->count == 1);
    EXPECT_TRUE(base_item->data.value[0] == OHOS_CAMERA_TYPE_TRUE_DEAPTH);

    base_item++;
    EXPECT_TRUE(base_item->item == OHOS_ABILITY_CAMERA_POSITION);
    EXPECT_TRUE(base_item->data_type == META_TYPE_BYTE);
    EXPECT_TRUE(base_item->count == 1);
    EXPECT_TRUE(base_item->data.value[0] == OHOS_CAMERA_POSITION_FRONT);
}

/*
* Feature: Metadata
* Function: addEntry, EncodeCameraMetadata, DecodeCameraMetadata
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test operations(add/find/delete) on metadata items with all data types
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_010, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> cameraMetadata = std::make_shared<CameraMetadata>(9, 80);

    common_metadata_header_t *metadata = cameraMetadata->get();
    ASSERT_NE(metadata, nullptr);

    camera_metadata_item_t item;

    // byte
    uint8_t connectionType = OHOS_CAMERA_CONNECTION_TYPE_REMOTE;
    bool ret = cameraMetadata->addEntry(OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &connectionType, 1);
    EXPECT_TRUE(ret == true);

    int result = FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 0);
    EXPECT_TRUE(item.item == OHOS_ABILITY_CAMERA_CONNECTION_TYPE);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.u8[0] == OHOS_CAMERA_CONNECTION_TYPE_REMOTE);

    // byte array
    uint8_t scores[4] = {1, 2, 3, 0xFF};
    ret = cameraMetadata->addEntry(OHOS_STATISTICS_FACE_SCORES, scores, sizeof(scores)/sizeof(scores[0]));
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_STATISTICS_FACE_SCORES, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 1);
    EXPECT_TRUE(item.item == OHOS_STATISTICS_FACE_SCORES);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == sizeof(scores)/sizeof(scores[0]));
    EXPECT_TRUE(memcmp(item.data.u8, scores, sizeof(scores)) == 0);

    // int32
    int32_t exposureCompensation = 0xFFFFFFFF;
    ret = cameraMetadata->addEntry(OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &exposureCompensation, 1);
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 2);
    EXPECT_TRUE(item.item == OHOS_CONTROL_AE_EXPOSURE_COMPENSATION);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.i32[0] == exposureCompensation);

    // int32 array
    int32_t activeArray[4] = {0, 0, 2000, 1500};
    ret = cameraMetadata->addEntry(OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeArray,
                                   sizeof(activeArray)/sizeof(activeArray[0]));
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 3);
    EXPECT_TRUE(item.item == OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == sizeof(activeArray)/sizeof(activeArray[0]));
    EXPECT_TRUE(memcmp(item.data.i32, activeArray, sizeof(activeArray)) == 0);

    // int64
    int64_t exposureTime = 0xFFFFFFFFFFFFFFFF;
    ret = cameraMetadata->addEntry(OHOS_SENSOR_EXPOSURE_TIME, &exposureTime, 1);
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_SENSOR_EXPOSURE_TIME, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 4);
    EXPECT_TRUE(item.item == OHOS_SENSOR_EXPOSURE_TIME);
    EXPECT_TRUE(item.data_type == META_TYPE_INT64);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.i64[0] == exposureTime);

    // float
    float gain = 21.3;
    ret = cameraMetadata->addEntry(OHOS_SENSOR_COLOR_CORRECTION_GAINS, &gain, 1);
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_SENSOR_COLOR_CORRECTION_GAINS, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 5);
    EXPECT_TRUE(item.item == OHOS_SENSOR_COLOR_CORRECTION_GAINS);
    EXPECT_TRUE(item.data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.f[0] == gain);

    // float array
    float zoomRatioRange[7] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0};
    ret = cameraMetadata->addEntry(OHOS_ABILITY_ZOOM_RATIO_RANGE, zoomRatioRange,
                                   sizeof(zoomRatioRange)/sizeof(zoomRatioRange[0]));
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_ABILITY_ZOOM_RATIO_RANGE, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 6);
    EXPECT_TRUE(item.item == OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(item.data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(item.count == sizeof(zoomRatioRange)/sizeof(zoomRatioRange[0]));
    EXPECT_TRUE(memcmp(item.data.f, zoomRatioRange, sizeof(zoomRatioRange)) == 0);

    // double array
    double gpsCoordinates[2] = {23.0166738, 77.7625576};
    ret = cameraMetadata->addEntry(OHOS_JPEG_GPS_COORDINATES, gpsCoordinates,
                                   sizeof(gpsCoordinates)/sizeof(gpsCoordinates[0]));
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_JPEG_GPS_COORDINATES, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 7);
    EXPECT_TRUE(item.item == OHOS_JPEG_GPS_COORDINATES);
    EXPECT_TRUE(item.data_type == META_TYPE_DOUBLE);
    EXPECT_TRUE(item.count == sizeof(gpsCoordinates)/sizeof(gpsCoordinates[0]));
    EXPECT_TRUE(memcmp(item.data.d, gpsCoordinates, sizeof(gpsCoordinates)) == 0);

    // rational
    camera_rational_t rational = {1, 2};
    ret = cameraMetadata->addEntry(OHOS_CONTROL_AE_COMPENSATION_STEP, &rational, 1);
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_AE_COMPENSATION_STEP, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 8);
    EXPECT_TRUE(item.item == OHOS_CONTROL_AE_COMPENSATION_STEP);
    EXPECT_TRUE(item.data_type == META_TYPE_RATIONAL);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(memcmp(item.data.r, &rational, sizeof(rational)) == 0);

    // Delete zoom ratio range and check if items beneath are moved up
    result = DeleteCameraMetadataItem(metadata, OHOS_ABILITY_ZOOM_RATIO_RANGE);
    EXPECT_TRUE(result == CAM_META_SUCCESS);

    result = FindCameraMetadataItem(metadata, OHOS_JPEG_GPS_COORDINATES, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 6);
    EXPECT_TRUE(item.item == OHOS_JPEG_GPS_COORDINATES);
    EXPECT_TRUE(item.data_type == META_TYPE_DOUBLE);
    EXPECT_TRUE(item.count == sizeof(gpsCoordinates)/sizeof(gpsCoordinates[0]));
    EXPECT_TRUE(memcmp(item.data.d, gpsCoordinates, sizeof(gpsCoordinates)) == 0);

    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_AE_COMPENSATION_STEP, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 7);
    EXPECT_TRUE(item.item == OHOS_CONTROL_AE_COMPENSATION_STEP);
    EXPECT_TRUE(item.data_type == META_TYPE_RATIONAL);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(memcmp(item.data.r, &rational, sizeof(rational)) == 0);

    // Encode camera metadata
    OHOS::MessageParcel data;
    ret = MetadataUtils::EncodeCameraMetadata(cameraMetadata, data);
    EXPECT_TRUE(ret == true);

    // Decode camera metadata
    std::shared_ptr<CameraMetadata> decodedCameraMetadata;
    MetadataUtils::DecodeCameraMetadata(data, decodedCameraMetadata);

    // validate metadata
    metadata = decodedCameraMetadata->get();
    result = FindCameraMetadataItem(metadata, OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 0);
    EXPECT_TRUE(item.item == OHOS_ABILITY_CAMERA_CONNECTION_TYPE);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.u8[0] == OHOS_CAMERA_CONNECTION_TYPE_REMOTE);

    result = FindCameraMetadataItem(metadata, OHOS_STATISTICS_FACE_SCORES, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 1);
    EXPECT_TRUE(item.item == OHOS_STATISTICS_FACE_SCORES);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == sizeof(scores)/sizeof(scores[0]));
    EXPECT_TRUE(memcmp(item.data.u8, scores, sizeof(scores)) == 0);

    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 2);
    EXPECT_TRUE(item.item == OHOS_CONTROL_AE_EXPOSURE_COMPENSATION);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.i32[0] == exposureCompensation);

    result = FindCameraMetadataItem(metadata, OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 3);
    EXPECT_TRUE(item.item == OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == sizeof(activeArray)/sizeof(activeArray[0]));
    EXPECT_TRUE(memcmp(item.data.i32, activeArray, sizeof(activeArray)) == 0);

    result = FindCameraMetadataItem(metadata, OHOS_SENSOR_EXPOSURE_TIME, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 4);
    EXPECT_TRUE(item.item == OHOS_SENSOR_EXPOSURE_TIME);
    EXPECT_TRUE(item.data_type == META_TYPE_INT64);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.i64[0] == exposureTime);

    result = FindCameraMetadataItem(metadata, OHOS_SENSOR_COLOR_CORRECTION_GAINS, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 5);
    EXPECT_TRUE(item.item == OHOS_SENSOR_COLOR_CORRECTION_GAINS);
    EXPECT_TRUE(item.data_type == META_TYPE_FLOAT);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.f[0] == gain);

    result = FindCameraMetadataItem(metadata, OHOS_JPEG_GPS_COORDINATES, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 6);
    EXPECT_TRUE(item.item == OHOS_JPEG_GPS_COORDINATES);
    EXPECT_TRUE(item.data_type == META_TYPE_DOUBLE);
    EXPECT_TRUE(item.count == sizeof(gpsCoordinates)/sizeof(gpsCoordinates[0]));
    EXPECT_TRUE(memcmp(item.data.f, gpsCoordinates, sizeof(gpsCoordinates)) == 0);

    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_AE_COMPENSATION_STEP, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 7);
    EXPECT_TRUE(item.item == OHOS_CONTROL_AE_COMPENSATION_STEP);
    EXPECT_TRUE(item.data_type == META_TYPE_RATIONAL);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(memcmp(item.data.r, &rational, sizeof(rational)) == 0);
}

/*
* Feature: Metadata
* Function: Capture Mirror Support
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test operations(add/find) on mirroring metadata tags
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_011, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> cameraMetadata = std::make_shared<CameraMetadata>(2, 0);

    common_metadata_header_t *metadata = cameraMetadata->get();
    ASSERT_NE(metadata, nullptr);

    camera_metadata_item_t item;

    uint8_t captureMirrorSupport = 1;
    bool ret = cameraMetadata->addEntry(OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED,
               &captureMirrorSupport, 1);
    EXPECT_TRUE(ret == true);

    int32_t result = FindCameraMetadataItem(metadata, OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 0);
    EXPECT_TRUE(item.item == OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.u8[0] == captureMirrorSupport);

    uint8_t captureMirror = 1;
    ret = cameraMetadata->addEntry(OHOS_CONTROL_CAPTURE_MIRROR, &captureMirror, 1);
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_CAPTURE_MIRROR, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 1);
    EXPECT_TRUE(item.item == OHOS_CONTROL_CAPTURE_MIRROR);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.u8[0] == captureMirror);
}

/*
* Feature: Metadata
* Function: Flash Mode Support
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test operations(add/find) on flash metadata tags
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_012, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> cameraMetadata = std::make_shared<CameraMetadata>(2, 0);

    common_metadata_header_t *metadata = cameraMetadata->get();
    ASSERT_NE(metadata, nullptr);

    camera_metadata_item_t item;

    uint8_t availFlashMode[4] = {OHOS_CAMERA_FLASH_MODE_CLOSE, OHOS_CAMERA_FLASH_MODE_OPEN,
                                 OHOS_CAMERA_FLASH_MODE_AUTO, OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN};
    bool ret = cameraMetadata->addEntry(OHOS_ABILITY_DEVICE_AVAILABLE_FLASHMODES, availFlashMode,
                                   sizeof(availFlashMode)/sizeof(availFlashMode[0]));
    EXPECT_TRUE(ret == true);

    int32_t result = FindCameraMetadataItem(metadata, OHOS_ABILITY_DEVICE_AVAILABLE_FLASHMODES,
                     &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 0);
    EXPECT_TRUE(item.item == OHOS_ABILITY_DEVICE_AVAILABLE_FLASHMODES);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == sizeof(availFlashMode)/sizeof(availFlashMode[0]));
    EXPECT_TRUE(memcmp(item.data.u8, availFlashMode, sizeof(availFlashMode)) == 0);

    uint8_t flashMode = OHOS_CAMERA_FLASH_MODE_AUTO;
    ret = cameraMetadata->addEntry(OHOS_CONTROL_FLASHMODE, &flashMode, 1);
    EXPECT_TRUE(ret == true);

    result = FindCameraMetadataItem(metadata, OHOS_CONTROL_FLASHMODE, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 1);
    EXPECT_TRUE(item.item == OHOS_CONTROL_FLASHMODE);
    EXPECT_TRUE(item.data_type == META_TYPE_BYTE);
    EXPECT_TRUE(item.count == 1);
    EXPECT_TRUE(item.data.u8[0] == flashMode);
}

/*
* Feature: Metadata
* Function: Available basic configurations
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test operations(add/find) on stream configuration metadata tags
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_013, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> cameraMetadata = std::make_shared<CameraMetadata>(1, 0);

    camera_metadata_item_t item;

    int32_t availBasicConfig[12] = {OHOS_CAMERA_FORMAT_YCBCR_420_888, 4160, 3120,
                                    OHOS_CAMERA_FORMAT_YCBCR_420_888, 4160, 2600,
                                    OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1440, 1080,
                                    OHOS_CAMERA_FORMAT_JPEG, 1440, 1080};

    bool ret = cameraMetadata->addEntry(OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS,
                                        availBasicConfig,
                                        sizeof(availBasicConfig)/sizeof(availBasicConfig[0]));
    EXPECT_TRUE(ret == true);

    common_metadata_header_t *metadata = cameraMetadata->get();
    ASSERT_NE(metadata, nullptr);

    int32_t result = FindCameraMetadataItem(metadata,
                                            OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 0);
    EXPECT_TRUE(item.item == OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == sizeof(availBasicConfig)/sizeof(availBasicConfig[0]));
    EXPECT_TRUE(memcmp(item.data.i32, availBasicConfig, sizeof(availBasicConfig)) == 0);
}

/*
* Feature: Metadata
* Function: Sensor active array, zoom cap and scene zoom cap tags
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test operations(add/find) on sensor active array, zoom cap and scene zoom cap tags
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_014, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> cameraMetadata = std::make_shared<CameraMetadata>(3, 0);

    camera_metadata_item_t item;

    int32_t activeArray[4] = {0, 0, 2000, 1500};
    bool ret = cameraMetadata->addEntry(OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeArray,
                                        sizeof(activeArray) / sizeof(activeArray[0]));
    EXPECT_TRUE(ret == true);

    int32_t zoomCap[] = {100, 600};
    ret = cameraMetadata->addEntry(OHOS_ABILITY_ZOOM_CAP, zoomCap, sizeof(zoomCap) / sizeof(zoomCap[0]));
    EXPECT_TRUE(ret == true);

    int32_t zoomSceneCap[] = {100, 1000};
    ret = cameraMetadata->addEntry(OHOS_ABILITY_SCENE_ZOOM_CAP, zoomSceneCap,
                                   sizeof(zoomSceneCap) / sizeof(zoomSceneCap[0]));
    EXPECT_TRUE(ret == true);

    common_metadata_header_t *metadata = cameraMetadata->get();
    ASSERT_NE(metadata, nullptr);

    int32_t result = FindCameraMetadataItem(metadata, OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 0);
    EXPECT_TRUE(item.item == OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == sizeof(activeArray)/sizeof(activeArray[0]));
    EXPECT_TRUE(memcmp(item.data.i32, activeArray, sizeof(activeArray)) == 0);

    result = FindCameraMetadataItem(metadata, OHOS_ABILITY_ZOOM_CAP, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 1);
    EXPECT_TRUE(item.item == OHOS_ABILITY_ZOOM_CAP);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == sizeof(zoomCap)/sizeof(zoomCap[0]));
    EXPECT_TRUE(memcmp(item.data.i32, zoomCap, sizeof(zoomCap)) == 0);

    result = FindCameraMetadataItem(metadata, OHOS_ABILITY_SCENE_ZOOM_CAP, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 2);
    EXPECT_TRUE(item.item == OHOS_ABILITY_SCENE_ZOOM_CAP);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == sizeof(zoomSceneCap)/sizeof(zoomSceneCap[0]));
    EXPECT_TRUE(memcmp(item.data.i32, zoomSceneCap, sizeof(zoomSceneCap)) == 0);
}

/*
* Feature: Metadata
* Function: Crop region tag
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test operations(add/find) on crop region tag
*/
HWTEST_F(CameraMetadataUnitTest, camera_metadata_unittest_015, TestSize.Level0)
{
    std::shared_ptr<CameraMetadata> cameraMetadata = std::make_shared<CameraMetadata>(3, 0);

    camera_metadata_item_t item;

    int32_t cropRegion[4] = {0, 0, 2000, 1500};
    bool ret = cameraMetadata->addEntry(OHOS_CONTROL_ZOOM_CROP_REGION, cropRegion,
                                   sizeof(cropRegion) / sizeof(cropRegion[0]));
    EXPECT_TRUE(ret == true);

    common_metadata_header_t *metadata = cameraMetadata->get();
    ASSERT_NE(metadata, nullptr);

    int32_t result = FindCameraMetadataItem(metadata, OHOS_CONTROL_ZOOM_CROP_REGION, &item);
    EXPECT_TRUE(result == CAM_META_SUCCESS);
    EXPECT_TRUE(item.index == 0);
    EXPECT_TRUE(item.item == OHOS_CONTROL_ZOOM_CROP_REGION);
    EXPECT_TRUE(item.data_type == META_TYPE_INT32);
    EXPECT_TRUE(item.count == sizeof(cropRegion) / sizeof(cropRegion[0]));
    EXPECT_TRUE(memcmp(item.data.i32, cropRegion, sizeof(cropRegion)) == 0);
}
} // CameraStandard
} // OHOS
