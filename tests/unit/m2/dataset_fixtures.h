/**
 * @file dataset_fixtures.h
 * @brief M2 测试共用数据集构造（程序内直接构造，不依赖外部文件）。
 * @details toy_cross 与 tests/golden/toy_cross/data.csv 同数据同序，便于
 *          「单测手算值 ↔ deaR 对拍值」交叉印证。
 */
#pragma once

#include "core/dataset/dataset.h"

#include <string>
#include <vector>

namespace test_fixtures
{
/** @brief toy_cross：4 DMU × (2 投入 + 1 产出)，手算基准与 deaR 对拍双确认。 */
inline opensuperdea::dataset::Dataset make_toy_cross()
{
    opensuperdea::dataset::Dataset data;
    data.dmu_names = {"A", "B", "C", "D"};
    data.input_names = {"in_labor", "in_capital"};
    data.output_names = {"out_revenue"};
    data.inputs.resize(4, 2);
    data.outputs.resize(4, 1);
    // A(2,1→1) B(3,2→3) C(4,3→6) D(5,4→7)
    data.inputs << 2.0, 1.0, 3.0, 2.0, 4.0, 3.0, 5.0, 4.0;
    data.outputs << 1.0, 3.0, 6.0, 7.0;
    return data;
}

/** @brief 单 DMU 数据集（VRS 超效率不可行判定用例）。 */
inline opensuperdea::dataset::Dataset make_single_dmu()
{
    opensuperdea::dataset::Dataset data;
    data.dmu_names = {"only"};
    data.input_names = {"in_x"};
    data.output_names = {"out_y"};
    data.inputs.resize(1, 1);
    data.outputs.resize(1, 1);
    data.inputs << 1.0;
    data.outputs << 1.0;
    return data;
}
} // namespace test_fixtures
