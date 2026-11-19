cmake_minimum_required(VERSION 3.16)
project(lanchat_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Test)
enable_testing()

# 仓库根 protocol/ 提供框架无关的 message_types.h（客户端直接 include）
set(LANCHAT_PROTOCOL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../protocol")

add_executable(lanchat_qt_test_frame_buffer
    tests/test_frame_buffer.cpp
    src/net/frame_buffer.cpp
)
target_include_directories(lanchat_qt_test_frame_buffer PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt_test_frame_buffer PRIVATE Qt6::Core)
add_test(NAME frame_buffer COMMAND lanchat_qt_test_frame_buffer)

add_executable(lanchat_qt_test_messages
    tests/test_messages.cpp
    src/net/frame_buffer.cpp
    src/protocol/messages.cpp
)
target_include_directories(lanchat_qt_test_messages PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt_test_messages PRIVATE Qt6::Core)
add_test(NAME messages COMMAND lanchat_qt_test_messages)

add_executable(lanchat_qt_test_session
    tests/test_session.cpp
    src/net/frame_buffer.cpp
    src/net/session.cpp
    src/protocol/messages.cpp
)
target_include_directories(lanchat_qt_test_session PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt_test_session PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME session COMMAND lanchat_qt_test_session)

add_executable(lanchat_qt_test_store
    tests/test_store.cpp
    src/model/store.cpp
)
target_include_directories(lanchat_qt_test_store PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt_test_store PRIVATE Qt6::Core)
add_test(NAME store COMMAND lanchat_qt_test_store)

add_executable(lanchat_qt_test_devmode
    tests/test_devmode.cpp
    src/app/devmode.cpp
)
target_include_directories(lanchat_qt_test_devmode PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt_test_devmode PRIVATE Qt6::Core Qt6::Test)
add_test(NAME devmode COMMAND lanchat_qt_test_devmode)

# 主客户端目标（WIN32 = 无控制台窗口）
add_executable(lanchat_qt WIN32
    src/main.cpp
    src/net/frame_buffer.cpp
    src/net/session.cpp
    src/protocol/messages.cpp
    src/model/store.cpp
    src/app/devmode.cpp
    src/app/server_process.cpp
    src/ui/theme.cpp
    src/ui/login_view.cpp
    src/ui/register_view.cpp
    src/ui/chat_view.cpp
    src/ui/contacts_panel.cpp
    src/ui/search_panel.cpp
    src/ui/main_window.cpp
)
target_include_directories(lanchat_qt PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)