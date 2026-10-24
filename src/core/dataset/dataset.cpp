/**
 * @file dataset.cpp
 * @brief 数据模型方法实现（轻量存取，逻辑集中在 builder/validator）。
 */
#include "dataset.h"

#include <algorithm>

namespace opensuperdea::dataset
{
std::size_t Dataset::dmu_count() const
{
    return dmu_names.size();
}

std::size_t Dataset::input_count() const
{
    return static_cast<std::size_t>(inputs.cols());
}

std::size_t Dataset::output_count() const
{
    return static_cast<std::size_t>(outputs.cols());
}

bool PanelData::is_balanced() const
{
    if (sections.empty())
    {
        return true;
    }
    const std::size_t reference_size = sections.begin()->second.dmu_names.size();
    const bool all_match =
        std::all_of(sections.begin(), sections.end(), [reference_size](const auto &entry) {
            return entry.second.dmu_names.size() == reference_size;
        });
    return all_match;
}
} // namespace opensuperdea::dataset
