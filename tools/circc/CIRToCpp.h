#ifndef CIRCC_TOOLS_CIRCC_CIRTOCPP_H
#define CIRCC_TOOLS_CIRCC_CIRTOCPP_H

#include "mlir/Support/LogicalResult.h"

namespace llvm {
class raw_ostream;
}

namespace mlir {
class ModuleOp;
}

namespace circc {

mlir::LogicalResult translateToCpp(mlir::ModuleOp sourceModule,
                                   llvm::raw_ostream &output);

} // namespace circc

#endif // CIRCC_TOOLS_CIRCC_CIRTOCPP_H
