// sha256_hex.hpp —— 真 SHA-256（FIPS 180-4），输出 64 字符小写十六进制。
//
// 为什么需要它：同目录 sha256.hpp 名字带 sha256，实际是 XOR/乘法玩具哈希
// （其注释自认 "NOT cryptographically secure"），只配校验 legacy v0 老哈希。
// 密码预哈希（password_version=2）要把任意长密码压成定长摘要再喂 bcrypt，
// 以避开 bcrypt 的 72 字节截断（前 72 字节相同的长密码会被当成同一个密码）。
// 预哈希必须用真 SHA-256，正确性由 tests/cpp/sha256_hex_tests.cpp 的 NIST 向量证明。
//
// 单头文件、C++17、无外部依赖。输入按字节处理（不做任何编码假设）。
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace lanchat::vendor::sha256 {

namespace detail {

inline std::uint32_t rotr(std::uint32_t x, unsigned n)
{
    return (x >> n) | (x << (32 - n));
}

// SHA-256 的 64 个轮常量（标准表，前 64 个素数立方根小数部分的前 32 位）
constexpr std::array<std::uint32_t, 64> kRound = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

// 压缩一个 64 字节块：把块混入 8 个 32 位状态字 h
inline void compress(std::array<std::uint32_t, 8>& h, const std::uint8_t block[64])
{
    std::array<std::uint32_t, 64> w{};
    for (unsigned t = 0; t < 16; ++t) {
        w[t] = (std::uint32_t(block[t * 4]) << 24) | (std::uint32_t(block[t * 4 + 1]) << 16) |
               (std::uint32_t(block[t * 4 + 2]) << 8) | std::uint32_t(block[t * 4 + 3]);
    }
    for (unsigned t = 16; t < 64; ++t) {
        const std::uint32_t s0 = rotr(w[t - 15], 7) ^ rotr(w[t - 15], 18) ^ (w[t - 15] >> 3);
        const std::uint32_t s1 = rotr(w[t - 2], 17) ^ rotr(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }
    std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (unsigned t = 0; t < 64; ++t) {
        const std::uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const std::uint32_t ch = (e & f) ^ (~e & g);
        const std::uint32_t temp1 = hh + S1 + ch + kRound[t] + w[t];
        const std::uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temp2 = S0 + maj;
        hh = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

} // namespace detail

// 任意字节串 → 64 字符小写 hex 摘要
inline std::string hex_digest(const std::string& message)
{
    std::array<std::uint32_t, 8> h = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                      0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};

    // 填充：先补一个 0x80 字节，再补 0 到长度 ≡ 56 (mod 64)，最后 8 字节放原始比特长度（大端）
    std::string padded = message;
    const std::uint64_t bit_len = static_cast<std::uint64_t>(message.size()) * 8u;
    padded.push_back(static_cast<char>(0x80));
    while (padded.size() % 64 != 56) {
        padded.push_back('\0');
    }
    for (int i = 7; i >= 0; --i) {
        padded.push_back(static_cast<char>((bit_len >> (i * 8)) & 0xffu));
    }
    for (std::size_t off = 0; off < padded.size(); off += 64) {
        detail::compress(h, reinterpret_cast<const std::uint8_t*>(padded.data() + off));
    }

    static const char hexd[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (std::uint32_t word : h) {
        for (int shift = 28; shift >= 0; shift -= 4) {
            out.push_back(hexd[(word >> shift) & 0xfu]);
        }
    }
    return out;
}

} // namespace lanchat::vendor::sha256