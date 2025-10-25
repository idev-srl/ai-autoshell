# AI-AutoShell Architecture

## Overview

AI-AutoShell is a minimal modular C++20 shell composed of a pipeline of phases:
Lexer -> Parser -> Expander -> POSIX Executor. Basic job control.

## Layering

1. Lex (`include/ai-autoshell/lex`): transforms input line into TokenStream.
2. Parse (`include/ai-autoshell/parse`): produces AST (List -> AndOr -> Pipeline -> Command).
3. Expand (`include/ai-autoshell/expand`): simple expansions (~, $VAR, ${VAR}).
4. Exec (`include/ai-autoshell/exec`): path resolution, redirections, built-ins, job control, executor.

## Main Loop

`src/main.cpp`:

- read line
- Lexer::run -> tokens
- parse_tokens -> AST
- ExecutorPOSIX.run(AST)
- update $? (m_ctx.last_status)

## AST

ListNode: segments separated by ';'
AndOrNode: sequence of Pipeline with logical operators ("&&","||")
PipelineNode: N CommandNode with pipes
CommandNode: argv, redirs, assigns, background flag

## Redirections

Supported: > >> < 2> 2>&1
Implementation: `apply_redirections` opens files and dup2 onto target fds.

## Built-ins

cd, pwd, echo, export, unset, exit, jobs, fg, bg.
Redirections applied by duplicating fds (save/restore).

## Job Control

`JobTable` tracks jobs: id, pgid, running, background.
Single command or pipeline background: non-blocking fork, setpgid, add job.
`jobs`: reaping and listing.
`fg`: wait on process group.
`bg`: stub (just reap) for now.

## Error Handling

- Command not found -> status 127.
- fork/pipe/open failure -> perror + status 1.

## Future Extensions

- Wildcard globbing
- Subshell & grouping
- Command substitution
- Security (confirm sensitive commands)
- AI suggestions integration

## Dependencies

Only libc++/libstdc++, GoogleTest for tests.

## Build

CMake >=3.16, target `ai-autoshell` + test executables.

## Process Groups

Pipeline: all children in same pgid for job control.

## Signals

SIGINT reset to default in children; ignored for single background.

## Current Limitations

- Partial job control (bg doesn't reactivate stop)
- No advanced parsing of nested quotes
- No support for special variables beyond $?
