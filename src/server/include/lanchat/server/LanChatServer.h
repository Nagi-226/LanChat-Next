#pragma once

#include "db/Database.h"
#include "db/UserRepository.h"

#include <cstdint>
#include <memory>
#include <string>

namespace lanchat::server {

class LanChatServer {
public:
    explicit LanChatServer(std::uint16_t port = 12346,
                           const std::string& dataDir = "data");
    int run();

    db::Database& database() { return *database_; }
    db::UserRepository& users() { return *users_; }

private:
    std::uint16_t port_;
    std::unique_ptr<db::Database> database_;
    std::unique_ptr<db::UserRepository> users_;
};

} // namespace lanchat::server
