add_executable(lanchat_server_next
  src/main.cpp
  src/LanChatServer.cpp
  src/TcpServer.cpp
  src/SessionPool.cpp
  src/PresenceManager.cpp
  src/MessageRouter.cpp
  src/FrameCodec.cpp
  src/ServerLogger.cpp
  src/AsyncSession.cpp
  src/db/Database.cpp
  src/db/UserRepository.cpp
  src/db/MessageRepository.cpp
  src/db/ChannelRepository.cpp
  src/db/LegacyMigrator.cpp
  vendor/sqlite/sqlite3.c
)

target_compile_definitions(lanchat_server_next PRIVATE
  NOMINMAX
  SQLITE_ENABLE_FTS5
  SQLITE_ENABLE_JSON1
)
target_compile_features(lanchat_server_next PRIVATE cxx_std_17)
target_include_directories(lanchat_server_next PRIVATE include ../../protocol vendor vendor/sqlite)

if(WIN32)
  target_link_libraries(lanchat_server_next PRIVATE ws2_32)
endif()

message(STATUS "LanChat-Next server: using vendored mini-asio/mini-json/mini-spdlog")

if(LANCHAT_BUILD_TESTS)
  # Existing test
  add_executable(lanchat_frame_codec_tests
    ../../tests/cpp/frame_codec_tests.cpp
    src/FrameCodec.cpp
  )
  target_compile_features(lanchat_frame_codec_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_frame_codec_tests PRIVATE include ../../protocol)
  add_test(NAME frame_codec COMMAND lanchat_frame_codec_tests)

  # ProtocolJson inline tests (no link deps)
  add_executable(lanchat_protocol_json_tests
    ../../tests/cpp/protocol_json_tests.cpp
  )
  target_compile_features(lanchat_protocol_json_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_protocol_json_tests PRIVATE include ../../protocol vendor)
  add_test(NAME protocol_json COMMAND lanchat_protocol_json_tests)

  # PresenceManager tests
  add_executable(lanchat_presence_manager_tests
    ../../tests/cpp/presence_manager_tests.cpp
    src/PresenceManager.cpp
  )
  target_compile_features(lanchat_presence_manager_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_presence_manager_tests PRIVATE include ../../protocol)
  add_test(NAME presence_manager COMMAND lanchat_presence_manager_tests)

endif()
