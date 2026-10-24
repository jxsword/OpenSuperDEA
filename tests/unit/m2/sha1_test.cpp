/**
 * @file sha1_test.cpp
 * @brief SHA-1 指纹测试（FIPS 180-1 标准向量；data_sha1 来源，02 册 §4.2）。
 */
#include "core/util/sha1.h"

#include <gtest/gtest.h>

TEST(Sha1, FR_OUT_001_KnownVectors)
{
    EXPECT_EQ(opensuperdea::util::sha1_hex(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    EXPECT_EQ(opensuperdea::util::sha1_hex("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
    EXPECT_EQ(
        opensuperdea::util::sha1_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
        "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}
