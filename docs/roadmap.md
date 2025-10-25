# AI-AutoShell Roadmap

## Completed Phases

1. Separate lexer (tokenizing operators, redir, basic assignments)
2. Parser + AST (list, and/or, pipeline, command, redirs, background flag)
3. Expander (~, $VAR, ${VAR})
4. POSIX executor (pipeline, redirections, built-ins, basic job control, background)
5. Basic unit tests (lexer, parser, expander, executor) 10/10 green

## In Progress / Next

- Technical documentation (architecture, grammar, roadmap) [IN PROGRESS]
- Extend tests for job control and redirection errors
- Final cleanup (broader $? handling, special variables)

## Future Enhancements

- Advanced job control (stop/continue, SIGTSTP, fg/bg complete)
- Globbing patterns (wildcards \* ? [])
- Command substitution `$( )` and backticks
- Here-document (<<)
- Subshell and grouping `( ... )`
- Store command history and AI suggestion integration
- Security layer: "dangerous" command analysis + confirmation
- Offline/online modes for AI agent

## Quality

- Add stress tests on long pipelines (>10 commands)
- Sanitizers and eventual fuzzing with libFuzzer

## Distribution

- Binary packaging (macOS/Linux) via CPack / GitHub Releases
- Possible Windows port (experimental phase)

## Versioning

MVP target: v0.1.0
Post-MVP advanced features: v0.2.x
Stability and AI plugins: v0.3.x

## Contributions

- Contribution guidelines to add (CONTRIBUTING.md)
- Code under MIT license
