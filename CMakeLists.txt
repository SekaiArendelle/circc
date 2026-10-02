cmake_minimum_required(VERSION 3.31)

if(POLICY CMP0219)
  cmake_policy(SET CMP0219 NEW)
endif()

project(CIRCC VERSION 0.1.0 LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(CIRCC_STANDALONE ON)
else()
  set(CIRCC_STANDALONE OFF)
endif()

# When circc is configured as an LLVM external project, LLVM has already made
# its CMake helpers available. The standalone path is useful for validating the
# project shell against Pixi's packaged LLVM without building ClangIR first.
if(NOT COMMAND add_llvm_executable)
  find_package(LLVM 23.1.2 EXACT REQUIRED CONFIG)
  list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
  include(AddLLVM)
  include(HandleLLVMOptions)
endif()

set(LLVM_LINK_COMPONENTS Support)

add_llvm_executable(circc
  tools/circc/circc.cpp
)

target_compile_features(circc PRIVATE cxx_std_23)

install(TARGETS circc
  RUNTIME DESTINATION bin
)

if(CIRCC_STANDALONE)
  enable_testing()
  add_test(NAME circc-help COMMAND $<TARGET_FILE:circc> --help)
endif()
