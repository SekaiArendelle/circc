set(LLVM_LINK_COMPONENTS Support)

add_llvm_library(CIRCCToCpp STATIC BUILDTREE_ONLY
  CIRToCpp.cpp
)

target_compile_features(CIRCCToCpp PUBLIC cxx_std_23)
target_include_directories(CIRCCToCpp PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${LLVM_MAIN_SRC_DIR}/../clang/include
  ${LLVM_BINARY_DIR}/tools/clang/include
  ${LLVM_MAIN_SRC_DIR}/../mlir/include
  ${LLVM_BINARY_DIR}/tools/mlir/include
)

target_link_libraries(CIRCCToCpp
  PUBLIC
  clangBasic
  MLIRIR
  MLIRSupport
  PRIVATE
  MLIRCIR
  MLIREmitCDialect
  MLIRTargetCpp
)
