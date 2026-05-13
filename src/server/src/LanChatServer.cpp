#include "lanchat/server/LanChatServer.h"
#include "lanchat/server/ServerLogger.h"
#include "lanchat/server/TcpServer.h"

namespace lanchat::server {

LanChatServer::LanChatServer(std::uint16_t port, const std::string& dataDir)
    : port_(port) {
    database_ = std::make_unique<db::Database>(dataDir);
    database_->open();
    users_ = std::make_unique<db::UserRepository>(*database_);
    messages_ = std::make_unique<db::MessageRepository>(*database_);
    channels_ = std::make_unique<db::ChannelRepository>(*database_);
}

int LanChatServer::run() {
    TcpServer server(port_, *users_, *messages_, *channels_);
    return server.run();
}

} // namespace lanchat::server
