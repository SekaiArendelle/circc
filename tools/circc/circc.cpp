#include "llvm/Support/CommandLine.h"

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv, "CIR to C translator\n");
  return 0;
}
