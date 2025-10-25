# 📘 AI-AutoShell — Developer Instructions for Copilot

## 🎯 Obiettivo

Stiamo costruendo una **shell autonoma** in **C++20**, completamente **standalone** (nessuna dipendenza da `/bin/sh` o altre shell).  
Funzionerà su **macOS/Linux (POSIX)** e, successivamente, su **Windows**.

La shell dovrà:

- Fornire tutte le funzionalità di una shell base (esecuzione comandi, redirezioni, pipe, variabili, ecc.).
- Integrare un **LLM** per interpretare richieste naturali (“creami una cartella prova e un file README”) e convertirle in comandi eseguibili.
- Offrire una modalità “**auto**” (esegue automaticamente) e una “**suggest**” (solo suggerimenti).
- Richiedere conferma all’utente per operazioni “pericolose” (cancellazioni, rete, privilegi elevati, ecc.).
- Essere **open source**, modulare, documentata e facilmente estendibile.

---

## ✅ Stato attuale (MVP evoluto)

- Moduli implementati: Lexer, Parser+AST, Expander, Executor POSIX (pipeline, operatori logici, redirezioni, background), Job control basico.
- Built-in disponibili: `cd`, `pwd`, `exit`, `export`, `unset`, `echo`, `jobs`, `fg`, `bg`.
- Redirezioni: `>`, `>>`, `<`, `2>`, `2>&1` con gestione file descriptor sicura.
- Operatori supportati: `|`, `&&`, `||`, `;`, `&` (background su comando/pipeline).
- Espansioni: `~`, `$VAR`, `${VAR}` (mancano globbing e command substitution).
- Job control: aggiunta/lista job, foreground/background; assente gestione stop/continue (SIGTSTP/SIGCONT) per ora.
- Test GoogleTest: 15 test verdi (lexer, parser, expander, executor, jobs).
- Documentazione iniziale presente in `docs/` (architettura, grammatica, roadmap).

---

## 🧩 Architettura target

ai-autoshell/
├─ CMakeLists.txt
├─ src/
│ ├─ main.cpp
│ ├─ tty/
│ │ ├─ term.hpp / term.cpp
│ │ ├─ line_edit.hpp / line_edit.cpp
│ ├─ lex/
│ │ ├─ lexer.hpp / lexer.cpp
│ ├─ parse/
│ │ ├─ tokens.hpp
│ │ ├─ ast.hpp
│ │ └─ parser.cpp
│ ├─ expand/
│ │ ├─ expand.hpp / expand.cpp
│ ├─ exec/
│ │ ├─ path.hpp / path.cpp
│ │ ├─ redir.hpp / redir.cpp
│ │ ├─ builtins.hpp / builtins.cpp
│ │ ├─ job.hpp / job.cpp
│ │ ├─ executor_posix.hpp / executor_posix.cpp
│ ├─ llm/
│ │ ├─ schema.hpp
│ │ ├─ planner.hpp / planner.cpp
│ └─ util/
│ ├─ log.hpp / log.cpp
│ ├─ str.hpp / str.cpp
├─ include/
├─ tests/
│ ├─ test_lexer.cpp
│ ├─ test_parser.cpp
│ ├─ test_expand.cpp
│ ├─ test_executor_basic.cpp
└─ docs/
├─ architecture.md
├─ grammar.md
├─ roadmap.md
└─ security-policy.md

---

## 🧠 Funzionalità previste (Roadmap MVP)

1. **Lexer / Tokenizer**

   - Supporto quote singole/doppie, escape, operatori `|`, `;`, `&&`, `||`, `>`, `<`, `>>`.
   - Output: `std::vector<Token>` con posizione e tipo.

2. **Parser → AST**

   - AST con nodi `Command`, `Pipeline`, `List`, `AndOr`, `Redir`.
   - Precedenze: `|` < `&&`/`||` < `;`.
   - Supporto redirezioni `>`, `>>`, `<`, `2>`, `2>&1`.

3. **Expander**

   - Espansione `~`, `$VAR`, `${VAR}` (no command substitution al MVP).

4. **Executor POSIX**

   - `fork`, `execvp`, `pipe`, `dup2`, gestione `SIGINT`, `SIGCHLD`.
   - Redirezioni e job control (`fg`, `bg`, `jobs`).
   - Built-in: `cd`, `pwd`, `exit`, `export`, `unset`, `echo`.

5. **LLM Integration (futuro)**
   - Modulo `llm/planner.cpp`: converte richieste naturali → JSON di piani.
   - Esecuzione controllata in base al campo `danger` (es: “dangerous”, “network”, ecc.).

---

## 📜 Grammar di riferimento (EBNF)

line := list
list := and_or { ‘;’ and_or }
and_or := pipeline { ( ‘&&’ | ‘||’ ) pipeline }
pipeline := command { ‘|’ command }
command := (assign_list)? simple (redir_list)? (background)?
assign_list := assign { assign }
assign := NAME ‘=’ value
redir_list := redir { redir }
redir := ‘>’ file | ‘>>’ file | ‘<’ file | ‘2>’ file | ‘2>&1’
background := ‘&’
simple := WORD { WORD }
value := QUOTED | WORD
file := WORD | QUOTED

---

## 🧱 Policy LLM (schema JSON futuro)

```json
{
  "steps": [
    {
      "argv": ["mkdir", "-p", "/home/user/docs"],
      "cwd": "/home/user",
      "env": {"KEY": "VALUE"},
      "redir": {"stdout": null, "stderr": null, "append_stdout": false},
      "background": false,
      "timeout_ms": 0,
      "danger": "none|elevated|dangerous|network",
      "why": "create a new folder for docs"
    }
  ]
}

⚠️ Nessuna stringa shell. Solo argv[] + strutture JSON valide.
⚠️ danger != none → richiesta conferma utente.

⸻

⚙️ Standard di sviluppo
	•	C++20, -Wall -Wextra -Wpedantic
	•	Debug: -fsanitize=address,undefined
	•	Error handling: std::optional / int + log su stderr
	•	No uso di system(), popen(), /bin/sh, boost
	•	No esecuzione diretta di testo generato da LLM
	•	Test: GoogleTest con ctest

⸻

🧩 Task list per Copilot

🧱 Fase 1 — Refactor
	•	Crea struttura di cartelle come sopra.
	•	Sposta la logica di tokenizzazione in lex/lexer.cpp con header.
	•	Crea parse/tokens.hpp con enum class TokenKind.
	•	Implementa lexer che produce Token { kind, lexeme, pos }.

✅ Test: parsing di echo "ciao mondo" && ls -l | grep cpp

⸻

🧱 Fase 2 — Parser / AST
	•	parse/ast.hpp: definisci strutture per Command, Pipeline, AndOr, List, Redir.
	•	parser.cpp: parser ricorsivo con precedenze.
✅ Test: parsing di cd /tmp && echo ok || echo fail; pwd

⸻

🧱 Fase 3 — Expander
	•	expand.cpp: gestisci ~, $VAR, ${VAR} su argomenti e file.
✅ Test: echo $HOME, echo ~/Documents

⸻

🧱 Fase 4 — Executor POSIX
	•	redir.cpp: implementa >, >>, <, 2>, 2>&1 con dup2.
	•	path.cpp: risoluzione PATH.
	•	executor_posix.cpp: gestione fork, execvp, waitpid, pipe.
	•	builtins.cpp: cd, pwd, exit, export, unset, echo, jobs, fg, bg.
✅ Test: echo test | cat, false && echo no, sleep 1 &, jobs

⸻

🧱 Fase 5 — Cleanup
	•	main.cpp deve solo gestire REPL → Lexer → Parser → Expander → Executor.
	•	Gestisci $? per ultimo status.
✅ Test: Ctrl-C non chiude la shell ma interrompe il processo figlio.

⸻

🧱 Fase 6 — Docs & Tests
	•	Scrivi documentazione tecnica (docs/*.md).
	•	Implementa test unitari (tests/) per lexer, parser, expander, executor.
✅ Test finale: ctest verde.

⸻

🚫 Da non fare
	•	❌ Niente system(), popen(), sh -c, ecc.
	•	❌ Niente concatenazione testuale di comandi.
	•	❌ Niente comandi con sudo o root escalation automatica.
	•	✅ Tutto deve passare per exec nativo e controllato.

⸻

🔮 Prossimi step (dopo MVP)
	•	Porting Windows (CreateProcess, Job Objects, console handling).
	•	Implementazione LLM con schema JSON → esecuzione controllata.
	•	Config file YAML/TOML in ~/.ai-autoshell/config.yaml.
	•	Modalità auto vs suggest.
	•	Integrazione plugin/moduli.

⸻

🧠 Copilot, procedi con i Task in ordine.
Mantieni compatibilità POSIX, modularità, sicurezza, e segui lo schema sopra per codice, test e documentazione.

---

👉 Copia **tutto** il blocco sopra (incluso il markdown) e incollalo nella chat di Copilot su VS Code.
In questo modo Copilot capirà la visione completa, la struttura, e potrà iniziare a generare i moduli successivi automaticamente.
```
