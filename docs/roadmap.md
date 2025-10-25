# AI-AutoShell Roadmap

## Fasi Completate

1. Lexer separato (tokenizzazione operatori, redir, assegnazioni basiche)
2. Parser + AST (list, and/or, pipeline, command, redirs, background flag)
3. Expander (~, $VAR, ${VAR})
4. Executor POSIX (pipeline, redirections, built-ins, basic job control, background)
5. Test unitari base (lexer, parser, expander, executor) 10/10 verdi

## In Corso / Prossimo

- Documentazione tecnica (architecture, grammar, roadmap) [IN CORSO]
- Estendere test per job control e errori redirection
- Cleanup finale (gestione $? più estesa, variabili speciali)

## Future Enhancements

- Job control avanzato (stop/continue, SIGTSTP, fg/bg completi)
- Globbing pattern (wildcards \* ? [])
- Command substitution `$( )` e backticks
- Here-document (<<)
- Subshell e grouping `( ... )`
- Store storico comandi e integrazione suggerimenti AI
- Layer sicurezza: analisi comando "pericoloso" + richiesta conferma
- Modalità offline/online per agent AI

## Qualità

- Aggiungere test stress su pipeline lunghe (>10 comandi)
- Sanitizers e eventualmente fuzzing con libFuzzer

## Distribuzione

- Packaging binario (macOS/Linux) via CPack / GitHub Releases
- Eventuale porting Windows (fase sperimentale)

## Versionamento

MVP target: v0.1.0
Obiettivo post-MVP con funzionalità avanzate: v0.2.x
Stabilità e plugin AI: v0.3.x

## Contributi

- Guidelines contributo da aggiungere (CONTRIBUTING.md)
- Codice sotto licenza MIT
