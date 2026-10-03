#include "CIRToCpp.h"

#include "clang/CIR/Dialect/IR/CIRDialect.h"

#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Support/FileUtilities.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

namespace {

enum class OutputLanguage { Cpp };

llvm::cl::SubCommand TranslateCommand("translate",
                                      "Translate CIR to source code");

llvm::cl::OptionCategory TranslateCategory("circc translate options");

llvm::cl::opt<std::string>
    InputFilename("input", llvm::cl::desc("Read CIR from <filename>"),
                  llvm::cl::value_desc("filename"), llvm::cl::init("-"),
                  llvm::cl::cat(TranslateCategory),
                  llvm::cl::sub(TranslateCommand));

llvm::cl::opt<std::string>
    OutputFilename("output", llvm::cl::desc("Write source code to <filename>"),
                   llvm::cl::value_desc("filename"), llvm::cl::init("-"),
                   llvm::cl::cat(TranslateCategory),
                   llvm::cl::sub(TranslateCommand));

llvm::cl::opt<OutputLanguage> Language(
    "language", llvm::cl::desc("Select the output language"),
    llvm::cl::values(clEnumValN(OutputLanguage::Cpp, "cpp", "C++ source code")),
    llvm::cl::init(OutputLanguage::Cpp), llvm::cl::cat(TranslateCategory),
    llvm::cl::sub(TranslateCommand));

void emitError(const llvm::Twine &message) {
  llvm::WithColor::error() << message << '\n';
}

void emitHint(const llvm::Twine &message) {
  llvm::errs() << "hint: " << message << '\n';
}

void printTopLevelHelp() {
  llvm::outs() << "OVERVIEW: CIR to source-code translator\n\n"
                  "USAGE: circc <command> [options]\n\n"
                  "COMMANDS:\n"
                  "  translate  Translate CIR to source code\n"
                  "  help       Display help for circc or a command\n"
                  "  version    Display the circc version\n\n"
                  "Run 'circc help <command>' for command-specific help.\n";
}

void printTranslateHelp() {
  llvm::outs() << "OVERVIEW: Translate CIR to source code\n\n"
                  "USAGE: circc translate [options]\n\n"
                  "OPTIONS:\n"
                  "  --input=<filename>     Read CIR from <filename> "
                  "(default: stdin)\n"
                  "  --output=<filename>    Write source code to <filename> "
                  "(default: stdout)\n"
                  "  --language=<language>  Select the output language "
                  "(currently: cpp)\n";
}

int printHelp(llvm::StringRef topic) {
  if (topic.empty()) {
    printTopLevelHelp();
    return 0;
  }
  if (topic == "translate") {
    printTranslateHelp();
    return 0;
  }
  if (topic == "help") {
    llvm::outs() << "USAGE: circc help [command]\n";
    return 0;
  }
  if (topic == "version") {
    llvm::outs() << "USAGE: circc version\n";
    return 0;
  }

  emitError(llvm::Twine("unknown help topic '") + topic + "'");
  emitHint("run 'circc help' to list available commands");
  return 1;
}

int runTranslate() {
  mlir::DialectRegistry registry;
  registry.insert<cir::CIRDialect, mlir::emitc::EmitCDialect>();
  mlir::MLIRContext context(registry);
  context.loadDialect<cir::CIRDialect, mlir::emitc::EmitCDialect>();

  std::string errorMessage;
  auto input = mlir::openInputFile(InputFilename, &errorMessage);
  if (!input) {
    emitError(errorMessage);
    return 1;
  }

  llvm::SourceMgr sourceManager;
  sourceManager.AddNewSourceBuffer(std::move(input), llvm::SMLoc());
  mlir::OwningOpRef<mlir::ModuleOp> sourceModule =
      mlir::parseSourceFile<mlir::ModuleOp>(sourceManager, &context);
  if (!sourceModule)
    return 1;

  std::string translated;
  llvm::raw_string_ostream translatedStream(translated);
  if (mlir::failed(circc::translateToCpp(*sourceModule, translatedStream)))
    return 1;
  translatedStream.flush();

  auto output = mlir::openOutputFile(OutputFilename, &errorMessage);
  if (!output) {
    emitError(errorMessage);
    return 1;
  }
  output->os() << translated;
  output->keep();
  return 0;
}

bool markOptionSeen(llvm::StringRef option, bool &seen) {
  if (!seen) {
    seen = true;
    return true;
  }
  emitError(llvm::Twine("option '") + option +
            "' may not be specified more than once");
  return false;
}

bool validateTranslateArguments(int argc, char **argv) {
  bool seenInput = false;
  bool seenOutput = false;
  bool seenLanguage = false;

  for (int index = 2; index < argc; ++index) {
    llvm::StringRef argument = argv[index];
    bool *seen = nullptr;
    llvm::StringRef option;
    if (argument == "--input" || argument.starts_with("--input=")) {
      option = "--input";
      seen = &seenInput;
    } else if (argument == "--output" || argument.starts_with("--output=")) {
      option = "--output";
      seen = &seenOutput;
    } else if (argument == "--language" ||
               argument.starts_with("--language=")) {
      option = "--language";
      seen = &seenLanguage;
    }

    if (seen != nullptr) {
      if (!markOptionSeen(option, *seen))
        return false;
      llvm::StringRef value;
      if (argument.contains('=')) {
        value = argument.split('=').second;
      } else if (++index < argc) {
        value = argv[index];
      } else {
        emitError(llvm::Twine("option '") + option + "' requires a value");
        return false;
      }
      if (option == "--language" && value != "cpp") {
        emitError(llvm::Twine("invalid value '") + value +
                  "' for option '--language'");
        emitHint("supported values: cpp");
        return false;
      }
      continue;
    }

    emitError(llvm::Twine("unknown option '") + argument +
              "' for command 'translate'");
    emitHint("run 'circc help translate' to list available options");
    return false;
  }
  return true;
}

} // namespace

int main(int argc, char **argv) {
  llvm::InitLLVM initLLVM(argc, argv);

  if (argc == 1) {
    emitError("no command specified");
    printTopLevelHelp();
    return 1;
  }

  llvm::StringRef command = argv[1];
  if (command == "help" || command == "--help") {
    if (command == "--help" && argc != 2) {
      emitError("--help does not accept arguments");
      return 1;
    }
    if (argc > 3) {
      emitError("help accepts at most one command");
      return 1;
    }
    return printHelp(argc == 3 ? argv[2] : "");
  }

  if (command == "version" || command == "--version") {
    if (argc != 2) {
      emitError(llvm::Twine(command) + " does not accept arguments");
      return 1;
    }
    llvm::outs() << "circc version " CIRCC_VERSION "\n";
    return 0;
  }

  if (command != "translate") {
    emitError(llvm::Twine("unknown command '") + command + "'");
    emitHint("run 'circc help' to list available commands");
    return 1;
  }

  for (int index = 2; index < argc; ++index) {
    if (llvm::StringRef(argv[index]) != "--help")
      continue;
    if (argc != 3) {
      emitError("--help does not accept other options");
      return 1;
    }
    return printHelp("translate");
  }

  if (!validateTranslateArguments(argc, argv))
    return 1;

  if (!llvm::cl::ParseCommandLineOptions(
          argc, argv, "CIR to source-code translator\n", &llvm::errs(), nullptr,
          nullptr, /*LongOptionsUseDoubleDash=*/true))
    return 1;

  if (TranslateCommand)
    return runTranslate();

  llvm_unreachable("the translate subcommand was not selected");
}
