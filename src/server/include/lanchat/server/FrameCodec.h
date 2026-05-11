#ifndef LANCHAT_SERVER_FRAME_CODEC_H
#define LANCHAT_SERVER_FRAME_CODEC_H

#include <cstdint>
#include <string>
#include <vector>

namespace lanchat::server {

class FrameCodec {
public:
    static std::vector<std::uint8_t> encode(const std::string& json);
    static bool tryDecode(std::vector<std::uint8_t>& buffer, std::string& outJson);
};

} // namespace lanchat::server

#endif // LANCHAT_SERVER_FRAME_CODEC_H
