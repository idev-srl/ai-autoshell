#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <memory>

namespace autoshell::ai {

struct LLMConfig {
    bool enabled = false;            // llm_enabled
    std::string provider = "none";  // openai, azure, ollama, etc.
    std::string model;               // model id
    std::string endpoint;            // HTTP endpoint (if remote)
    std::string api_key_env;         // env var containing key
    std::string api_key;             // direct key (less secure; prefer env)
    std::string stub_file;           // local file with canned responses (for offline)
    int max_tokens = 512;
    double temperature = 0.2;
    int timeout_seconds = 20;        // network timeout
    double prompt_price_per_1k = 0.0;    // USD cost per 1K prompt tokens (for cost estimation)
    double completion_price_per_1k = 0.0; // USD cost per 1K completion tokens
};

// Response from LLM completion.
struct LLMCompletion {
    std::string text;                // raw model text
    std::string source;              // stub|env|remote|echo
    int prompt_tokens = -1;          // number of tokens in prompt (if reported)
    int completion_tokens = -1;      // number of tokens in completion
    int total_tokens = -1;           // total usage (prompt+completion)
    double prompt_cost = -1.0;       // estimated USD cost for prompt part
    double completion_cost = -1.0;   // estimated USD cost for completion part
    double total_cost = -1.0;        // sum
};

class LLMClient {
public:
    virtual ~LLMClient() = default;
    virtual std::optional<LLMCompletion> complete(const std::string& prompt) = 0;
};

// Stub implementation: if stub_file set, returns first matching line (or whole file); otherwise echoes prompt.
class StubLLMClient : public LLMClient {
public:
    explicit StubLLMClient(const LLMConfig& cfg) : m_cfg(cfg) {}
    std::optional<LLMCompletion> complete(const std::string& prompt) override;
private:
    LLMConfig m_cfg;
};

// OpenAI client (Chat Completions minimal). Richiede libcurl.
class OpenAILLMClient : public LLMClient {
public:
    explicit OpenAILLMClient(const LLMConfig& cfg) : m_cfg(cfg) {}
    std::optional<LLMCompletion> complete(const std::string& prompt) override;
private:
    LLMConfig m_cfg;
};

// Factory helper (for future provider switch)
// Forward for extended factory (defined in llm_ollama.cpp)
std::unique_ptr<LLMClient> make_llm_provider_extended(const LLMConfig&);
// Extended providers supported: openai, ollama, claude, gemini

inline std::unique_ptr<LLMClient> make_llm(const LLMConfig& cfg) {
    return make_llm_provider_extended(cfg);
}

} // namespace autoshell::ai
