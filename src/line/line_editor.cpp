#include <map>
extern std::map<std::string, std::string> g_completion_colors;
/*
 * Line editor implementation (minimal)
 */
#include <ai-autoshell/line/line_editor.hpp>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <iostream>
#include <algorithm>

namespace autoshell {

static struct termios g_orig;

LineEditor::LineEditor() {}
LineEditor::~LineEditor() { if (m_raw) disable_raw(); }

void LineEditor::enable_raw() {
    if (m_raw) return;
    struct termios t; tcgetattr(STDIN_FILENO, &t); g_orig = t;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1; t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
    m_raw = true;
}
void LineEditor::disable_raw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig); m_raw=false;
}

int LineEditor::read_key() {
    unsigned char c; if (read(STDIN_FILENO, &c, 1) != 1) return -1; return c;
}

void LineEditor::write(const std::string& s) { ::write(STDOUT_FILENO, s.c_str(), s.size()); }

std::string LineEditor::read_line(const std::string& prompt,
                                  const CompletionOptions& comp,
                                  const std::vector<std::string>& history) {
    enable_raw();
    write(prompt);
    std::string buf; size_t hist_index = history.size(); // one past last
    bool last_was_tab = false;
    while (true) {
        int k = read_key();
        if (k == -1) { disable_raw(); return ""; }
        if (k == '\n') { write("\n"); disable_raw(); return buf; }
        if (k == 3) { // Ctrl-C
            write("^C\n"); disable_raw(); return ""; }
        if (k == 4) { // Ctrl-D
            disable_raw(); return std::string(); }
        if (k == 127 || k == 8) { // Backspace
            if (!buf.empty()) { buf.pop_back(); write("\b \b"); }
            last_was_tab = false;
            continue;
        }
        if (k == '\t') { // Tab completion
            // Trova ultimo token prefix
            size_t start = buf.find_last_of(' ');
            size_t token_pos = (start==std::string::npos)?0:start+1;
            std::string prefix = buf.substr(token_pos);
            if (comp.provider) {
                auto matches = comp.provider(buf, prefix);
                if (matches.empty()) continue;
                // Calcola common prefix
                std::string common = matches[0];
                for (auto &m : matches) {
                    size_t j=0; while (j<common.size() && j<m.size() && common[j]==m[j]) ++j;
                    common.resize(j);
                }
                if (matches.size()==1) {
                    std::string add = matches[0].substr(prefix.size());
                    buf += add; write(add);
                    // Se directory aggiungi '/' se non presente
                    if (matches[0].size() && matches[0].back()!='/' ) {
                        // Heuristic directory check: se esiste come dir nel cwd
                        if (access(matches[0].c_str(), F_OK)==0) {
                            // Try stat
                            struct stat st; if (stat(matches[0].c_str(), &st)==0 && S_ISDIR(st.st_mode)) {
                                buf.push_back('/'); write("/");
                            } else {
                                buf.push_back(' '); write(" ");
                            }
                        } else {
                            buf.push_back(' '); write(" ");
                        }
                    }
                    last_was_tab = false; // completamento definitivo
                } else if (common.size()>prefix.size()) {
                    std::string add = common.substr(prefix.size()); buf += add; write(add);
                    last_was_tab = false; // avanzato prefisso
                } else {
                    if (last_was_tab) {
                        write("\n");
                        int col=0;
                        extern std::map<std::string, std::string> g_completion_colors;
                        for (auto &m : matches) {
                            auto it = g_completion_colors.find(m);
                            if (it != g_completion_colors.end() && !it->second.empty()) {
                                write(it->second); // codice colore ANSI
                                write(m);
                                write("\033[0m"); // reset colore
                            } else {
                                write(m);
                            }
                            write("  ");
                            if(++col%4==0) write("\n");
                        }
                        if (col%4!=0) write("\n");
                        write(prompt); write(buf);
                        last_was_tab = false;
                    } else {
                        last_was_tab = true; // prossimo Tab mostra lista
                    }
                }
            }
            continue;
        }
        if (k == 27) { // ESC sequence
            int k1 = read_key(); int k2 = read_key();
            if (k1=='[') {
                if (k2=='A') { // Up
                    if (!history.empty() && hist_index>0) {
                        hist_index--; buf = history[hist_index];
                        write("\r"); write(std::string( prompt.size()+buf.size(),' ')); // clear line crude
                        write("\r"); write(prompt); write(buf);
                    }
                } else if (k2=='B') { // Down
                    if (!history.empty() && hist_index < history.size()) {
                        hist_index++;
                        if (hist_index==history.size()) buf.clear(); else buf = history[hist_index];
                        write("\r"); write(std::string( prompt.size()+buf.size(),' '));
                        write("\r"); write(prompt); write(buf);
                    }
                }
            }
            last_was_tab = false;
            continue;
        }
        // Normal printable
        if (k >= 32 && k < 127) {
            buf.push_back(static_cast<char>(k));
            write(std::string(1, static_cast<char>(k)));
            last_was_tab = false;
        }
    }
}

} // namespace autoshell
