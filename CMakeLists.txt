cmake_minimum_required(VERSION 3.28)
project(LanChatNext LANGUAGES CXX)

option(LANCHAT_BUILD_SERVER_NEXT "Build the v1.1.0 C++ server skeleton" ON)

if(LANCHAT_BUILD_SERVER_NEXT)
  add_subdirectory(src/server)
endif()
