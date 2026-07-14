add_executable(morsehgp3d_exact_types_tests test_exact_types.cpp)
target_link_libraries(morsehgp3d_exact_types_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_exact_types_dump exact_types_dump.cpp)
target_link_libraries(morsehgp3d_exact_types_dump PRIVATE morsehgp3d::exact)

if(MSVC)
  target_compile_options(morsehgp3d_exact_types_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_exact_types_dump PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_exact_types_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_exact_types_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_test(NAME morsehgp3d.exact_types COMMAND morsehgp3d_exact_types_tests)

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
add_test(
  NAME morsehgp3d.exact_contract_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_exact_types.py"
    "$<TARGET_FILE:morsehgp3d_exact_types_dump>"
)
