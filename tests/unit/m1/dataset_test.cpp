/**
 * @file dataset_test.cpp
 * @brief 数据模型存取与序列化契约测试（FR-ENG-004；02 册 §1）。
 */
#include "core/dataset/dataset.h"

#include <gtest/gtest.h>

namespace
{
using opensuperdea::dataset::Dataset;
using opensuperdea::dataset::PanelData;
} // namespace

TEST(DatasetModel, FR_ENG_004_DimensionAccessors)
{
    Dataset data;
    data.dmu_names = {"A", "B", "C"};
    data.inputs.resize(3, 2);
    data.outputs.resize(3, 1);
    EXPECT_EQ(data.dmu_count(), 3U);
    EXPECT_EQ(data.input_count(), 2U);
    EXPECT_EQ(data.output_count(), 1U);
}

TEST(DatasetModel, FR_ENG_004_MatrixRoundTrip)
{
    Dataset data;
    data.dmu_names = {"A", "B"};
    data.inputs.resize(2, 1);
    data.inputs << 1.5, 2.5;
    data.outputs.resize(2, 1);
    data.outputs << 3.5, 4.5;
    // 存取往返：写入值与读出值逐位一致（Eigen 双精度，无格式损失）
    EXPECT_DOUBLE_EQ(data.inputs(1, 0), 2.5);
    EXPECT_DOUBLE_EQ(data.outputs(0, 0), 3.5);
}

TEST(DatasetModel, FR_ENG_004_PanelBalanceCheck)
{
    PanelData panel;
    panel.periods = {"2019", "2020"};
    Dataset first;
    first.dmu_names = {"A", "B"};
    Dataset second;
    second.dmu_names = {"A"};
    panel.sections = {{"2019", first}, {"2020", second}};
    EXPECT_FALSE(panel.is_balanced());
    panel.sections.at("2020").dmu_names = {"A", "B"};
    EXPECT_TRUE(panel.is_balanced());
}
