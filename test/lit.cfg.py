import os

import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst


config.name = "circc"
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)
config.suffixes = [".test", ".cir"]
config.excludes = ["Inputs"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.circc_test_exec_root

llvm_config.with_environment(
    "PATH", os.path.dirname(config.circc_cxx_compiler), append_path=True
)

llvm_config.add_tool_substitutions(
    [
        ToolSubst("FileCheck"),
        ToolSubst("not"),
        ToolSubst("%circc", command=os.path.join(config.circc_tools_dir, "circc")),
    ],
    [config.llvm_tools_dir],
)

config.substitutions.append(("%clangxx", config.circc_cxx_compiler))
