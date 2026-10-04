set(LLVM_LINK_COMPONENTS Support)

add_llvm_executable(circc
  circc.cpp
)

target_compile_definitions(circc PRIVATE CIRCC_VERSION="${PROJECT_VERSION}")
target_link_libraries(circc PRIVATE
  CIRCCToCpp
  MLIRCIR
  MLIREmitCDialect
  MLIRParser
)

install(TARGETS circc
  RUNTIME DESTINATION bin
)
