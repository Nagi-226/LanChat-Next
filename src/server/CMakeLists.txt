set(LANCHAT_VENDOR_C_SOURCES
  vendor/openwall_bcrypt/crypt_blowfish.c
  vendor/openwall_bcrypt/crypt_gensalt.c
  vendor/openwall_bcrypt/wrapper.c
  vendor/sqlite/sqlite3.c
)

if(MSVC)
  set_source_files_properties(${LANCHAT_VENDOR_C_SOURCES} PROPERTIES
    COMPILE_OPTIONS "/WX-;/wd4244;/wd4996"
  )
endif()

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
  src/db/FriendRepository.cpp
  src/db/LegacyMigrator.cpp
  vendor/openwall_bcrypt/crypt_blowfish.c
  vendor/openwall_bcrypt/crypt_gensalt.c
  vendor/openwall_bcrypt/wrapper.c
  vendor/sqlite/sqlite3.c
)

target_compile_definitions(lanchat_server_next PRIVATE
  ASIO_STANDALONE
  NOMINMAX
  SQLITE_ENABLE_FTS5
  SQLITE_ENABLE_JSON1
)
target_compile_features(lanchat_server_next PRIVATE cxx_std_17)
target_include_directories(lanchat_server_next PRIVATE include ../../protocol vendor/asio/include vendor vendor/sqlite)
lanchat_harden_target(lanchat_server_next)

if(WIN32)
  target_link_libraries(lanchat_server_next PRIVATE ws2_32)
endif()

message(STATUS "LanChat-Next server: using standalone Asio transport with vendored mini-json/mini-spdlog")

if(LANCHAT_BUILD_TESTS)
  # Existing test
  add_executable(lanchat_frame_codec_tests
    ../../tests/cpp/frame_codec_tests.cpp
    src/FrameCodec.cpp
  )
  target_compile_features(lanchat_frame_codec_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_frame_codec_tests PRIVATE include ../../protocol)
  lanchat_harden_target(lanchat_frame_codec_tests)
  lanchat_enable_test_asserts(lanchat_frame_codec_tests)
  add_test(NAME frame_codec COMMAND lanchat_frame_codec_tests)

  # ProtocolJson inline tests (no link deps)
  add_executable(lanchat_protocol_json_tests
    ../../tests/cpp/protocol_json_tests.cpp
  )
  target_compile_features(lanchat_protocol_json_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_protocol_json_tests PRIVATE include ../../protocol vendor)
  lanchat_harden_target(lanchat_protocol_json_tests)
  lanchat_enable_test_asserts(lanchat_protocol_json_tests)
  add_test(NAME protocol_json COMMAND lanchat_protocol_json_tests)

  add_executable(lanchat_asio_transport_tests
    ../../tests/cpp/asio_transport_tests.cpp
  )
  target_compile_features(lanchat_asio_transport_tests PRIVATE cxx_std_17)
  target_compile_definitions(lanchat_asio_transport_tests PRIVATE ASIO_STANDALONE NOMINMAX)
  target_include_directories(lanchat_asio_transport_tests PRIVATE include ../../protocol vendor/asio/include vendor)
  lanchat_harden_target(lanchat_asio_transport_tests)
  lanchat_enable_test_asserts(lanchat_asio_transport_tests)
  add_test(NAME asio_transport COMMAND lanchat_asio_transport_tests)

  # PresenceManager tests
  add_executable(lanchat_presence_manager_tests
    ../../tests/cpp/presence_manager_tests.cpp
    src/PresenceManager.cpp
  )
  target_compile_features(lanchat_presence_manager_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_presence_manager_tests PRIVATE include ../../protocol)
  lanchat_harden_target(lanchat_presence_manager_tests)
  lanchat_enable_test_asserts(lanchat_presence_manager_tests)
  add_test(NAME presence_manager COMMAND lanchat_presence_manager_tests)

  add_executable(lanchat_database_tests
    ../../tests/cpp/database_tests.cpp
    src/db/Database.cpp
    src/db/UserRepository.cpp
    src/ServerLogger.cpp
    vendor/openwall_bcrypt/crypt_blowfish.c
    vendor/openwall_bcrypt/crypt_gensalt.c
    vendor/openwall_bcrypt/wrapper.c
    vendor/sqlite/sqlite3.c
  )
  target_compile_definitions(lanchat_database_tests PRIVATE NOMINMAX SQLITE_ENABLE_FTS5 SQLITE_ENABLE_JSON1)
  target_compile_features(lanchat_database_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_database_tests PRIVATE include ../../protocol vendor vendor/sqlite)
  lanchat_harden_target(lanchat_database_tests)
  lanchat_enable_test_asserts(lanchat_database_tests)
  add_test(NAME database COMMAND lanchat_database_tests)

  add_executable(lanchat_friend_repository_tests
    ../../tests/cpp/friend_repository_tests.cpp
    src/db/Database.cpp
    src/db/UserRepository.cpp
    src/db/FriendRepository.cpp
    src/ServerLogger.cpp
    vendor/openwall_bcrypt/crypt_blowfish.c
    vendor/openwall_bcrypt/crypt_gensalt.c
    vendor/openwall_bcrypt/wrapper.c
    vendor/sqlite/sqlite3.c
  )
  target_compile_definitions(lanchat_friend_repository_tests PRIVATE NOMINMAX SQLITE_ENABLE_FTS5 SQLITE_ENABLE_JSON1)
  target_compile_features(lanchat_friend_repository_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_friend_repository_tests PRIVATE include ../../protocol vendor vendor/sqlite)
  lanchat_harden_target(lanchat_friend_repository_tests)
  lanchat_enable_test_asserts(lanchat_friend_repository_tests)
  add_test(NAME friend_repository COMMAND lanchat_friend_repository_tests)

  add_executable(lanchat_message_repository_tests
    ../../tests/cpp/message_repository_tests.cpp
    src/db/Database.cpp
    src/db/MessageRepository.cpp
    src/ServerLogger.cpp
    vendor/sqlite/sqlite3.c
  )
  target_compile_definitions(lanchat_message_repository_tests PRIVATE NOMINMAX SQLITE_ENABLE_FTS5 SQLITE_ENABLE_JSON1)
  target_compile_features(lanchat_message_repository_tests PRIVATE cxx_std_17)
  target_include_directories(lanchat_message_repository_tests PRIVATE include ../../protocol vendor vendor/sqlite)
  lanchat_harden_target(lanchat_message_repository_tests)
  lanchat_enable_test_asserts(lanchat_message_repository_tests)
  add_test(NAME message_repository COMMAND lanchat_message_repository_tests)

endif()
