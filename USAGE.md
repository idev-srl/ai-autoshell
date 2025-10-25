# AI-AutoShell Usage Guide

This guide summarizes what you can do today with AI-AutoShell (evolved MVP).

## Startup

Launch `ai-autoshell` and you'll see a banner with system info and capabilities.

Prompt: `user@host [cwd]$`

## Built-in Commands

- `cd [dir]` changes directory (supports `cd -`).
- `pwd` prints current directory.
- `echo ...` prints arguments (variable & tilde expansion before globbing).
- `export VAR=VAL [OTHER=...]` sets environment variables.
- `unset VAR [...]` removes variables.
- `jobs` shows background / stopped / finished jobs.
- `fg <id>` brings a job to foreground (resumes if stopped).
- `bg <id>` resumes a stopped job in background.
- `exit` closes the shell.

## External Execution

Use standard `argv`: e.g. `ls -l`, `grep pattern file.txt`.
The shell resolves the executable scanning `$PATH`.

## Pipelines & Logic

- Pipeline: `cmd1 | cmd2 | cmd3`
- AND / OR: `cmd1 && cmd2 || cmd3`
- Sequence: `cmd1; cmd2; cmd3`

## Background & Job Control

- Append `&` at end of a command or pipeline: `sleep 5 &`
- Stopped job (Ctrl-Z) recognized if process receives `SIGTSTP`.
- `jobs`, `fg`, `bg` manage states (running, stopped, done).

## Redirections

- Output: `>` overwrite, `>>` append
- Input: `<`
- Error: `2>`
- Merge stdout/stderr: `2>&1`

Example: `grep foo file.txt > out.log 2>&1`

## Expansions

- Tilde: `~` and `~/subdir`
- Variables: `$VAR`, `${VAR}`
- Globbing: `*`, `?`, `[abc]` (cwd only, no recursive path patterns)
  - No match ⇒ word unchanged

Examples:

- `echo ~$USER` expands HOME and USER
- `echo *.txt` lists .txt files in cwd
- `echo data_??.log` files with two chars between underscore and .log

## Last Command Status

Special variable `$?` (also exported as env `?`) holds exit status of last command.

## Recently Added Advanced Features

- Subshell grouping: wrap commands in `(...)` even in pipelines: `(echo hi)|cat`.
- Command substitution: `echo X$(echo hi)Y` → `XhiY`. (MVP: single non-nested substitution per token.)
- Ctrl-Z (SIGTSTP) forwarded to foreground group: suspends job which then appears in `jobs` as stopped and can be resumed with `fg` or `bg`.
- History with arrows: Use ↑ and ↓ to navigate previous commands.
- Enhanced tab completion:
  - Built-ins, files and directories in cwd (directories suffixed '/').
  - After `cd` show only directories (supports relative `./`, `../`, absolute `/` and HOME `~/`).
  - Hidden names (starting with '.') appear only if prefix started with '.'.
  - Commands from `$PATH` cached (invalidation when `PATH` changes).
  - First Tab: complete common prefix or single match (adds '/' if directory, space if file/command).
  - Second consecutive Tab (without edits): shows candidate list formatted in columns.
  - Directory caching reduces FS access (no change detection yet; update by changing directory).

## Existing Limitations

- Globbing does not traverse directories (`src/*.cpp` unsupported).
- Missing here-doc (`<<`), process substitution, alias, persistent history.
- Command substitution lacks multi-nesting or multiple occurrences in same token.
- Subshell does not isolate environment variables (shared MVP context).
- LLM safety layer and planner not implemented yet.

## Quick Debug

If something seems stuck:

- Use `jobs` to view active processes.
- Send `Ctrl-C` to interrupt foreground job.

## Complete Examples

```sh
# Pipeline + redirection
cat file.txt | grep ERROR | wc -l > error_count.txt

# And/Or
make && echo "Build ok" || echo "Build failed"

# Background
sleep 10 &
jobs

# Foreground a job
fg 1

# Globbing
echo __glob_*.txt

# Variables
export NAME=Luigi
echo "Hello $NAME"
```

## Next Steps (condensed roadmap)

- Command substitution, grouping, here-doc.
- Better job control (Ctrl-Z, automatic stop/continue).
- LLM layer with JSON plans.
- User configuration and plugins.

Happy exploring!

## Configuration (rc file)

Create `~/.ai-autoshellrc` to customize prompt.

Supported keys:

- `prompt_format` template with placeholders `{user} {host} {cwd} {status}`
- `color` enable/disable ANSI colors (`true/false`)

Example:

```ini
prompt_format={user}@{host} [{cwd}] (st={status})$
color=true
```

See `ai-autoshellrc.example` and `docs/config.md` for details.

## .ash Scripts

You can run non-interactive scripts via runner:

```sh
./build/ai-autoshell-script examples/hello.ash
```

Features:

- Executes each line (skips empty and `#` comments).
- Uses same engine as interactive shell (lexer, parser, executor, job control).
- Interrupt with Ctrl-C stops execution.

Current limitations (script runner):

- No `set -e`, no functions or `{ }` blocks.
- Variables propagate only in runner parent process (like normal exports).
- No here-doc; command substitution available with above limits.

Basic example (`examples/hello.ash`):

```sh
echo "Script start"
export NAME=Luigi
echo "Hello $NAME"
ls | grep cpp || echo "No cpp"
sleep 1 &
jobs
fg 1
echo "End script (previous status=$?)"
```
