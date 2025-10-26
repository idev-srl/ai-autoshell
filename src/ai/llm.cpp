#include <ai-autoshell/ai/llm.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <cstdlib>
#include <optional>

namespace autoshell::ai {

static std::string trim(const std::string& s){ size_t a=0; while(a<s.size() && std::isspace((unsigned char)s[a])) ++a; size_t b=s.size(); while(b>a && std::isspace((unsigned char)s[b-1])) --b; return s.substr(a,b-a); }

std::optional<LLMCompletion> StubLLMClient::complete(const std::string& prompt) {
    // 1. If stub_file configured and readable: return its contents (or first line matching a prefix rule)
    if (!m_cfg.stub_file.empty()) {
        std::ifstream in(m_cfg.stub_file);
        if (in) {
            std::ostringstream oss; oss << in.rdbuf();
            std::string data = oss.str();
            if (!data.empty()) {
                return LLMCompletion{data, "stub_file", -1, -1, -1, -1.0, -1.0, -1.0};
            }
        }
    }
    // 2. If provider!=none and api_key_env present: placeholder remote call simulation
    if (m_cfg.enabled && m_cfg.provider != "none") {
        const char* key_env = nullptr;
        if (!m_cfg.api_key_env.empty()) key_env = std::getenv(m_cfg.api_key_env.c_str());
        bool have_key = (key_env && *key_env) || !m_cfg.api_key.empty();
        if (have_key) {
            std::string used_key = key_env && *key_env ? std::string("env:") + m_cfg.api_key_env : "direct";
            std::string simulated = "(LLM simulated response provider='" + m_cfg.provider + "' model='" + m_cfg.model + "' key=" + used_key + ")";
            return LLMCompletion{simulated, "simulated_remote", -1, -1, -1, -1.0, -1.0, -1.0};
        }
    }
    // 3. Fallback puro: restituisce il prompt (nessuna generazione di comandi)
    return LLMCompletion{prompt, "stub_plain", -1, -1, -1, -1.0, -1.0, -1.0};
}

} // namespace autoshell::ai
