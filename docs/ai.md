# AI Planner (MVP)

The AI planner converts a natural language request into a structured execution plan.

## Goals

- Never execute raw generated shell strings unchecked.
- Provide auditable JSON plan before execution (suggest mode).
- Require confirmation for dangerous steps.

## Plan Schema

```json
{
  "request": "original text",
  "dangerous": true,
  "risk_summary": "Step s2 requires confirmation; ",
  "steps": [
    {
      "id": "s1",
      "description": "List files in current directory",
      "command": "ls -la",
      "confirm": false
    },
    {
      "id": "s2",
      "description": "Remove build directory",
      "command": "rm -rf build",
      "confirm": true
    }
  ]
}
```

Fields:

- request: user natural language input
- dangerous: at least one step confirm=true
- risk_summary: concatenated notes
- steps: ordered execution units
  - id: incremental label
  - description: human readable intent
  - command: shell command (still parsed by normal shell pipeline)
  - confirm: requires explicit user consent prior to auto mode execution

## Built-in Usage

```sh
aio suggest list files
aio auto clean build folder
```

`suggest` prints plan only. `auto` prints plan then executes steps (asks confirmation if dangerous).

## Rule-Based Expansion

Current mapping examples:

- "list files" -> `ls -la`
- "find text" -> `grep -R 'TODO' .`
- "remove build" / "clean build" -> `rm -rf build`
  Fallback: echo the request.

## Danger Heuristics

Flag confirm=true if command contains any of:

- `rm `
- `sudo`
- `chmod 777`

## Configuration Keys (`~/.ai-autoshellrc`)

```
ai_enabled=true
planner_mode=suggest
llm_provider=openai
llm_model=gpt-4o-mini
llm_endpoint=https://api.openai.com/v1/chat/completions
llm_api_key_env=OPENAI_API_KEY
```

(LLM not invoked yet; placeholders for future integration.)

## Future Work

- Real LLM prompt assembly (summary + proposed steps) with deterministic formatting.
- Enhanced safety classifier (path patterns, wildcard deletes, network operations).
- Step dependency graph (parallelization opportunities).
- Rollback hints for destructive operations.
- Streaming plan refinement (interactive approval per step).

## Safety Principles

1. Explicit user consent for dangerous commands.
2. Structured plan separate from execution.
3. Minimal necessary information passed to model.
4. Avoid ambiguous wildcard expansions from model; shell still performs parsing.

---

End of AI planner MVP doc.
