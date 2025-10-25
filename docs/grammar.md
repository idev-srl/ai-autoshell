# AI-AutoShell Grammar (Simplified)

This grammar describes the MVP without subshell or grouping.

Notation: `*` zero or more, `+` one or more, `?` optional.

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

- WORD: sequence of non-separator characters (spaces or operators), with quotes and escapes already resolved by the lexer.
- ASSIGN: pattern NAME=VALUE recognized by the lexer (NAME prefix in [A-Za-z\_][A-Za-z0-9_]\*).
- Operators: `| && || ; & > >> < 2> 2>&1`

Precedence:

1. Redirections bind to the command.
2. Pipeline (left associative).
3. AND/OR, short-circuit evaluation.
4. List separated by ';'.

Background:

- Only a flag at command level (may appear on the last command of a pipeline to put the entire pipeline in background).

Limitations:

- No support yet for parentheses, subshell, `$( )`, backticks, here-doc.
- Lenient parsing: simple errors (redir without target) interrupt that part without global abort.
