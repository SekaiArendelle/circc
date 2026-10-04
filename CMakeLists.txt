cmake_minimum_required(VERSION 3.31)

project(CIRCC VERSION 0.1.0 LANGUAGES C CXX)

if(NOT COMMAND add_llvm_executable)
  message(FATAL_ERROR "circc must be configured as an LLVM external project")
endif()

add_subdirectory(lib)
add_subdirectory(tools)

if(LLVM_INCLUDE_TESTS)
  add_subdirectory(test)
endif()
