/*
 * Line editor minimal - AI-AutoShell
 * Supporta: inserimento caratteri, Backspace, Enter, frecce Up/Down history, Tab completion MVP.
 */
#pragma once
#include <string>
#include <vector>
#include <functional>

namespace autoshell {

struct CompletionOptions {
    // provider riceve buffer completo corrente e prefisso token da completare
    std::function<std::vector<std::string>(const std::string& buffer,const std::string& prefix)> provider;
};

class LineEditor {
public:
    LineEditor();
    ~LineEditor();
    std::string read_line(const std::string& prompt,
                          const CompletionOptions& comp,
                          const std::vector<std::string>& history);
private:
    bool m_raw = false;
    void enable_raw();
    void disable_raw();
    int read_key();
    void write(const std::string& s);
};

} // namespace autoshell
