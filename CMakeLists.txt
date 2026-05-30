cmake_minimum_required(VERSION 3.28)
project(LanChatNext LANGUAGES C CXX)

option(LANCHAT_BUILD_SERVER_NEXT "Build the v1.1.0 C++ server skeleton" ON)
option(LANCHAT_BUILD_TESTS "Build LanChat-Next C++ smoke tests" ON)
option(LANCHAT_ENABLE_ASAN "Enable AddressSanitizer where supported" OFF)
option(LANCHAT_WARNINGS_AS_ERRORS "Treat LanChat C++ warnings as errors" OFF)

function(lanchat_harden_target target_name)
  if(MSVC)
    target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
    target_compile_options(${target_name} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:/WX->
      $<$<COMPILE_LANGUAGE:C>:/wd4244>
      $<$<COMPILE_LANGUAGE:C>:/wd4996>
      $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
      $<$<COMPILE_LANGUAGE:CXX>:/permissive->
      $<$<COMPILE_LANGUAGE:CXX>:/W4>
      $<$<COMPILE_LANGUAGE:CXX>:/wd4996>
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:${LANCHAT_WARNINGS_AS_ERRORS}>>:/WX>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/O2>
    )
    target_link_options(${target_name} PRIVATE $<$<CONFIG:Release>:/LTCG>)
  else()
    target_compile_options(${target_name} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:-Wall>
      $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
      $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:${LANCHAT_WARNINGS_AS_ERRORS}>>:-Werror>
      $<$<BOOL:${LANCHAT_ENABLE_ASAN}>:-fsanitize=address>
    )
    target_link_options(${target_name} PRIVATE $<$<BOOL:${LANCHAT_ENABLE_ASAN}>:-fsanitize=address>)
  endif()
endfunction()

function(lanchat_enable_test_asserts target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release>:/UNDEBUG>)
  else()
    target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
  endif()
endfunction()

if(LANCHAT_BUILD_TESTS)
  enable_testing()
endif()

if(LANCHAT_BUILD_SERVER_NEXT)
  add_subdirectory(src/server)
endif()
