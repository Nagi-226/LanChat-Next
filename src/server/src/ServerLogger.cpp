#include "lanchat/server/ServerLogger.h"

#include "mini_spdlog.hpp"

#ifdef _WIN32
#include <direct.h>
#define mkdir(p) _mkdir(p)
#else
#include <sys/stat.h>
#endif

namespace lanchat::server {

ServerLogger& ServerLogger::instance() {
    static ServerLogger logger;
    return logger;
}

void ServerLogger::init(const std::string& logDir) {
    mkdir(logDir.c_str());
    std::string logPath = logDir + "/lanchat-server.log";
    vendor::spdlog::Logger::instance().init(logPath, vendor::spdlog::Level::Info);
}

void ServerLogger::debug(const std::string& msg) { vendor::spdlog::Logger::instance().debug(msg); }
void ServerLogger::info(const std::string& msg)  { vendor::spdlog::Logger::instance().info(msg); }
void ServerLogger::warn(const std::string& msg)  { vendor::spdlog::Logger::instance().warn(msg); }
void ServerLogger::error(const std::string& msg) { vendor::spdlog::Logger::instance().error(msg); }

} // namespace lanchat::server
