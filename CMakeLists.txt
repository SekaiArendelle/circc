cmake_minimum_required(VERSION 3.31)

project(CIRCC VERSION 0.1.0 LANGUAGES C CXX)

if(NOT COMMAND add_llvm_executable)
  message(FATAL_ERROR "circc must be configured as an LLVM external project")
endif()

set(LLVM_LINK_COMPONENTS Support)

add_llvm_executable(circc
  tools/circc/circc.cpp
)

target_compile_features(circc PRIVATE cxx_std_23)

install(TARGETS circc
  RUNTIME DESTINATION bin
)
