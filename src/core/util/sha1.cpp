/**
 * @file sha1.cpp
 * @brief SHA-1 实现（FIPS 180-1；自包含无依赖）。
 */
#include "sha1.h"

#include <array>
#include <cstring>

namespace opensuperdea::util
{
namespace
{
/** @brief 循环左移。 */
std::uint32_t rotate_left(std::uint32_t value, unsigned int bits)
{
    return (value << bits) | (value >> (32U - bits));
}

/** @brief 大端写入 32 位字。 */
void append_hex(std::string &out, std::uint32_t word)
{
    static const char *k_digits = "0123456789abcdef";
    for (unsigned int round = 0U; round < 8U; ++round)
    {
        const unsigned int shift = 28U - (round * 4U);
        out.push_back(k_digits[(word >> shift) & 0xFU]);
    }
}
} // namespace

std::string sha1_hex(std::string_view data)
{
    std::uint32_t h0 = 0x67452301U;
    std::uint32_t h1 = 0xEFCDAB89U;
    std::uint32_t h2 = 0x98BADCFEU;
    std::uint32_t h3 = 0x10325476U;
    std::uint32_t h4 = 0xC3D2E1F0U;

    // 消息填充：追加 0x80、补零至 56 mod 64、追加 64 位大端位长
    std::string message(data);
    const std::uint64_t bit_length = static_cast<std::uint64_t>(message.size()) * 8U;
    message.push_back(static_cast<char>(0x80));
    while (message.size() % 64U != 56U)
    {
        message.push_back('\0');
    }
    for (unsigned int round = 0U; round < 8U; ++round)
    {
        const unsigned int shift = 56U - (round * 8U);
        message.push_back(static_cast<char>((bit_length >> shift) & 0xFFU));
    }

    std::array<std::uint32_t, 80> words{};
    for (std::size_t offset = 0; offset < message.size(); offset += 64U)
    {
        for (std::size_t i = 0; i < 16; ++i)
        {
            const auto *block =
                reinterpret_cast<const unsigned char *>(message.data() + offset + (i * 4U));
            words[i] = (static_cast<std::uint32_t>(block[0]) << 24U) |
                       (static_cast<std::uint32_t>(block[1]) << 16U) |
                       (static_cast<std::uint32_t>(block[2]) << 8U) |
                       static_cast<std::uint32_t>(block[3]);
        }
        for (std::size_t i = 16; i < 80; ++i)
        {
            words[i] = rotate_left(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
        }

        std::uint32_t a = h0;
        std::uint32_t b = h1;
        std::uint32_t c = h2;
        std::uint32_t d = h3;
        std::uint32_t e = h4;
        for (std::size_t i = 0; i < 80; ++i)
        {
            std::uint32_t f = 0;
            std::uint32_t k = 0;
            if (i < 20)
            {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999U;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1U;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDCU;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6U;
            }
            const std::uint32_t temp = rotate_left(a, 5) + f + e + k + words[i];
            e = d;
            d = c;
            c = rotate_left(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::string hex;
    hex.reserve(40);
    append_hex(hex, h0);
    append_hex(hex, h1);
    append_hex(hex, h2);
    append_hex(hex, h3);
    append_hex(hex, h4);
    return hex;
}
} // namespace opensuperdea::util
