/**
 * @file solve_options.cpp
 * @brief 求解选项公共类型的字符串互转实现。
 */
#include "solve_options.h"

namespace opensuperdea::registry
{
bool orientation_from_string(const std::string &text, Orientation &out)
{
    if (text == "in")
    {
        out = Orientation::INPUT;
        return true;
    }
    if (text == "out")
    {
        out = Orientation::OUTPUT;
        return true;
    }
    if (text == "non")
    {
        out = Orientation::NON_DIRECTIONAL;
        return true;
    }
    return false;
}

std::string to_string(Orientation orientation)
{
    switch (orientation)
    {
    case Orientation::INPUT:
        return "in";
    case Orientation::OUTPUT:
        return "out";
    case Orientation::NON_DIRECTIONAL:
        return "non";
    }
    return "in";
}

bool rts_from_string(const std::string &text, Rts &out)
{
    if (text == "crs")
    {
        out = Rts::CRS;
        return true;
    }
    if (text == "vrs")
    {
        out = Rts::VRS;
        return true;
    }
    if (text == "nirs")
    {
        out = Rts::NIRS;
        return true;
    }
    if (text == "ndrs")
    {
        out = Rts::NDRS;
        return true;
    }
    return false;
}

std::string to_string(Rts rts)
{
    switch (rts)
    {
    case Rts::CRS:
        return "crs";
    case Rts::VRS:
        return "vrs";
    case Rts::NIRS:
        return "nirs";
    case Rts::NDRS:
        return "ndrs";
    }
    return "crs";
}
} // namespace opensuperdea::registry
