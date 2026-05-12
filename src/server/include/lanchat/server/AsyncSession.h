#pragma once

#include "mini_asio.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace lanchat::server {

class TcpServer;

class AsyncSession : public std::enable_shared_from_this<AsyncSession> {
public:
    AsyncSession(vendor::asio::io_context& ctx, TcpServer& server);
    ~AsyncSession();

    AsyncSession(const AsyncSession&) = delete;
    AsyncSession& operator=(const AsyncSession&) = delete;

    vendor::asio::ip::tcp::socket& socket() { return socket_; }

    void start();
    void deliver(const std::string& json);
    void close();

    uint64_t id() const { return id_; }
    int userId() const { return user_id_; }
    void setUserId(int id) { user_id_ = id; }
    bool authenticated() const { return user_id_ != 0; }

    void heartbeatReceived();
    bool isTimedOut() const;
    int missedHeartbeats() const { return missed_heartbeats_; }

private:
    void doRead();
    void doWrite();
    void onMessage(const std::string& json);

    vendor::asio::io_context& ctx_;
    TcpServer& server_;
    vendor::asio::ip::tcp::socket socket_;
    uint64_t id_;
    int user_id_ = 0;

    std::vector<uint8_t> read_buffer_;
    std::vector<uint8_t> read_accumulator_;
    std::vector<std::string> write_queue_;
    size_t write_offset_ = 0;
    bool writing_ = false;

    std::chrono::steady_clock::time_point last_heartbeat_;
    int missed_heartbeats_ = 0;

    static uint64_t next_id_;
};

} // namespace lanchat::server
