# AGENTS.md — Developer Guide for `circc`

This file is the entry point for AI coding agents. It describes the repository
workflow, build commands, validation expectations, and coding conventions.
Human-facing contribution policy lives in [CONTRIBUTING.md](./CONTRIBUTING.md).

## Mandatory rules

- Do not run Git write operations without explicit human instruction. This
  includes `git add`, `git commit`, `git push`, rebases, tag creation, and
  opening or modifying pull requests and issues.
- Treat the Git index as human-owned shared state. Do not unstage or rewrite
  staged changes that you did not create. Report unexpected staged changes in
  the final summary instead of discarding them.
- Keep changes focused. Preserve unrelated work in a dirty worktree and do not
  reformat files or lines outside the requested change.
- After changing code or build configuration, run the narrowest relevant
  checks and then the normal `circc` build when practical.
- If a request relies on a false premise or would produce an incorrect design,
  stop and explain the problem with concrete repository evidence before
  proposing a corrected direction.

## Project overview

`circc` is a C++23 CIR-to-C translator built as an external project inside a
pinned LLVM/ClangIR 23.1.2 source tree. There is intentionally no standalone
build because packaged LLVM distributions do not include CIR.

| Path | Purpose |
|------|---------|
| `tools/circc/` | Command-line entry point |
| `include/circc/` | Public translation interfaces |
| `lib/Target/Cpp/` | CIR-to-C++ translation library |
| `cmake/caches/LLVM.cmake` | CIR-enabled LLVM initial cache |
| `cmake/toolchains/Clang.cmake` | Pixi Clang toolchain selection |
| `pixi.toml` / `pixi.lock` | Pinned tools, platforms, and tasks |
| `.deps/llvm-project/` | Ignored LLVM 23.1.2 source checkout |
| `build/` | Ignored LLVM and `circc` build tree |

Read [README.md](./README.md) before changing the build setup.

## Development environment

Pixi is the supported environment manager. It pins Clang, LLD, LLVM host
tools, CMake, Ninja, sccache, Python, and supporting libraries for `win-64` and
`linux-64`.

Windows builds use the MSVC ABI and require Visual Studio Build Tools and a
Windows SDK on the host.

The LLVM checkout is a dependency, not part of this repository. Do not edit
files under `.deps/llvm-project` to implement `circc` behavior. If an upstream
patch is genuinely necessary, keep it explicit and reviewable in this
repository rather than silently modifying the ignored checkout.

## Workflow

1. **Inspect** — Read the relevant source, CMake configuration, and upstream
   CIR/MLIR interfaces before deciding on an implementation.
2. **Implement** — Keep project code outside `.deps/llvm-project` and follow
   the LLVM conventions below.
3. **Format** — Use the pinned `clang-format` on new files and touched lines.
   Prefer `git clang-format`/`git-clang-format` when available to avoid
   unrelated formatting changes.
4. **Build** — Configure when CMake inputs changed, then build the `circc`
   target.
5. **Review** — For substantive behavior, IR transformation, ownership,
   control-flow, ABI, or public-interface changes, request an independent
   read-only subagent review when subagents are available. Validate findings
   yourself and rerun affected checks after fixes.
6. **Submit** — Do not modify Git or remote state unless explicitly requested
   by the human.

## Commands

Run commands from the repository root.

One-time LLVM checkout:

```console
pixi run fetch-llvm
```

Configure LLVM, Clang, MLIR, CIR, and `circc`:

```console
pixi run configure
```

Build only `circc` and its transitive dependencies:

```console
pixi run build
```

Smoke-check the current executable:

```console
pixi run circc --help
```

Run the `circc` regression tests:

```console
pixi run test
```

After changing `cmake/caches/LLVM.cmake`, remember that cache entries without
`FORCE` may retain their old values. Use a fresh configure when required:

```console
pixi run cmake --fresh -G Ninja -S .deps/llvm-project/llvm -B build -C cmake/caches/LLVM.cmake
```

Formatting examples:

```console
pixi run clang-format -i tools/circc/circc.cpp
git clang-format --diff
```

Do not run a full LLVM build or the full LLVM test suite when the narrower
`circc` target is sufficient. As tests are added, run the tests that cover the
changed behavior and document their command here.

## Coding conventions

- Follow the LLVM Coding Standards and the repository `.clang-format`. The
  pinned upstream reference is
  `.deps/llvm-project/llvm/docs/CodingStandards.md` after `fetch-llvm`.
- Match surrounding LLVM, Clang, CIR, and MLIR APIs rather than introducing a
  separate house style. In particular, use LLVM naming, include ordering,
  comment style, and error-handling conventions.
- LLVM is normally built without C++ exceptions and RTTI. Do not introduce
  code that requires exceptions, `dynamic_cast`, or RTTI unless the build
  configuration is intentionally changed and justified.
- Prefer LLVM support types and error facilities where they integrate with
  LLVM/CIR APIs; do not add parallel abstractions without a concrete need.
- Use lambdas only as anonymous, local callbacks at their call site. If a
  callable needs a name or reuse, define a regular function instead of storing
  a lambda in a named variable.
- Avoid C++ contextual (soft) keywords such as `module`, `import`, `final`,
  and `override` as variable or parameter names, even where they are legal
  identifiers. Prefer descriptive names such as `sourceModule`.
- Keep public interfaces small and document non-obvious ownership, lifetime,
  and IR invariants.
- Add tests with behavior changes. Translation work should eventually cover
  both emitted text and recompilation/semantic validation where applicable.
- Do not add project-specific style rules that conflict with upstream LLVM.
  When in doubt, follow the code in the pinned LLVM 23.1.2 tree.

## Commit messages

Use `<type>(<scope>): <subject>` with an optional explanatory body and
trailers. See [CONTRIBUTING.md](./CONTRIBUTING.md#commit-messages) and
[`.gitmessage`](./.gitmessage). Never create a commit unless the human
explicitly requests it.
