# Contributi a AI-AutoShell

Grazie per l'interesse! Questo progetto mira a costruire una shell autonoma sicura e modulare.

## Requisiti

- C++20
- CMake >= 3.16
- macOS o Linux (Windows in futuro)
- Ninja (opzionale per build più rapide)

## Setup rapido

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j
ctest --output-on-failure
```

## Linee guida codice

- Attiva sempre: `-Wall -Wextra -Wpedantic`
- Evita `system()`, `popen()`, shell wrapping (`sh -c`). Solo `fork` + `execvp`.
- Non introdurre dipendenze pesanti; prima valuta standard library.
- Mantieni funzioni piccole e a singola responsabilità.
- Usa header sotto `include/ai-autoshell/...` per API pubbliche.
- Commenta invarianti non ovvie e edge cases.

## Struttura modulo

1. Lexer → Tokens
2. Parser → AST
3. Expander → argv/file path normalizzati
4. Executor → processi/FD/jobs

## Test

- Ogni nuovo modulo richiede almeno 1 test happy path + 1 edge case.
- Usa GoogleTest; i file vanno in `tests/` con nome `test_<nome>.cpp`.
- Evita dipendenza dai test sull'ambiente dell'utente (HOME, PATH) — se necessario mocka.

## Commit

- Messaggi in inglese.
- Prefissi suggeriti: `feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `build:`.

## Pull Request

- Descrivi: problema, soluzione, rischi, test aggiunti.
- Mantieni PR piccole (< ~600 LOC diff quando possibile).

## Job Control (stato)

- Adesso: solo running/completed.
- Futuro: stopped (SIGTSTP), continue (SIGCONT), gestione foreground group.

## Sicurezza

- Nessuna esecuzione diretta di testo fornito da LLM.
- Validare input per path traversal dove rilevante.

## Roadmap extra

- Globbing, command substitution, here-docs.
- Integrazione LLM pianificatore.
- Porting Windows.

## Domande

Apri una Issue oppure discuti nel PR.

Buon hacking!
