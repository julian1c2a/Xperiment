cmake_minimum_required(VERSION 3.10)
project(Xperiment LANGUAGES CXX)

# Request C++14 and strict standard conformance
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++14, not gnu++14

add_executable(Xperiment Xperiment.cpp)

# Compiler-specific strict flags
if(MSVC)
  target_compile_options(Xperiment PRIVATE "/permissive-" "/std:c++14" "/W4" "/EHsc")
else()
  target_compile_options(Xperiment PRIVATE -std=c++14 -pedantic -Wall -Wextra -Werror)
endif()

# --- Test executable for expected_cpp14.hpp ---
add_executable(run_tests test_expected.cpp)

# Aplicar las mismas flags estrictas al ejecutable de test
if(MSVC)
  target_compile_options(run_tests PRIVATE "/permissive-" "/std:c++14" "/W4" "/EHsc")
else()
  target_compile_options(run_tests PRIVATE -std=c++14 -pedantic -Wall -Wextra -Werror)
endif()

# Recordando tu preferencia, colocar el ejecutable del test en 'build/build_tests'
set_target_properties(run_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build_tests"
)
