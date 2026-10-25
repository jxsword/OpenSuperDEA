/**
 * @file dmu_result.cpp
 * @brief DmuResult 的状态字符串转换与数值判定。
 */
#include "dmu_result.h"

#include <cmath>

namespace opensuperdea::result
{
std::string to_string(DmuStatus status)
{
    switch (status)
    {
    case DmuStatus::EFFICIENT:
        return "efficient";
    case DmuStatus::INEFFICIENT:
        return "inefficient";
    case DmuStatus::SUPER_EFFICIENT:
        return "super_efficient";
    case DmuStatus::INFEASIBLE:
        return "infeasible";
    case DmuStatus::NON_UNIQUE_NOTE:
        return "non_unique_note";
    }
    return "inefficient";
}

bool DmuResult::has_score() const
{
    return std::isfinite(score);
}
} // namespace opensuperdea::result
