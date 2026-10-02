# Contributing to `circc`

Thanks for your interest in contributing.

`circc` is developed as an external project in a pinned LLVM/ClangIR source
tree. For environment setup, build commands, validation, and coding
conventions, see [AGENTS.md](./AGENTS.md).

## Commit messages

Commits use the following structure:

```text
<type>(<scope>): <subject>

<body>

<footer>
```

- **type** — `feat`, `fix`, `refactor`, `perf`, `style`, `docs`, `test`,
  `build`, `ci`, `chore`, or `revert`. Use `chore` when no more specific type
  applies.
- **scope** — optional; use the affected area, such as `cli`, `cir`, `emitc`,
  `cmake`, `pixi`, `tests`, `docs`, or `ci`.
- **subject** — lowercase imperative mood, no trailing period, at most 72
  columns.
- **body** — optional, wrapped at 72 columns. Explain why the change is needed;
  the diff already shows what changed.
- **footer** — optional issue references and attribution trailers.

A concise template is available in [`.gitmessage`](./.gitmessage). To use it
for this checkout without changing repository contents:

```console
git config commit.template .gitmessage
```

### Attribution

When a model materially contributes code, documentation, tests, or other
committed content, record that provenance with:

```text
Assisted-by: <model name> [<version if known>]
```

Include a version only when it is known. Do not add the trailer for discussion,
search, or review when no model-produced material is included. Reserve
`Co-authored-by` for human collaborators.

## Pull requests

Keep pull requests focused and include:

- the problem and intended behavior;
- relevant CIR/MLIR or LLVM compatibility considerations;
- the checks that were run;
- focused tests for behavior changes.

Do not include generated LLVM build outputs or modifications made directly in
the ignored `.deps/llvm-project` checkout.

## Reporting issues

Include expected behavior, actual behavior, a minimal CIR input when possible,
and platform/toolchain details such as OS, target triple, LLVM version, and
CMake configuration.

Avoid publishing exploit details for security-sensitive issues before the
maintainers have had an opportunity to respond.

## License

Contributions are licensed under the project license in [LICENSE](./LICENSE).
