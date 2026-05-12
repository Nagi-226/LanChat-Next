#pragma once

#include <string>

namespace lanchat::server {

class ServerLogger {
public:
    static ServerLogger& instance();

    void init(const std::string& logDir = "logs");

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    ServerLogger() = default;
};

} // namespace lanchat::server
