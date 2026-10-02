# circc

`circc` is a work-in-progress CIR-to-C translator based on LLVM/ClangIR
23.1.2. The repository currently contains only its build and command-line
shell; no translation is implemented yet.

## Toolchain

Pixi pins the bootstrap compiler and linker to Clang/LLD 23.1.2 on Windows
x86-64 and Linux x86-64. CMake selects the Pixi compilers and Python by
absolute path so host installations cannot leak into the build. CMake uses
Ninja, and the full LLVM build uses sccache.

```console
pixi install
```

Windows builds use the MSVC ABI and require the Visual Studio 2022 Build Tools
and a Windows SDK to be installed on the host.

## Fast shell build

This path builds the empty `circc` executable against conda-forge's packaged
LLVM. It validates the local build shell but does not provide CIR libraries.

```console
pixi run configure
pixi run check
```

The executable is written to `build/dev/bin/circc` (or `circc.exe`).

## CIR-enabled LLVM build

For CIR development, build `circc` as an LLVM external project. The initial
cache enables Clang, MLIR, LLD, CIR, assertions, the host target, and sccache.

```console
pixi run fetch-llvm
pixi run configure-llvm
pixi run build-llvm
```

The pinned LLVM checkout is stored in `.deps/llvm-project` and the build tree
in `build/llvm`. Both are intentionally ignored by Git. The resulting program
is in `build/llvm/bin`.

`fetch-llvm` is intentionally a one-time command. To reuse an existing LLVM
checkout, place the `llvmorg-23.1.2` tree at `.deps/llvm-project` before running
`configure-llvm`.

## Layout

```text
cmake/caches/LLVM.cmake  CIR-enabled LLVM configuration
tools/circc/circc.cpp    command-line entry point
CMakePresets.json        fast standalone development build
pixi.toml                pinned tools and common tasks
```
