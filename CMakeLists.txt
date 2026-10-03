cmake_minimum_required(VERSION 3.31)

project(CIRCC VERSION 0.1.0 LANGUAGES C CXX)

if(NOT COMMAND add_llvm_executable)
  message(FATAL_ERROR "circc must be configured as an LLVM external project")
endif()

set(LLVM_LINK_COMPONENTS Support)

add_llvm_executable(circc
  tools/circc/CIRToCpp.cpp
  tools/circc/circc.cpp
)

target_compile_features(circc PRIVATE cxx_std_23)
target_compile_definitions(circc PRIVATE CIRCC_VERSION="${PROJECT_VERSION}")
target_include_directories(circc PRIVATE
  ${LLVM_MAIN_SRC_DIR}/../clang/include
  ${LLVM_BINARY_DIR}/tools/clang/include
  ${LLVM_MAIN_SRC_DIR}/../mlir/include
  ${LLVM_BINARY_DIR}/tools/mlir/include
)

target_link_libraries(circc PRIVATE
  MLIRCIR
  MLIREmitCDialect
  MLIRIR
  MLIRParser
  MLIRSupport
  MLIRTargetCpp
)

if(LLVM_INCLUDE_TESTS)
  add_subdirectory(test)
endif()

install(TARGETS circc
  RUNTIME DESTINATION bin
)
