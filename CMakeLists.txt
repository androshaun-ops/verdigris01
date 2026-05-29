cmake_minimum_required(VERSION 3.16)
project(verdigris_edge_takehome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SANITIZERS "Enable ASan/UBSan (Clang/GCC)" OFF)
if(ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

add_library(edge_core
  src/common/fake_clock.cpp
  src/common/metrics.cpp
  src/probe/probe_fake.cpp
  src/sim/world.cpp
  src/sim/simulator.cpp
  src/conn_mgr/conn_mgr.cpp
  src/fw/firmware_node.cpp
)

target_include_directories(edge_core PUBLIC src)

add_executable(edge_tests tests/test_all.cpp)
target_link_libraries(edge_tests PRIVATE edge_core)

enable_testing()
add_test(NAME edge_tests COMMAND edge_tests)
