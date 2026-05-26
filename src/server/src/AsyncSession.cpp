#include "lanchat/server/AsyncSession.h"
#include "lanchat/server/FrameCodec.h"
#include "lanchat/server/ProtocolJson.h"
#include "lanchat/server/ServerLogger.h"
#include "lanchat/server/TcpServer.h"

namespace lanchat::server {

std::atomic<uint64_t> AsyncSession::next_id_{1};

AsyncSession::AsyncSession(vendor::asio::io_context& ctx, TcpServer& server)
    : ctx_(ctx), server_(server), socket_(ctx), id_(next_id_.fetch_add(1, std::memory_order_relaxed)) {
    last_heartbeat_ = std::chrono::steady_clock::now();
}

AsyncSession::~AsyncSession() {
    close();
}

void AsyncSession::start() {
    ServerLogger::instance().info(
        "session " + std::to_string(id_) + " connected");
    last_heartbeat_ = std::chrono::steady_clock::now();
    read_buffer_.resize(65536);
    doRead();
}

void AsyncSession::deliver(const std::string& json) {
    auto frame = FrameCodec::encode(json);
    write_queue_.push_back(
        std::string(reinterpret_cast<const char*>(frame.data()), frame.size()));
    if (!writing_) {
        doWrite();
    }
}

void AsyncSession::close() {
    socket_.close();
}

void AsyncSession::doRead() {
    auto self = shared_from_this();
    socket_.async_read_some(read_buffer_.data(), read_buffer_.size(),
        [this, self](vendor::asio::error_code ec, size_t bytes) {
            if (ec) {
                ServerLogger::instance().info(
                    "session " + std::to_string(id_) + " read error, closing");
                server_.removeSession(id_);
                return;
            }
            read_accumulator_.insert(read_accumulator_.end(),
                                     read_buffer_.begin(), read_buffer_.begin() + bytes);

            std::string message;
            while (FrameCodec::tryDecode(read_accumulator_, message)) {
                onMessage(message);
            }

            // Cap per-session buffering so malformed peers cannot grow memory forever.
            if (read_accumulator_.size() > 4 * 1024 * 1024) {
                read_accumulator_.clear();
            }

            doRead();
        });
}

void AsyncSession::doWrite() {
    if (write_queue_.empty()) {
        writing_ = false;
        return;
    }
    writing_ = true;
    auto self = shared_from_this();
    auto& front = write_queue_.front();
    socket_.async_write_some(front.data() + write_offset_, front.size() - write_offset_,
        [this, self](vendor::asio::error_code ec, size_t bytes) {
            if (ec) {
                write_queue_.clear();
                write_offset_ = 0;
                writing_ = false;
                return;
            }
            write_offset_ += bytes;
            if (write_offset_ < write_queue_.front().size()) {
                doWrite();
                return;
            }
            write_queue_.erase(write_queue_.begin());
            write_offset_ = 0;
            doWrite();
        });
}

void AsyncSession::onMessage(const std::string& json) {
    bool ok = false;
    const auto request = protocol_json::parseObject(json, ok);
    if (ok && protocol_json::typeField(request) == lanchat::protocol::MsgType::Heartbeat) {
        heartbeatReceived();
        deliver(protocol_json::ok(lanchat::protocol::MsgType::HeartbeatAck));
        return;
    }
    // Forward to server for dispatch
    server_.onMessage(shared_from_this(), json);
}

void AsyncSession::heartbeatReceived() {
    last_heartbeat_ = std::chrono::steady_clock::now();
    missed_heartbeats_ = 0;
}

bool AsyncSession::isTimedOut() const {
    auto elapsed = std::chrono::steady_clock::now() - last_heartbeat_;
    return elapsed > std::chrono::seconds(90);
}

} // namespace lanchat::server
