#ifndef LANCHAT_SERVER_LANCHAT_SERVER_H
#define LANCHAT_SERVER_LANCHAT_SERVER_H

#include <cstdint>

namespace lanchat::server {

class LanChatServer {
public:
    explicit LanChatServer(std::uint16_t port = 12346);
    int run();

private:
    std::uint16_t port_;
};

} // namespace lanchat::server

#endif // LANCHAT_SERVER_LANCHAT_SERVER_H
