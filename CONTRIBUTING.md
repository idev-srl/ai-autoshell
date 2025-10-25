# Contributing to AI-AutoShell

Thanks for your interest! This project aims to build a safe, modular autonomous shell.

## Requirements

- C++20
- CMake >= 3.16
- macOS or Linux (Windows in the future)
- Ninja (optional for faster builds)

## Quick Setup

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j
ctest --output-on-failure
```

## Code Guidelines

- Always enable: `-Wall -Wextra -Wpedantic`.
- Avoid `system()`, `popen()`, shell wrapping (`sh -c`). Only use `fork` + `execvp`.
- Do not introduce heavy dependencies; prefer the standard library first.
- Keep functions small and single-responsibility.
- Public API headers belong under `include/ai-autoshell/...`.
- Comment non-obvious invariants and edge cases.

## Module Flow

1. Lexer → Tokens
2. Parser → AST
3. Expander → normalized argv / file paths
4. Executor → processes / FDs / jobs

## Tests

- Each new module requires at least 1 happy path test + 1 edge case.
- Use GoogleTest; files go in `tests/` named `test_<name>.cpp`.
- Avoid relying on the user environment (HOME, PATH); mock if necessary.

## Commits

- Messages in English.
- Suggested prefixes: `feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `build:`.

## Pull Requests

- Describe: problem, solution, risks, added tests.
- Keep PRs small (< ~600 LOC diff when possible).

## Job Control (status)

- Current: only running/completed.
- Future: stopped (SIGTSTP), continue (SIGCONT), foreground group management.

## Security

- No direct execution of text supplied by an LLM.
- Validate input for path traversal when relevant.

## Extra Roadmap

- Globbing, command substitution, here-docs.
- LLM planner integration.
- Windows port.

## Questions

Open an Issue or discuss in the PR.

Happy hacking!
