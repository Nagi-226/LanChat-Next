#include "lanchat/server/Net.h"

#include <cassert>
#include <iostream>

int main()
{
    static_assert(lanchat::server::net_backend_is_real_asio,
                  "server transport must use standalone Asio for v2.0.0-a1");
    lanchat::server::net::io_context ctx;
    lanchat::server::net::post(ctx, [] {});
    ctx.poll();
    std::cout << "Asio transport tests passed\n";
    return 0;
}
