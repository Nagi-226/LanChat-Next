#include "lanchat/server/FrameCodec.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using lanchat::server::FrameCodec;

namespace {
void assertRoundTrip()
{
    const std::string body = R"({"type":20})";
    auto frame = FrameCodec::encode(body);
    assert(frame.size() == body.size() + 4);
    assert(frame[0] == 0);
    assert(frame[1] == 0);
    assert(frame[2] == 0);
    assert(frame[3] == body.size());

    std::string decoded;
    assert(FrameCodec::tryDecode(frame, decoded));
    assert(decoded == body);
    assert(frame.empty());
}

void assertPartialFrameWaits()
{
    auto frame = FrameCodec::encode(R"({"type":20})");
    frame.pop_back();
    std::string decoded;
    assert(!FrameCodec::tryDecode(frame, decoded));
    assert(decoded.empty());
    assert(!frame.empty());
}

void assertStickyFramesDecodeInOrder()
{
    auto first = FrameCodec::encode(R"({"type":20})");
    auto second = FrameCodec::encode(R"({"type":23,"id":1})");
    first.insert(first.end(), second.begin(), second.end());

    std::string decoded;
    assert(FrameCodec::tryDecode(first, decoded));
    assert(decoded == R"({"type":20})");
    assert(FrameCodec::tryDecode(first, decoded));
    assert(decoded == R"({"type":23,"id":1})");
    assert(first.empty());
}

void assertInvalidLengthReturnsError()
{
    std::vector<std::uint8_t> frame{0, 0, 0, 0};
    std::string decoded;
    assert(!FrameCodec::tryDecode(frame, decoded));
    assert(decoded.empty());
    assert(frame.empty());
}

void assertFrameSizeLimitIs256KiB()
{
    const std::string ok(256 * 1024, 'x');
    const std::string tooLarge(256 * 1024 + 1, 'x');
    assert(FrameCodec::encode(ok).size() == ok.size() + 4);

    bool threw = false;
    try {
        (void)FrameCodec::encode(tooLarge);
    } catch (...) {
        threw = true;
    }
    assert(threw);
}
} // namespace

int main()
{
    assertRoundTrip();
    assertPartialFrameWaits();
    assertStickyFramesDecodeInOrder();
    assertInvalidLengthReturnsError();
    assertFrameSizeLimitIs256KiB();
    std::cout << "FrameCodec tests passed\n";
    return 0;
}
