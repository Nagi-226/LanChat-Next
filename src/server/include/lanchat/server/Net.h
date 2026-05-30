#pragma once

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE 1
#endif

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include <asio.hpp>

namespace lanchat::server {

namespace net = ::asio;

inline constexpr bool net_backend_is_real_asio = true;

} // namespace lanchat::server
