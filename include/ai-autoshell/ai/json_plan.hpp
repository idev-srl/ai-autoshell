#pragma once
#include <string>
#include <vector>
#include <optional>

namespace autoshell::ai {
struct ParsedStep { std::string id; std::string description; std::string command; bool confirm=false; };
struct ParsedPlan { std::string request; std::vector<ParsedStep> steps; bool valid=false; };

// Parse minimal JSON produced by LLM (assumes UTF-8, no nested objects except steps array).
ParsedPlan parse_plan_json(const std::string& json);
}
