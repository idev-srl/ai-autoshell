// AI planner interface (initial stub)
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace autoshell::ai {

// A single step in an execution plan.
struct PlanStep {
    std::string id;              // unique step id (e.g. s1)
    std::string description;     // human description
    std::string command;         // shell command (argv joined) to execute
    bool confirm = false;        // require explicit user confirmation due to risk
};

// Full plan produced by planner.
struct Plan {
    std::string request;               // original natural language request
    std::vector<PlanStep> steps;       // ordered steps
    std::string risk_summary;          // aggregated risk notes
    bool dangerous = false;            // any step flagged confirm
};

// Simple serialization to JSON (hand-written, minimal, no external lib).
inline std::string to_json(const Plan& p) {
    std::string out = "{\n";
    out += "  \"request\": \""; // naive escaping (TODO improve)
    for (char c : p.request) { if (c=='"') out += "\\\""; else out += c; }
    out += "\",\n";
    out += "  \"dangerous\": "; out += (p.dangerous?"true":"false"); out += ",\n";
    out += "  \"risk_summary\": \""; for (char c: p.risk_summary){ if(c=='"') out+="\\\""; else out+=c; } out += "\",\n";
    out += "  \"steps\": [\n";
    for (size_t i=0;i<p.steps.size();++i) {
        auto &s = p.steps[i];
        out += "    { \"id\": \"" + s.id + "\", \"description\": \"";
        for(char c: s.description){ if(c=='"') out+="\\\""; else out+=c; }
        out += "\", \"command\": \"";
        for(char c: s.command){ if(c=='"') out+="\\\""; else out+=c; }
        out += "\", \"confirm\": "; out += (s.confirm?"true":"false"); out += " }";
        if (i+1<p.steps.size()) out += ",";
        out += "\n";
    }
    out += "  ]\n";
    out += "}\n";
    return out;
}

// Planner configuration derived from shell config file.
struct PlannerConfig {
    bool enabled = false;              // ai_enabled
    std::string mode = "suggest";      // planner_mode: suggest|auto
    std::string provider = "none";     // llm_provider
    std::string model = "";            // llm_model
    std::string endpoint = "";         // llm_endpoint
    std::string api_key_env = "";      // llm_api_key_env
    int max_tokens = 0;                 // llm_max_tokens
    double temperature = 0.0;           // llm_temperature
};

// Rule-based stub planner for MVP.
class Planner {
public:
    explicit Planner(const PlannerConfig& cfg) : m_cfg(cfg) {}

    Plan plan(const std::string& request) const;
private:
    PlannerConfig m_cfg;
    std::vector<PlanStep> rule_expand(const std::string& request) const;
    bool is_dangerous(const std::string& cmd) const;
};

} // namespace autoshell::ai
