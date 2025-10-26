<div align="center">

<h1>AI-AutoShell</h1>
<p>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/License-MIT-blue.svg" /></a>
  <a href="https://github.com/idev-srl/ai-autoshell/actions/workflows/build.yml"><img alt="Build" src="https://github.com/idev-srl/ai-autoshell/actions/workflows/build.yml/badge.svg" /></a>
  <a href="https://github.com/idev-srl"><img alt="Made by iDev" src="https://img.shields.io/badge/Made%20by-iDev-purple.svg" /></a>
</p>

</div>

AI-AutoShell is a modern, self-contained shell written in C++20. It does <em>not</em> delegate to /bin/sh; it implements its own lexer, parser, expander and POSIX executor with job control. Future versions will integrate an LLM that can translate natural language requests into safe, structured execution plans.

---

## Table of Contents

1. Motivation & Vision
2. Key Features (MVP)
3. Project Layout
4. Building & Running
5. Configuration (`~/.ai-autoshellrc`)
6. Command Completion & Colors
7. Roadmap
8. Contributing
9. Security & Safety Principles
10. License

---

## 1. Motivation & Vision

Traditional shells mix parsing, expansion and execution with decades of legacy behavior. AI-AutoShell aims to:

- Provide a clean, testable C++20 implementation of a POSIX-like interactive shell.
- Offer structured execution primitives (argv + redirections) suitable for LLM planning without free-form text evaluation.
- Support an "auto" (execute) and "suggest" (propose only) AI mode for safe adoption.
- Maintain strict safety around dangerous operations (deletions, privileged, network).

## 2. Key Features (Current MVP)

- Built-ins: `cd`, `pwd`, `exit`, `echo`, `export`, `unset`, `jobs`, `fg`, `bg`.
- Operators: pipelines `|`, logical `&&` / `||`, list `;`, background `&`.
- Redirections: `>`, `>>`, `<`, `2>`, `2>&1`.
- Expansions: `~`, `$VAR`, `${VAR}` (globbing & command substitution pending).
- Job control: list jobs, foreground/background movement, basic signals.
- Interactive line editor: history navigation (↑/↓), tab completion (context-aware, directory filtering for `cd`), colored suggestions.
- Clean separation: Lexer → Parser (AST) → Expander → Executor POSIX.

## 3. Project Layout

```text
CMakeLists.txt
src/
  main.cpp                # REPL
  line/line_editor.cpp    # Minimal raw TTY editor with completion
  lex/lexer.cpp           # Tokenization
  parse/parser.cpp        # AST construction
  expand/expand.cpp       # Variable & tilde expansion
  exec/executor_posix.cpp # Process spawning, pipelines, jobs
include/ai-autoshell/...  # Public headers
tests/                    # GoogleTest suites
docs/                     # Architecture, grammar, roadmap, security
```

## 4. Building & Running

Requirements: C++20 compiler (clang++ or g++), CMake ≥ 3.24.

```

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/ai-autoshell

```

Run tests (if present):

```

ctest --test-dir build --output-on-failure

```

## 5. Configuration (`~/.ai-autoshellrc`)

Create a file in your home directory:

```

color=on

```

Placeholders: `{user}`, `{host}`, `{cwd}`, `{status}` (last exit code). Set `color=off` to disable ANSI colors.

## 6. Command Completion & Colors

Tab completion supports:

- Built-ins (cyan)
- Executables in `$PATH` (green, cached)
- Directories (blue, trailing `/`)
- Context-sensitive `cd` (only directories, supports `~/`, `./`, `../`, absolute paths)
  Hidden entries only shown when you start the prefix with `.`. Double-Tab lists matches in columns with colors.

## AI Planner & LLM Configuration

The shell includes an experimental AI planner built-in `ai` (enable via rc file).

Usage:

```sh
ai suggest list files in current directory
aio auto clean build folder
./build/ai-autoshell --ai-debug   # avvia con output JSON completo dei piani
```

Modes:

- suggest: show JSON plan, do not execute
- auto: generate plan then execute sequentially (with confirmation if dangerous)

Config keys in `~/.ai-autoshellrc`:

| Key               | Meaning                                            | Example                                                 |
| ----------------- | -------------------------------------------------- | ------------------------------------------------------- |
| ai_enabled        | Enable AI features                                 | ai_enabled=true                                         |
| planner_mode      | Default mode (suggest/auto)                        | planner_mode=suggest                                    |
| llm_provider      | Provider tag (openai/anthropic/local/none)         | llm_provider=openai                                     |
| llm_model         | Model id                                           | llm_model=gpt-4o-mini                                   |
| llm_endpoint      | Override HTTPS endpoint                            | llm_endpoint=https://api.openai.com/v1/chat/completions |
| llm_api_key_env   | Env var containing API key                         | llm_api_key_env=OPENAI_API_KEY                          |
| llm_enabled       | Enable LLM enrichment of first step (default true) | llm_enabled=true                                        |
| llm_stub_file     | Path to local stub response file                   | llm_stub_file=/path/plan_hint.txt                       |
| llm_api_key       | Direct API key (NOT recommended; prefer env var)   | llm_api_key=sk-XXXX                                     |
| (flag) --ai-debug | Show full JSON plan output (otherwise summary)     | ./build/ai-autoshell --ai-debug                         |

Current implementation is rule-based. If `llm_enabled=true` a lightweight enrichment is performed:

Priority order for enrichment source:

1. Stub file (`llm_stub_file`): entire file content appended as note.
2. Simulated remote (if provider!=none and env key present OR direct key) -> placeholder string.
3. Echo fallback (just returns prompt text).

Only the first plan step description is extended with: `(LLM note: ...)`.

Default non-debug output (`ai suggest ...`) mostra solo il numero di step e i comandi. Usa `--ai-debug` per il JSON completo.

Security note: usa `llm_api_key_env` invece di `llm_api_key` per non salvare segreti in chiaro su disco.

JSON plan schema:

```json
{
  "request": "list files",
  "dangerous": false,
  "risk_summary": "",
  "steps": [
    {
      "id": "s1",
      "description": "List files in current directory",
      "command": "ls -la",
      "confirm": false
    }
  ]
}
```

Dangerous detection (confirmation required): commands containing `rm `, `sudo`, `chmod 777`.

## 7. Roadmap

Near-term:

- Globbing (`*`, `?`, `[]`)
- Command substitution `$(...)`
- Safer signal handling & job suspension/resume improvements
- LLM planner module producing JSON execution plans (no free-form shell strings)
  Mid-term:
- Windows port (CreateProcess, pipes, job objects)
- Config via YAML/TOML
- "Suggest" vs "Auto" AI modes with confirmation layer
- Plugin/module system

## 8. Contributing

Pull requests welcome. Please:

1. Keep changes modular (one concern per PR).
2. Add/extend tests for new behavior.
3. Run `ctest` and ensure green before requesting review.
4. Avoid introducing unsafe constructs (`system`, `popen`, shell text concatenation).

## 9. Security & Safety Principles

- No execution of raw text generated by LLM; only structured argv vectors.
- Confirmation required for elevated/dangerous operations (future AI integration).
- No automatic privilege escalation.
- Defensive error handling: prefer explicit checks & clear stderr messages.

## 10. License

MIT © iDev. See `LICENSE` (to be added if missing). Feel free to use, modify, and redistribute with attribution.

---

### Development Standards

Compiler flags: `-Wall -Wextra -Wpedantic` (consider sanitizers in debug: `-fsanitize=address,undefined`).
Testing: GoogleTest via CTest.
Avoid: `system()`, `popen()`, `/bin/sh` delegation, dynamic code eval.

### Quick Example

```bash
echo "hello" | cat
false && echo no || echo recovered
cd /tmp && echo "In tmp"; pwd
```

### Status Environment Variable

The shell exports `?` with the last command exit code (e.g., use `{status}` in the prompt format).

---

If you encounter issues or have feature ideas, open an issue. Contributions that improve robustness, test coverage, or safety are highly appreciated.
