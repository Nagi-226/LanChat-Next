cmake_minimum_required(VERSION 3.28)
project(LanChatNext LANGUAGES C CXX)

option(LANCHAT_BUILD_SERVER_NEXT "Build the v1.1.0 C++ server skeleton" ON)
option(LANCHAT_BUILD_TESTS "Build LanChat-Next C++ smoke tests" ON)

if(LANCHAT_BUILD_TESTS)
  enable_testing()
endif()

if(LANCHAT_BUILD_SERVER_NEXT)
  add_subdirectory(src/server)
endif()
