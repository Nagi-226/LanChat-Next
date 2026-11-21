cmake_minimum_required(VERSION 3.22)
project(lanchat_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

# 记住账户：DPAPI 加解密往返 + 取消清除（Windows 链 crypt32）
add_executable(lanchat_qt_test_credentials
    tests/test_credentials.cpp
    src/app/credentials.cpp
)
target_include_directories(lanchat_qt_test_credentials PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt_test_credentials PRIVATE Qt6::Core Qt6::Test)
if(WIN32)
    target_link_libraries(lanchat_qt_test_credentials PRIVATE crypt32)
endif()
add_test(NAME credentials COMMAND lanchat_qt_test_credentials)

# ============================================================================
# 测试运行环境：把 Qt 的 bin 目录加进 PATH
#
# 不加的话，测试 exe 运行时找不到 Qt6Core.dll，ctest 报 0xc0000135
# （STATUS_DLL_NOT_FOUND）——表现为"5 个测试里 2 个红"，而代码其实没问题。
# 新 clone 或 CI 第一次跑必踩，容易被误判成"代码坏了"。
#
# 为什么用 ENVIRONMENT_MODIFICATION 而不是 ENVIRONMENT：
#   ENVIRONMENT 的值是"分号分隔的 VAR=value 列表"，而 Windows 的 PATH 本身含 ';'，
#   直接写 "PATH=<dir>;$ENV{PATH}" 会被 CMake 当成多个列表项切坏。
#   path_list_prepend 由 CMake 自己处理分隔符，是官方推荐的写法（需 3.22+）。
# ============================================================================
# Qt6_DIR 形如 <prefix>/lib/cmake/Qt6 → bin 在 <prefix>/bin
get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
set(_qt_bin_dir "${_qt_prefix}/bin")
if(NOT EXISTS "${_qt_bin_dir}")
    # 兜底：从 Qt6::Core 的导入位置反推（某些包管理器布局不同）
    get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION)
    if(NOT _qt_core_loc)
        get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION_RELEASE)
    endif()
    if(_qt_core_loc)
        get_filename_component(_qt_bin_dir "${_qt_core_loc}" DIRECTORY)
    endif()
endif()

if(EXISTS "${_qt_bin_dir}")
    set_tests_properties(frame_buffer messages session store devmode credentials PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir}")
    message(STATUS "ctest: 已把 Qt bin 目录前置到 PATH -> ${_qt_bin_dir}")
else()
    message(WARNING
        "ctest: 未能定位 Qt6 的 bin 目录（Qt6_DIR=${Qt6_DIR}）。"
        "测试可能因找不到 Qt DLL 失败，错误码 0xc0000135。")
endif()

# 主客户端目标（WIN32 = 无控制台窗口）
add_executable(lanchat_qt WIN32
    src/main.cpp
    src/net/frame_buffer.cpp
    src/net/session.cpp
    src/protocol/messages.cpp
    src/model/store.cpp
    src/app/devmode.cpp
    src/app/credentials.cpp
    src/app/server_process.cpp
    src/ui/theme.cpp
    src/ui/avatar.cpp
    src/ui/login_view.cpp
    src/ui/register_view.cpp
    src/ui/chat_view.cpp
    src/ui/contacts_panel.cpp
    src/ui/members_panel.cpp
    src/ui/main_window.cpp
    resources/resources.qrc
)
target_include_directories(lanchat_qt PRIVATE src ${LANCHAT_PROTOCOL_DIR})
target_link_libraries(lanchat_qt PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
if(WIN32)
    # DPAPI（CryptProtectData）在 crypt32：记住账户的密码加密靠它
    target_link_libraries(lanchat_qt PRIVATE crypt32)
endif()