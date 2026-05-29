#include "lanchat/server/FrameCodec.h"

#include <stdexcept>

namespace lanchat::server {

std::vector<std::uint8_t> FrameCodec::encode(const std::string& json)
{
    if (json.size() > MaxFrameBytes) {
        throw std::runtime_error("frame too large");
    }

    const auto len = static_cast<std::uint32_t>(json.size());
    std::vector<std::uint8_t> frame;
    frame.reserve(4 + json.size());
    frame.push_back(static_cast<std::uint8_t>((len >> 24) & 0xff));
    frame.push_back(static_cast<std::uint8_t>((len >> 16) & 0xff));
    frame.push_back(static_cast<std::uint8_t>((len >> 8) & 0xff));
    frame.push_back(static_cast<std::uint8_t>(len & 0xff));
    frame.insert(frame.end(), json.begin(), json.end());
    return frame;
}

bool FrameCodec::tryDecode(std::vector<std::uint8_t>& buffer, std::string& outJson)
{
    if (buffer.size() < 4) {
        return false;
    }

    const auto len =
        (static_cast<std::uint32_t>(buffer[0]) << 24) |
        (static_cast<std::uint32_t>(buffer[1]) << 16) |
        (static_cast<std::uint32_t>(buffer[2]) << 8) |
        static_cast<std::uint32_t>(buffer[3]);

    if (len == 0 || len > MaxFrameBytes) {
        buffer.clear();
        outJson.clear();
        return false;
    }

    if (buffer.size() < 4 + len) {
        return false;
    }

    outJson.assign(buffer.begin() + 4, buffer.begin() + 4 + len);
    buffer.erase(buffer.begin(), buffer.begin() + 4 + len);
    return true;
}

} // namespace lanchat::server
