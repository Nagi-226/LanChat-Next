#include "lanchat/server/LanChatServer.h"

#include "lanchat/server/TcpServer.h"

namespace lanchat::server {

LanChatServer::LanChatServer(std::uint16_t port) : port_(port) {}

int LanChatServer::run()
{
    TcpServer server(port_);
    return server.run();
}

} // namespace lanchat::server
