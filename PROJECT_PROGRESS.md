# ai-autoshell Project Status

Date: 27 October 2025

## Core Project Goals

Interactive LLM-assisted shell that:

- Suggests commands (`ai suggest`)
- Executes commands automatically (`ai auto`) with confirmation
- Supports multiple LLM providers (cloud and local)
- Shows token usage and estimated cost
- Allows dynamic configuration (pricing, provider, model) via rc file and runtime commands

## Completed Work

1. Core shell architecture (lexer, parser, expander, executor, jobs, glob, subshell; heuristic planner removed)
2. OpenAI integration (chat/completions) with text extraction
3. Added pricing fields and estimated cost calculation (prompt, completion, total) in AI output
4. Pricing parsed from rc file + runtime command `ai pricing <prompt> <completion>`
5. Display of token usage and costs (when available) after AI commands
6. Local provider Ollama added (simple HTTP call, configurable model)
7. Claude provider (Anthropic /v1/messages) with text + usage parsing (input/output tokens)
8. Gemini provider (Google generativelanguage) with content + usageMetadata parsing
9. Unified multiple LLM implementations into single file `llm_ollama.cpp` to reduce linkage problems
10. Extended factory `make_llm` for dynamic provider selection: `openai | ollama | claude | gemini`
11. Fallback handling when tokens/cost unavailable (informative message)
12. Banner command shows pricing if set
13. Updated CMake build for new sources and consolidation
14. Removed unused heuristic planner
15. Threading and safe capture of AI results with cost calculations

## Technical Improvements Implemented

- Lightweight manual parsing to avoid heavy JSON dependencies
- Cost calculation based on per-1K token pricing
- Code consolidation to avoid incomplete type errors

## Remaining Work

1. Robust JSON parsing (introduce lightweight lib, e.g. `nlohmann::json`) to reduce brittle string-searching
2. Streaming output (progressive chunks) for providers that support it (OpenAI, Claude, Ollama)
3. Advanced error handling with codes and retry/backoff
4. Suggestion caching and deduplication
5. "Dry-run" mode for `ai auto` (show execution plan before running)
6. Additional configurable parameters: temperature, max_tokens, top_p, top_k (per provider where applicable)
7. Secure auth: env vars, macOS keychain support
8. Structured logging (levels: info, warn, error) + `--verbose` flag
9. Unit tests for LLM response parsing (mock JSON payloads)
10. Response time benchmarking per provider + simple performance profile
11. Expanded README with per-provider examples
12. Prompt input validation (length limits, JSON-safe escaping)
13. Multi-message conversation history instead of single prompt
14. Local rate limiting to avoid exceeding quotas
15. Session aggregate metrics: cumulative tokens/cost
16. Refactor duplicated logic (JSON escaping, HTTP headers, token usage parsing) into shared utilities
17. Offline mode with local model fallback when no provider reachable
18. Packaging / release (binary + install instructions) and CI (build + tests)
19. Session cost cap to abort if estimated cost threshold exceeded
20. Separate default model configuration per provider
21. Internationalization of messages (currently mixed Italian/English) and language selection
22. Safety filtering of potentially destructive generated commands (`rm -rf`, etc.)
23. Token vs cost analysis to recommend cheapest provider dynamically
24. "Explain" mode to accompany suggested command with explanation
25. Future provider integrations (e.g. Mistral, DeepSeek, direct local `llama.cpp`)

## Priority (Next 5 Steps)

1. Common refactor: parsing + JSON escaping utilities
2. Introduce lightweight JSON library and replace manual parsing
3. Configurable parameters (temperature, max_tokens)
4. Unit tests for LLM response parsing (mock payloads)
5. Expanded README with provider examples

## Dependencies / Considerations

- Adding JSON library requires CMake update (header-only include)
- Streaming requires differential CURL callback handling
- Some APIs (Gemini) need specific safety parameters documented

## Final Note

Core works; extensibility is in place. Missing: robust parsing, usability features, and production-quality hardening.

---

Update this file as steps are completed.
