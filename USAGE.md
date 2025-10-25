# AI-AutoShell Usage Guide

Questa guida riassume ciò che puoi fare oggi con AI-AutoShell (MVP evoluto).

## Avvio

Lancia `ai-autoshell` e vedrai un banner con informazioni di sistema e funzionalità.

Prompt: `user@host [cwd]$`

## Comandi built-in

- `cd [dir]` cambia directory (supporta `cd -`).
- `pwd` stampa la directory corrente.
- `echo ...` stampa gli argomenti (espansione variabili e tilde prima del globbing).
- `export VAR=VAL [ALTRE=...]` imposta variabili ambiente.
- `unset VAR [...]` rimuove variabili.
- `jobs` mostra job in background / stopped / conclusi.
- `fg <id>` porta un job in foreground (riprende se stoppato).
- `bg <id>` riprende un job stoppato in background.
- `exit` chiude la shell.

## Esecuzione esterna

Usa `argv` normali: es. `ls -l`, `grep pattern file.txt`.
La shell risolve l'eseguibile scorrendo `$PATH`.

## Pipeline & Logica

- Pipeline: `cmd1 | cmd2 | cmd3`
- AND / OR: `cmd1 && cmd2 || cmd3`
- Sequenza: `cmd1; cmd2; cmd3`

## Background & Job Control

- Aggiungi `&` alla fine di un comando o pipeline: `sleep 5 &`
- Job stoppato (Ctrl-Z non ancora implementato) è riconosciuto se il processo riceve `SIGTSTP`.
- `jobs`, `fg`, `bg` gestiscono stato (running, stopped, done).

## Redirezioni

- Output: `>` sovrascrive, `>>` accoda
- Input: `<`
- Error: `2>`
- Unisci stdout/stderr: `2>&1`

Esempio: `grep foo file.txt > out.log 2>&1`

## Espansioni

- Tilde: `~` e `~/subdir`
- Variabili: `$VAR`, `${VAR}`
- Globbing: `*`, `?`, `[abc]` (solo nel cwd, niente percorsi annidati)
  - Nessun match ⇒ parola invariata

Esempi:

- `echo ~$USER` → espande HOME e USER
- `echo *.txt` → elenca file .txt in cwd
- `echo data_??.log` → file con due caratteri tra underscore e .log

## Stato ultimo comando

Variabile speciale `$?` (anche esportata come env `?`) contiene exit status dell'ultimo comando.

## Funzionalità avanzate appena aggiunte

- Subshell grouping: racchiudi comandi tra parentesi `(...)` anche in pipeline: `(echo hi)|cat`.
- Command substitution: `echo X$(echo hi)Y` produce `XhiY`. (Implementazione MVP: una sostituzione semplice non annidata per token.)
- Ctrl-Z (SIGTSTP) inoltrato al gruppo foreground: sospende correttamente il job che poi appare in `jobs` come stoppato e può essere ripreso con `fg` o `bg`.
- History con frecce: Usa ↑ e ↓ per navigare i comandi precedenti.
- Tab completion evoluta:
  - Built-in, file e directory nel cwd (directory con suffisso '/').
  - Dopo `cd` vengono mostrati solo le directory (supporta prefissi relativi `./`, `../`, assoluti `/` e HOME `~/`).
  - I nomi nascosti (che iniziano con '.') compaiono solo se il prefisso iniziava con '.'.
  - Comandi dal `$PATH` cache-izzati (invalidazione quando cambia `PATH`).
  - Primo Tab: completa prefisso comune o singolo match (aggiunge '/' se directory, spazio se file/command).
  - Secondo Tab consecutivo (senza modifiche): mostra lista candidati formattata a colonne.
  - Caching directory per ridurre accessi FS (non ancora invalidazione su modifiche; aggiornamento manuale cambiando directory).

## Limitazioni ancora presenti

- Globbing non attraversa directory (`src/*.cpp` non supportato).
- Mancano here-doc (`<<`), process substitution, alias, history persistente.
- Command substitution non supporta annidamenti multipli o più occorrenze nello stesso token.
- Subshell non isola variabili d'ambiente (condivisione diretta del contesto MVP).
- Sicurezza LLM e pianificazione non ancora implementate.

## Debug rapido

Se qualcosa sembra bloccato:

- Usa `jobs` per vedere processi attivi.
- Invia `Ctrl-C` per interrompere il primo piano.

## Esempi completi

```sh
# Pipeline + redirezione
cat file.txt | grep ERROR | wc -l > error_count.txt

# And/Or
make && echo "Build ok" || echo "Build fallita"

# Background
sleep 10 &
jobs

# Foreground di un job
fg 1

# Globbing
echo __glob_*.txt

# Variabili
export NAME=Luigi
echo "Ciao $NAME"
```

## Prossimi passi (roadmap sintetica)

- Command substitution, grouping, here-doc.
- Miglior job control (Ctrl-Z, stop/continue automatico).
- Layer LLM con piani JSON.
- Configurazione utente e plugin.

Buona esplorazione!

## Configurazione (rc file)

Puoi creare `~/.ai-autoshellrc` per personalizzare il prompt.

Chiavi supportate:

- `prompt_format` template con placeholder `{user} {host} {cwd} {status}`
- `color` abilita/disabilita colori ANSI (`true/false`)

Esempio:

```ini
prompt_format={user}@{host} [{cwd}] (st={status})$
color=true
```

Vedi `ai-autoshellrc.example` e `docs/config.md` per dettagli.

## Script .ash

Puoi eseguire script non interattivi con il runner:

```
./build/ai-autoshell-script examples/hello.ash
```

Caratteristiche:

- Esegue ogni linea (ignora vuote e commenti `#`).
- Usa lo stesso motore della shell interattiva (lexer, parser, executor, job control).
- Interruzione con Ctrl-C ferma l'esecuzione.

Limitazioni attuali (script runner):

- Nessun `set -e`, nessuna gestione di funzioni o blocchi `{ }`.
- Variabili si propagano solo nel processo padre del runner (come export normali).
- Non supporta here-doc; command substitution disponibile ma con i limiti sopra.

Esempio base (`examples/hello.ash`):

```
echo "Inizio script"
export NAME=Luigi
echo "Ciao $NAME"
ls | grep cpp || echo "Nessun cpp"
sleep 1 &
jobs
fg 1
echo "Fine script (status precedente=$?)"
```
