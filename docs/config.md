# AI-AutoShell Configuration

Optional configuration file: `~/.ai-autoshellrc`

Format:

- Each line `key=value`
- `#` starts a comment
- Spaces are not auto-trimmed (avoid spaces around `=`)

Supported keys:

| Key           | Description                               | Default                 |
| ------------- | ----------------------------------------- | ----------------------- |
| prompt_format | Prompt template with placeholders         | `{user}@{host} {cwd}$ ` |
| color         | Enable ANSI color sequences in the prompt | `true`                  |

Available placeholders in `prompt_format`:

- `{user}` username
- `{host}` hostname
- `{cwd}` last component of current directory
- `{status}` exit status of last command (`$?`)

Examples:

```ini
# Compact prompt with status and cwd in brackets
prompt_format={user}@{host} [{cwd}] (st={status})$
color=true
```

```ini
# Prompt without colors
color=false
prompt_format={cwd}$
```

Colors:

- Current colors: user=green, host=blue, cwd=yellow, status=red.
- Disabling `color` removes ANSI codes.

Notes:

- If the file does not exist defaults are used.
- Unknown keys are ignored.
- Future: alias, history, LLM security levels, plugins.

Common errors:

- Using quotes: not needed (`prompt_format="{user}"` → quotes appear in prompt).
- Adding spaces after `=`: `prompt_format= {user}` produces leading space.

Quick test:

```sh
cp ai-autoshellrc.example ~/.ai-autoshellrc
$EDITOR ~/.ai-autoshellrc
./build/ai-autoshell
```

Future parameters (roadmap):

- `history_size=1000`
- `danger_confirm=on`
- `planner_mode=suggest|auto`
