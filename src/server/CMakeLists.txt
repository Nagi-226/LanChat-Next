add_executable(lanchat_server_next
  src/main.cpp
  src/LanChatServer.cpp
  src/TcpServer.cpp
  src/FrameCodec.cpp
)

target_compile_features(lanchat_server_next PRIVATE cxx_std_17)
target_include_directories(lanchat_server_next PRIVATE include ../../protocol)

if(WIN32)
  target_link_libraries(lanchat_server_next PRIVATE ws2_32)
endif()

find_path(ASIO_INCLUDE_DIR asio.hpp)
if(ASIO_INCLUDE_DIR)
  target_include_directories(lanchat_server_next PRIVATE ${ASIO_INCLUDE_DIR})
  target_compile_definitions(lanchat_server_next PRIVATE LANCHAT_HAS_STANDALONE_ASIO=1)
else()
  message(STATUS "Standalone asio.hpp not found; v1.1.0 skeleton uses the WinSock listener fallback. Add asio before v1.1.3.")
endif()
