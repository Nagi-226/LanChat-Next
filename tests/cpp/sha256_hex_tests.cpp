// sha256_hex_tests.cpp —— 真 SHA-256 的已知答案测试（NIST 标准向量）。
// 为什么需要：vendor/sha256.hpp 名字里有 sha256 但实际是 XOR/乘法玩具哈希
// （其注释自认 not cryptographically secure），只配用于 legacy v0 校验。
// 密码预哈希（password_version=2，防 bcrypt 72 字节截断）必须用真 SHA-256，
// 所以新实现 sha256_hex.hpp 要用标准向量证明它算得对，而不是"看起来像"。
#include "sha256_hex.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    using lanchat::vendor::sha256::hex_digest;

    // NIST FIPS 180-4 示例向量
    assert(hex_digest("") ==
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert(hex_digest("abc") ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(hex_digest("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
           "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    // 100 万个 'a'：跨多块 + 长度填充边界
    assert(hex_digest(std::string(1000000, 'a')) ==
           "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    // 72/73 字节边界：预哈希要把任意长度压到 64 字符，边界两侧都必须正确
    assert(hex_digest(std::string(72, 'x')).size() == 64);
    assert(hex_digest(std::string(73, 'x')) != hex_digest(std::string(72, 'x')));
    // 前 72 字节相同、尾部不同的两个长密码：预哈希后必须不同（截断修复的核心）
    const std::string a = std::string(72, 'A') + "TAIL-ONE";
    const std::string b = std::string(72, 'A') + "TAIL-TWO";
    assert(hex_digest(a) != hex_digest(b));

    std::cout << "SHA-256 hex tests passed\n";
    return 0;
}