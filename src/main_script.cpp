/*
 * AI-AutoShell Script Runner (.ash)
 * Minimal implementation: reads a .ash file line by line, skips comments (#...) and blank lines,
 * runs each line through lexer->parser->executor.
 */
#include <ai-autoshell/lex/lexer.hpp>
#include <ai-autoshell/parse/parser.hpp>
#include <ai-autoshell/exec/executor_posix.hpp>
#include <ai-autoshell/expand/expand.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <csignal>

using namespace autoshell;

static volatile sig_atomic_t g_stop = 0;
void sigint_handler(int){ g_stop = 1; }

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ai-autoshell-script <file.ash>" << std::endl;
        return 1;
    }
    std::string path = argv[1];
    std::ifstream in(path);
    if (!in) { std::perror("open script"); return 1; }

    std::signal(SIGINT, sigint_handler);

    ExecContext ctx;
    ExecutorPOSIX executor(ctx);
    int last_status = 0;

    std::string line; size_t lineno=0;
    while (std::getline(in, line)) {
        ++lineno;
        if (g_stop) { std::cerr << "Interrupted" << std::endl; break; }
        // Trim
        auto notspace = [](int ch){ return !std::isspace(ch); };
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), notspace));
        line.erase(std::find_if(line.rbegin(), line.rend(), notspace).base(), line.end());
        if (line.empty()) continue;
        if (line[0]=='#') continue; // comment
        Lexer lex(line);
        auto tokens = lex.run();
        AST ast = parse_tokens(tokens);
        last_status = executor.run(ast);
        if (last_status != 0) {
            std::cerr << "Line " << lineno << " exit status " << last_status << std::endl;
        }
    }

    return last_status;
}
