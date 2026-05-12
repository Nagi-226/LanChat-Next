// mini_spdlog.hpp  Minimal thread-safe logger for LanChat-Next server
// Single-header, C++17, no external dependencies.
// Purpose: Replace spdlog until it is vendored (v1.2.0).
//
// Features: file+console output, timestamped entries, env-var log level
// Usage:
//   auto& log = lanchat::vendor::spdlog::Logger::instance();
//   log.init("logs/lanchat-server.log");
//   log.info("Server started on port {}", 12346);
#pragma once

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace lanchat::vendor::spdlog {

enum class Level { Debug = 0, Info = 1, Warn = 2, Error = 3, Off = 4 };

inline const char* level_name(Level lv) {
    switch (lv) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
        default:           return "---- ";
    }
}

class Logger {
public:
    static Logger& instance() {
        static Logger log;
        return log;
    }

    void init(const std::string& filePath, Level minLevel = Level::Info,
              size_t maxFileSize = 5 * 1024 * 1024, int maxFiles = 3) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_path_ = filePath;
        min_level_ = minLevel;
        max_file_size_ = maxFileSize;
        max_files_ = maxFiles;

        // Check env var for log level override
        const char* env = std::getenv("LANCHAT_LOG_LEVEL");
        if (env) {
            std::string s(env);
            if (s == "debug") min_level_ = Level::Debug;
            else if (s == "warn") min_level_ = Level::Warn;
            else if (s == "error") min_level_ = Level::Error;
            else if (s == "off") min_level_ = Level::Off;
        }

        open_log_file();
    }

    void debug(const std::string& msg)   { log(Level::Debug, msg); }
    void info(const std::string& msg)    { log(Level::Info,  msg); }
    void warn(const std::string& msg)    { log(Level::Warn,  msg); }
    void error(const std::string& msg)   { log(Level::Error, msg); }

private:
    Logger() = default;

    void open_log_file() {
        if (file_stream_.is_open()) file_stream_.close();
        file_stream_.open(file_path_, std::ios::out | std::ios::app);
        if (!file_stream_.is_open()) {
            std::fprintf(stderr, "[LOGGER] cannot open %s\n", file_path_.c_str());
        }
    }

    void rotate_if_needed() {
        if (!file_stream_.is_open()) return;
        auto pos = file_stream_.tellp();
        if (static_cast<size_t>(pos) >= max_file_size_) {
            file_stream_.close();
            // Rotate: log.2 -> log.3, log.1 -> log.2, log -> log.1
            for (int i = max_files_ - 1; i >= 0; --i) {
                std::string src = (i == 0) ? file_path_ : file_path_ + "." + std::to_string(i);
                std::string dst = file_path_ + "." + std::to_string(i + 1);
                std::remove(dst.c_str());
                std::rename(src.c_str(), dst.c_str());
            }
            open_log_file();
        }
    }

    void log(Level lv, const std::string& msg) {
        if (lv < min_level_) return;

        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) % 1000;

        char time_buf[32];
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &time_t);
#else
        localtime_r(&time_t, &tm_buf);
#endif
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_buf);

        char line[2048];
        int thread_id = static_cast<int>(
            std::hash<std::thread::id>{}(std::this_thread::get_id()) % 10000);
        std::snprintf(line, sizeof(line), "[%s.%03lld] [%s] [%04d] %s\n",
                      time_buf, static_cast<long long>(ms.count()),
                      level_name(lv), thread_id, msg.c_str());

        // Console output
        std::fputs(line, stderr);

        // File output
        if (file_stream_.is_open()) {
            rotate_if_needed();
            file_stream_ << line;
            file_stream_.flush();
        }
    }

    std::mutex mutex_;
    std::string file_path_;
    std::ofstream file_stream_;
    Level min_level_ = Level::Info;
    size_t max_file_size_ = 5 * 1024 * 1024;
    int max_files_ = 3;
};

} // namespace lanchat::vendor::spdlog
