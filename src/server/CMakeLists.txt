add_executable(lanchat_server_next
  src/main.cpp
  src/LanChatServer.cpp
  src/TcpServer.cpp
  src/FrameCodec.cpp
  src/ServerLogger.cpp
  src/AsyncSession.cpp
  src/db/Database.cpp
  src/db/UserRepository.cpp
  src/db/LegacyMigrator.cpp
)

target_compile_features(lanchat_server_next PRIVATE cxx_std_17)
target_include_directories(lanchat_server_next PRIVATE include ../../protocol vendor)

if(WIN32)
  target_link_libraries(lanchat_server_next PRIVATE ws2_32)
endif()

message(STATUS "LanChat-Next server: using vendored mini-asio/mini-json/mini-spdlog")

if(LANCHAT_BUILD_TESTS)
  add_executable(lanchat_frame_codec_tests
    ../../tests/cpp/frame_codec_tests.cpp
    src/FrameCodec.cpp
  )
  target_compile_features(lanchat_frame_codec_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_frame_codec_tests PRIVATE include ../../protocol)
  add_test(NAME frame_codec COMMAND lanchat_frame_codec_tests)
endif()
