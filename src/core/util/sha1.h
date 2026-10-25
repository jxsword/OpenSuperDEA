/**
 * @file sha1.h
 * @brief SHA-1 摘要（数据文件指纹，design_docs/02 册 §4.2 data_sha1）。
 * @details 自包含实现（FIPS 180-1），仅用于对拍同源判定与结果溯源，非安全用途。
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace opensuperdea::util
{
/**
 * @brief 计算 SHA-1 摘要。
 * @param data 输入字节
 * @return 40 字符小写十六进制摘要
 */
[[nodiscard]] std::string sha1_hex(std::string_view data);
} // namespace opensuperdea::util
