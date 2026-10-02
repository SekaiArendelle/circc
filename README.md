# circc

`circc` is a work-in-progress CIR-to-C translator based on LLVM/ClangIR
23.1.2. The repository currently contains only its build and command-line
shell; no translation is implemented yet.

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

## Layout

```text
cmake/caches/LLVM.cmake  CIR-enabled LLVM configuration
tools/circc/circc.cpp    command-line entry point
pixi.toml                pinned tools and common tasks
```

## Contributing

Development workflow and coding conventions are documented in
[AGENTS.md](./AGENTS.md). Human-facing contribution and commit-message policy
is in [CONTRIBUTING.md](./CONTRIBUTING.md).
