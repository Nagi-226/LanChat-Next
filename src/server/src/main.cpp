#include "lanchat/server/LanChatServer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    std::uint16_t port = 12346;
    if (argc > 1) {
        const int parsed = std::atoi(argv[1]);
        if (parsed > 0 && parsed <= 65535) {
            port = static_cast<std::uint16_t>(parsed);
        } else {
            std::cerr << "Invalid port: " << argv[1] << "\n";
            return 2;
        }
    }

    lanchat::server::LanChatServer server(port);
    return server.run();
}
