cmake_minimum_required(VERSION 3.16)
project(ai-autoshell LANGUAGES CXX)

# Options
option(BUILD_POSIX "Build POSIX (macOS/Linux) shell" ON)
option(BUILD_WINDOWS "Build Windows shell (experimental stub)" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer in Debug builds (non-MSVC)" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Sanitizers (Debug only, non-MSVC)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE MATCHES "Debug|"
   AND NOT MSVC)
  add_link_options(-fsanitize=address,undefined)
  add_compile_options(-fsanitize=address,undefined)
endif()

# ----------------------------------------------------------------------------
# POSIX target (macOS/Linux)
# ----------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
  set(AI_AUTOSHELL_POSIX TRUE)
elseif(APPLE)
  set(AI_AUTOSHELL_POSIX TRUE)
endif()

if(BUILD_POSIX AND AI_AUTOSHELL_POSIX)
  add_executable(ai-autoshell
    src/main.cpp
    src/lex/lexer.cpp
  src/parse/parser.cpp
  src/expand/expand.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
  )
  target_include_directories(ai-autoshell PRIVATE src)
  target_include_directories(ai-autoshell PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_definitions(ai-autoshell PRIVATE AI_AUTOSHELL_POSIX=1)
  # Script runner (.ash)
  add_executable(ai-autoshell-script
    src/main_script.cpp
    src/lex/lexer.cpp
    src/parse/parser.cpp
    src/expand/expand.cpp
    src/exec/path.cpp
    src/exec/redir.cpp
    src/exec/builtins.cpp
    src/exec/job.cpp
    src/exec/executor_posix.cpp
  src/line/line_editor.cpp
  )
  target_include_directories(ai-autoshell-script PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_definitions(ai-autoshell-script PRIVATE AI_AUTOSHELL_POSIX=1)
  install(TARGETS ai-autoshell-script RUNTIME DESTINATION bin)
  # On some libc++, libstdc++ setups filesystem is implicit with C++20.
  # Nothing special to link here for the current code.

  # Install rule
  install(TARGETS ai-autoshell RUNTIME DESTINATION bin)
endif()

# ----------------------------------------------------------------------------
# Tests (GoogleTest)
# ----------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_lexer
  tests/test_lexer.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/expand/expand.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_lexer PRIVATE GTest::gtest_main)
target_include_directories(test_lexer PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_parser
  tests/test_parser.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/expand/expand.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_parser PRIVATE GTest::gtest_main)
target_include_directories(test_parser PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(GoogleTest)
gtest_discover_tests(test_lexer)
gtest_discover_tests(test_parser)

add_executable(test_expand
  tests/test_expand.cpp
  src/expand/expand.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_expand PRIVATE GTest::gtest_main)
target_include_directories(test_expand PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(test_expand)

add_executable(test_executor
  tests/test_executor.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/expand/expand.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_executor PRIVATE GTest::gtest_main)
target_include_directories(test_executor PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(test_executor)

add_executable(test_jobs
  tests/test_jobs.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/expand/expand.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_jobs PRIVATE GTest::gtest_main)
target_include_directories(test_jobs PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(test_jobs)

add_executable(test_jobs_advanced
  tests/test_jobs_advanced.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/expand/expand.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_jobs_advanced PRIVATE GTest::gtest_main)
target_include_directories(test_jobs_advanced PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(test_jobs_advanced)

add_executable(test_glob
  tests/test_glob.cpp
  src/expand/expand.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_glob PRIVATE GTest::gtest_main)
target_include_directories(test_glob PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(test_glob)

add_executable(test_subshell
  tests/test_subshell.cpp
  src/expand/expand.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_subshell PRIVATE GTest::gtest_main)
target_include_directories(test_subshell PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(test_subshell)

add_executable(test_command_subst
  tests/test_command_subst.cpp
  src/expand/expand.cpp
  src/lex/lexer.cpp
  src/parse/parser.cpp
  src/exec/path.cpp
  src/exec/redir.cpp
  src/exec/builtins.cpp
  src/exec/job.cpp
  src/exec/executor_posix.cpp
  src/line/line_editor.cpp
)
target_link_libraries(test_command_subst PRIVATE GTest::gtest_main)
target_include_directories(test_command_subst PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)
gtest_discover_tests(test_command_subst)

# ----------------------------------------------------------------------------
# Windows target (placeholder)
# ----------------------------------------------------------------------------
if(WIN32 AND BUILD_WINDOWS)
  add_executable(ai-autoshell-win
    # TODO: add a Windows-specific entry point (e.g., src/main_win.cpp)
  )
  target_compile_definitions(ai-autoshell-win PRIVATE AI_AUTOSHELL_WINDOWS=1 _WIN32_WINNT=0x0A00)
  target_link_libraries(ai-autoshell-win PRIVATE ws2_32)
  install(TARGETS ai-autoshell-win RUNTIME DESTINATION bin)
endif()

# ----------------------------------------------------------------------------
# Helpful messages
# ----------------------------------------------------------------------------
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
if(TARGET ai-autoshell)
  message(STATUS "Building POSIX target: ai-autoshell")
endif()
if(TARGET ai-autoshell-win)
  message(STATUS "Building Windows target: ai-autoshell-win")
endif()