# circc

`circc` is a work-in-progress CIR-to-C translator based on LLVM/ClangIR
23.1.2. The initial translator lowers a deliberately small CIR subset through MLIR's
EmitC dialect and emits C++ source.

## Toolchain

Pixi pins the bootstrap compiler, linker, and LLVM host tools to 23.1.2 on Windows
x86-64 and Linux x86-64. CMake selects the Pixi compilers and Python by
absolute path so host installations cannot leak into the build. CMake uses
Ninja and sccache.

```console
pixi install
```

Windows builds use the MSVC ABI and require the Visual Studio 2022 Build Tools
and a Windows SDK to be installed on the host.

## Build

For CIR development, build `circc` as an LLVM external project. The initial
cache enables Clang, MLIR, CIR, assertions, the host target, and sccache. Pixi's
LLD is used as the linker, but the LLD subproject is not built.

```console
pixi run build
```

The pinned LLVM checkout is stored in `.deps/llvm-project` and the build tree
in `build`. Both are intentionally ignored by Git. The resulting program is in
`build/bin`.

The tasks form a dependency chain, so `pixi run build` fetches and configures
LLVM when needed. The `fetch-llvm` and `configure` stages can also be run
individually. To reuse an existing LLVM checkout, place the `llvmorg-23.1.2`
tree at `.deps/llvm-project` before running any of the tasks.

To build a Clang that can generate CIR, use the same LLVM checkout and build
tree. The existing configuration enables CIR and uses sccache for both C and
C++ compilation:

```console
pixi run build-clang
pixi run clang-cir -fclangir -emit-cir -S input.c -o input.cir
```

`clang-cir` runs the locally built compiler in the Pixi environment. The
packaged Pixi Clang remains the bootstrap compiler used to build the project.

## Command line

`circc` uses subcommands and long options. The initial `translate` interface
targets the C++ source emitted by MLIR's EmitC backend.
After building, run it through Pixi so its runtime libraries are available.

```console
pixi run circc help
pixi run circc help translate
pixi run circc version
pixi run circc translate --input=input.cir --output=output.cpp --language=c++17
```

`--language` accepts `c++11`, `c++14`, `c++17`, `c++20`, `c++23`, and
`c++26`. The default is `c++17`.
The selected standard controls C++ keyword validation during translation.

## Contributing

Development workflow and coding conventions are documented in
[AGENTS.md](./AGENTS.md). Human-facing contribution and commit-message policy
is in [CONTRIBUTING.md](./CONTRIBUTING.md).
