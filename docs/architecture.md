# AI-AutoShell Architecture

## Overview

AI-AutoShell è una shell minimale C++20 modulare composta da pipeline di fasi:
Lexer -> Parser -> Expander -> Executor POSIX. Job control basilare.

## Layering

1. Lex (`include/ai-autoshell/lex`): trasforma input line in TokenStream.
2. Parse (`include/ai-autoshell/parse`): produce AST (List -> AndOr -> Pipeline -> Command).
3. Expand (`include/ai-autoshell/expand`): espansioni semplici (~, $VAR, ${VAR}).
4. Exec (`include/ai-autoshell/exec`): path resolution, redirections, built-ins, job control, executor.

## Main Loop

`src/main.cpp`:

- legge linea
- Lexer::run -> tokens
- parse_tokens -> AST
- ExecutorPOSIX.run(AST)
- aggiorna $? (m_ctx.last_status)

## AST

ListNode: segmenti separati da ';'
AndOrNode: sequenza di Pipeline con operatori logici ("&&","||")
PipelineNode: N CommandNode con pipe
CommandNode: argv, redirs, assigns, background flag

## Redirections

Supporto: > >> < 2> 2>&1
Implementazione: `apply_redirections` apre file e dup2 su fd target.

## Built-ins

cd, pwd, echo, export, unset, exit, jobs, fg, bg.
Redirezioni applicate duplicando fds (salvataggio/restauro).

## Job Control

`JobTable` traccia jobs: id, pgid, running, background.
Background singolo comando o pipeline: fork non bloccante, setpgid, aggiunta job.
`jobs`: reaping e listing.
`fg`: attesa gruppo di processo.
`bg`: stub (solo reap).

## Error Handling

- Comando non trovato -> status 127.
- Fallimento fork/pipe/open -> perror + status 1.

## Estensioni Future

- Globbing wildcard
- Subshell e grouping
- Command substitution
- Sicurezza (conferme per comandi sensibili)
- Integrazione AI suggerimenti

## Dipendenze

Solo libc++/libstdc++, GoogleTest per test.

## Build

CMake >=3.16, target `ai-autoshell` + test executables.

## Process Groups

Pipeline: tutti i figli nello stesso pgid per job control.

## Segnali

SIGINT reset a default nei child; ignorato per background singolo.

## Limitazioni Attuali

- Job control parziale (bg non riattiva stop)
- Nessun parsing avanzato di quote nested
- Nessun supporto a variabili speciali oltre $?.
