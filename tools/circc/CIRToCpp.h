#ifndef CIRCC_TOOLS_CIRCC_CIRTOCPP_H
#define CIRCC_TOOLS_CIRCC_CIRTOCPP_H

#include "clang/Basic/LangStandard.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"

#include "llvm/Support/raw_ostream.h"

namespace circc {

mlir::LogicalResult translateToCpp(mlir::ModuleOp sourceModule,
                                   llvm::raw_ostream &output,
                                   clang::LangStandard::Kind standard);

} // namespace circc

#endif // CIRCC_TOOLS_CIRCC_CIRTOCPP_H
