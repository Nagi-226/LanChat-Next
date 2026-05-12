// mini_asio.hpp  Minimal asio-compatible async I/O for LanChat-Next server
// Single-header, C++17, platform sockets, poll-based single-threaded event loop.
// Purpose: Replace standalone asio until it is vendored.
//
// Usage:
//   lanchat::vendor::asio::io_context ctx;
//   lanchat::vendor::asio::ip::tcp::acceptor acceptor(ctx, port);
//   acceptor.async_accept(socket, [](auto ec) { ... });
//   ctx.run();
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using native_socket_t = SOCKET;
constexpr native_socket_t kInvalidSocket = INVALID_SOCKET;
#define MINI_ASIO_EAGAIN WSAEWOULDBLOCK
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
using native_socket_t = int;
constexpr native_socket_t kInvalidSocket = -1;
#define MINI_ASIO_EAGAIN EAGAIN
#endif

namespace lanchat::vendor::asio {

using error_code = std::error_code;

inline native_socket_t make_nonblocking(native_socket_t fd) {
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
    return fd;
}

inline void close_native(native_socket_t fd) {
    if (fd == kInvalidSocket) return;
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

inline std::string last_socket_error() {
#ifdef _WIN32
    return "WSA error " + std::to_string(WSAGetLastError());
#else
    return "errno " + std::to_string(errno) + ": " + strerror(errno);
#endif
}

//  io_context 

class io_context {
public:
    io_context() {
#ifdef _WIN32
        WSADATA data{};
        WSAStartup(MAKEWORD(2, 2), &data);
#endif
    }

    ~io_context() {
        stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    io_context(const io_context&) = delete;
    io_context& operator=(const io_context&) = delete;

    void run();
    void stop();
    void post(std::function<void()> task);

    // Internal: register interest for a socket.
    using handler_t = std::function<void(error_code, size_t)>;
    using timer_handler_t = std::function<void(error_code)>;
    void add_read(native_socket_t fd, handler_t h);
    void add_write(native_socket_t fd, handler_t h);
    void add_timer(std::chrono::steady_clock::time_point deadline, timer_handler_t h);
    void del_socket(native_socket_t fd);

private:
    struct slot {
        native_socket_t fd = kInvalidSocket;
        handler_t on_read;
        handler_t on_write;
        std::chrono::steady_clock::time_point deadline;
        handler_t on_timer;
    };

    slot& get_or_create(native_socket_t fd);
    void poll_once(int timeoutMs);

    std::unordered_map<native_socket_t, slot> slots_;
    std::vector<std::function<void()>> pending_;
    std::vector<native_socket_t> timer_queue_;
    std::atomic<bool> stopped_{false};
    bool wsa_cleanup_{false};
};

namespace ip::tcp {

class socket {
public:
    socket(io_context& ctx) : ctx_(&ctx), fd_(kInvalidSocket) {}

    socket(socket&& other) noexcept
        : ctx_(other.ctx_), fd_(other.fd_) {
        other.fd_ = kInvalidSocket;
    }

    ~socket() { close(); }

    socket(const socket&) = delete;
    socket& operator=(const socket&) = delete;

    native_socket_t native_handle() const { return fd_; }

    void assign(native_socket_t fd) {
        close();
        fd_ = make_nonblocking(fd);
    }

    bool is_open() const { return fd_ != kInvalidSocket; }

    void close() {
        if (fd_ != kInvalidSocket) {
            ctx_->del_socket(fd_);
            close_native(fd_);
            fd_ = kInvalidSocket;
        }
    }

    template<typename Handler>
    void async_read_some(void* buffer, size_t maxSize, Handler&& handler) {
        ctx_->add_read(fd_, [this, buffer, maxSize, h = std::forward<Handler>(handler)]
                              (error_code ec, size_t) mutable {
            if (ec) { h(ec, 0); return; }
#ifdef _WIN32
            int n = ::recv(fd_, static_cast<char*>(buffer), static_cast<int>(maxSize), 0);
#else
            int n = static_cast<int>(::recv(fd_, buffer, maxSize, 0));
#endif
            if (n > 0) {
                h(error_code{}, static_cast<size_t>(n));
            } else if (n == 0) {
                h(std::make_error_code(std::errc::connection_reset), 0);
            } else {
#ifdef _WIN32
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
                    // Re-register and try again
                    async_read_some(buffer, maxSize, std::forward<Handler>(h));
                } else {
                    h(std::error_code(err, std::system_category()), 0);
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    async_read_some(buffer, maxSize, std::forward<Handler>(h));
                } else {
                    h(std::error_code(errno, std::system_category()), 0);
                }
#endif
            }
        });
    }

    template<typename Handler>
    void async_write_some(const void* data, size_t size, Handler&& handler) {
        ctx_->add_write(fd_, [this, data, size, h = std::forward<Handler>(handler)]
                              (error_code ec, size_t) mutable {
            if (ec) { h(ec, 0); return; }
#ifdef _WIN32
            int n = ::send(fd_, static_cast<const char*>(data), static_cast<int>(size), 0);
#else
            int n = static_cast<int>(::send(fd_, data, size, 0));
#endif
            if (n > 0) {
                h(error_code{}, static_cast<size_t>(n));
            } else {
#ifdef _WIN32
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
                    async_write_some(data, size, std::forward<Handler>(h));
                } else {
                    h(std::error_code(err, std::system_category()), 0);
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    async_write_some(data, size, std::forward<Handler>(h));
                } else {
                    h(std::error_code(errno, std::system_category()), 0);
                }
#endif
            }
        });
    }

private:
    io_context* ctx_;
    native_socket_t fd_;
};

class acceptor {
public:
    acceptor(io_context& ctx, uint16_t port) : ctx_(&ctx) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd_ == kInvalidSocket) {
            throw std::runtime_error("socket() failed: " + last_socket_error());
        }
        make_nonblocking(fd_);

        int reuse = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            close_native(fd_);
            throw std::runtime_error("bind() failed: " + last_socket_error());
        }
        if (::listen(fd_, SOMAXCONN) != 0) {
            close_native(fd_);
            throw std::runtime_error("listen() failed: " + last_socket_error());
        }
    }

    ~acceptor() { close(); }

    acceptor(const acceptor&) = delete;
    acceptor& operator=(const acceptor&) = delete;

    void close() {
        if (fd_ != kInvalidSocket) {
            ctx_->del_socket(fd_);
            close_native(fd_);
            fd_ = kInvalidSocket;
        }
    }

    template<typename Handler>
    void async_accept(socket& peer, Handler&& handler) {
        ctx_->add_read(fd_, [this, &peer, h = std::forward<Handler>(handler)]
                             (error_code ec, size_t) mutable {
            if (ec) { h(ec); return; }
            native_socket_t client = ::accept(fd_, nullptr, nullptr);
            if (client != kInvalidSocket) {
                peer.assign(client);
                h(error_code{});
            } else {
#ifdef _WIN32
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
                    async_accept(peer, std::forward<Handler>(h));
                } else {
                    h(std::error_code(err, std::system_category()));
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    async_accept(peer, std::forward<Handler>(h));
                } else {
                    h(std::error_code(errno, std::system_category()));
                }
#endif
            }
        });
    }

private:
    io_context* ctx_;
    native_socket_t fd_ = kInvalidSocket;
};

} // namespace ip::tcp

class steady_timer {
public:
    explicit steady_timer(io_context& ctx) : ctx_(&ctx) {}

    void expires_after(std::chrono::steady_clock::duration d) {
        deadline_ = std::chrono::steady_clock::now() + d;
    }

    template<typename Handler>
    void async_wait(Handler&& handler) {
        // Register a poll slot with a deadline
        // The io_context will invoke the handler when time expires
        ctx_->add_timer(deadline_, [this, h = std::forward<Handler>(handler)]
                                   (error_code ec) mutable {
            if (!ec) {
                deadline_ = std::chrono::steady_clock::time_point{}; // reset
            }
            h(ec);
        });
    }

    void cancel() {
        deadline_ = std::chrono::steady_clock::time_point{};
    }

private:
    friend class io_context;
    io_context* ctx_;
    std::chrono::steady_clock::time_point deadline_;
};

//  io_context implementation 

inline void io_context::add_read(native_socket_t fd, handler_t h) {
    auto& s = get_or_create(fd);
    s.fd = fd;
    s.on_read = std::move(h);
}

inline void io_context::add_write(native_socket_t fd, handler_t h) {
    auto& s = get_or_create(fd);
    s.fd = fd;
    s.on_write = std::move(h);
}

inline void io_context::del_socket(native_socket_t fd) {
    slots_.erase(fd);
}

inline io_context::slot& io_context::get_or_create(native_socket_t fd) {
    return slots_[fd];
}

inline void io_context::post(std::function<void()> task) {
    pending_.push_back(std::move(task));
}

inline void io_context::stop() {
    stopped_ = true;
}

inline void io_context::add_timer(
    std::chrono::steady_clock::time_point deadline, timer_handler_t h) {
    auto& s = get_or_create(kInvalidSocket);
    s.deadline = deadline;
    s.on_timer = [h = std::move(h)](error_code ec, size_t) { h(ec); };
    timer_queue_.push_back(kInvalidSocket);
}

inline void io_context::poll_once(int timeoutMs) {
#ifdef _WIN32
    std::vector<WSAPOLLFD> pfds;
#else
    std::vector<pollfd> pfds;
#endif
    std::vector<native_socket_t> fds;

    for (auto& [fd, s] : slots_) {
        if (fd == kInvalidSocket) continue; // timer entries
        short events = 0;
        if (s.on_read) events |= POLLIN;
        if (s.on_write) events |= POLLOUT;
        if (events == 0) continue;
#ifdef _WIN32
        pfds.push_back({fd, events, 0});
#else
        pfds.push_back({fd, events, 0});
#endif
        fds.push_back(fd);
    }

    // Check timers
    auto now = std::chrono::steady_clock::now();
    for (auto& [fd, s] : slots_) {
        if (fd != kInvalidSocket) continue;
        if (s.on_timer && s.deadline != std::chrono::steady_clock::time_point{}
            && now >= s.deadline) {
            auto h = std::move(s.on_timer);
            s.on_timer = nullptr;
            s.deadline = std::chrono::steady_clock::time_point{};
            h(error_code{}, 0);
        }
    }

    if (pfds.empty()) {
        // No sockets to poll, but we need to sleep for timer resolution
        // Use a short sleep to avoid busy-waiting
#ifdef _WIN32
        Sleep(static_cast<DWORD>(timeoutMs));
#else
        usleep(static_cast<useconds_t>(timeoutMs * 1000));
#endif
        return;
    }

#ifdef _WIN32
    int n = WSAPoll(pfds.data(), static_cast<ULONG>(pfds.size()), timeoutMs);
#else
    int n = ::poll(pfds.data(), pfds.size(), timeoutMs);
#endif
    if (n < 0) return;

    for (size_t i = 0; i < pfds.size(); ++i) {
        native_socket_t fd = fds[i];
        auto it = slots_.find(fd);
        if (it == slots_.end()) continue;

        auto& s = it->second;
        short revents =
#ifdef _WIN32
            pfds[i].revents;
#else
            pfds[i].revents;
#endif
        if ((revents & (POLLIN | POLLERR | POLLHUP)) && s.on_read) {
            auto h = std::move(s.on_read);
            s.on_read = nullptr;
            if (revents & (POLLERR | POLLHUP)) {
                h(std::make_error_code(std::errc::connection_reset), 0);
            } else {
                h(error_code{}, 0);
            }
        }
        if ((revents & POLLOUT) && s.on_write) {
            auto h = std::move(s.on_write);
            s.on_write = nullptr;
            h(error_code{}, 0);
        }
    }
}

inline void io_context::run() {
    stopped_ = false;
    while (!stopped_) {
        // Execute pending tasks
        auto tasks = std::move(pending_);
        pending_.clear();
        for (auto& t : tasks) {
            t();
        }

        // Poll sockets with 50ms timeout (for timer resolution)
        if (!stopped_) {
            poll_once(50);
        }
    }
}

} // namespace lanchat::vendor::asio
