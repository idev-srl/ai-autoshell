# AI-AutoShell Grammar (Semplificata)

Questa grammatica descrive l'MVP senza subshell né grouping.

Notation: `*` zero o più, `+` uno o più, `?` opzionale.

```
line        := list EOF
list        := and_or (';' and_or)* ';'?
and_or      := pipeline ( ( '&&' | '||' ) pipeline )*
pipeline    := command ( '|' command )*
command     := assigns* words redirs* background?
assigns     := ASSIGN+
words       := WORD+
redirs      := redir+
redir       := '>' WORD
             | '>>' WORD
             | '<' WORD
             | '2>' WORD
             | '2>&1'
background  := '&'
```

Token Types:

- WORD: sequenza di caratteri non separator (spazi o operatori), con quote e escape già risolte dal lexer.
- ASSIGN: pattern NAME=VALUE riconosciuto dal lexer (prefisso NAME in [A-Za-z\_][A-Za-z0-9_]\*).
- Operators: `| && || ; & > >> < 2> 2>&1`

Precedenza:

1. Redirections legate al comando.
2. Pipeline (associa a sinistra).
3. AND/OR, valutazione short-circuit.
4. List separato da ';'.

Background:

- Solo flag a livello di command (può apparire sull'ultimo comando di una pipeline per mettere l'intera pipeline in background).

Limitazioni:

- Nessun supporto ancora per parentesi, subshell, `$( )`, backticks, here-doc.
- Parsing tollerante: errori semplici (redir senza target) interrompono quella parte senza abort globale.
