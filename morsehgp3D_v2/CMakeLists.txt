cmake_minimum_required(VERSION 3.22)
project(morsehgp3d_v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(mhgp
  src/catalogue.cpp
  src/forest.cpp
  src/run.cpp
)
target_include_directories(mhgp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(mhgp PRIVATE -Wall -Wextra -Wpedantic)
find_package(Threads REQUIRED)
target_link_libraries(mhgp PUBLIC Threads::Threads)

add_executable(mhgp_cli tools/mhgp_cli.cpp)
target_link_libraries(mhgp_cli PRIVATE mhgp)

enable_testing()
add_executable(mhgp_tests tests/tests.cpp)
target_link_libraries(mhgp_tests PRIVATE mhgp)
add_test(NAME mhgp_tests COMMAND mhgp_tests)

add_executable(mhgp_oracle tests/oracle_main.cpp)
target_link_libraries(mhgp_oracle PRIVATE mhgp)
add_test(NAME mhgp_oracle COMMAND mhgp_oracle)

add_executable(mhgp_regressions tests/regressions.cpp)
target_link_libraries(mhgp_regressions PRIVATE mhgp)
add_test(NAME mhgp_regressions COMMAND mhgp_regressions)

add_executable(mhgp_oracle2 tests/oracle2.cpp)
target_link_libraries(mhgp_oracle2 PRIVATE mhgp)
add_test(NAME mhgp_oracle2 COMMAND mhgp_oracle2)
