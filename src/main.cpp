// AI-AutoShell main (versione pulita con completion colorata)
#include <ai-autoshell/lex/lexer.hpp>
#include <ai-autoshell/parse/ast.hpp>
#include <ai-autoshell/parse/tokens.hpp>
#include <ai-autoshell/parse/parser.hpp>
#include <ai-autoshell/expand/expand.hpp>
#include <ai-autoshell/exec/executor_posix.hpp>
#include <ai-autoshell/line/line_editor.hpp>

#include <algorithm>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <cstdio>
#include <sys/utsname.h>
#include <unistd.h>
namespace fs = std::filesystem;

// Mappa colori usata dal line editor per suggerimenti
namespace autoshell { std::map<std::string,std::string> g_completion_colors; }
static volatile sig_atomic_t g_interrupted = 0;
static volatile sig_atomic_t g_tstp = 0;
extern std::optional<pid_t> g_foreground_pgid; // definita in executor_posix.cpp

struct ShellConfig { std::string prompt_format = "{user}@{host} {cwd}$ "; bool color = true; };
static ShellConfig g_cfg;

static std::string getenv_or(const char* k, const std::string& def="") { const char* v = std::getenv(k); return v?std::string(v):def; }
static std::string apply_color(const std::string& s, const char* code){ if(!g_cfg.color) return s; return std::string("\x1b[")+code+"m"+s+"\x1b[0m"; }
static void load_config(){ std::string home=getenv_or("HOME"); if(home.empty()) return; std::ifstream in(home+"/.ai-autoshellrc"); if(!in) return; std::string line; while(std::getline(in,line)){ if(line.empty()||line[0]=='#') continue; auto eq=line.find('='); if(eq==std::string::npos) continue; auto key=line.substr(0,eq); auto val=line.substr(eq+1); if(key=="prompt_format") g_cfg.prompt_format=val; else if(key=="color") g_cfg.color=(val=="1"||val=="true"||val=="on"); } }
static void sigint_handler(int){ g_interrupted=1; }
static void sigtstp_handler(int){ g_tstp=1; if(g_foreground_pgid) kill(-*g_foreground_pgid,SIGTSTP); }
static std::string make_prompt(){
    std::string user=getenv_or("USER","user"), host=getenv_or("HOSTNAME","host"), cwd;
    try { cwd=fs::current_path().filename().string(); } catch(...) { cwd="?"; }
    std::string p=g_cfg.prompt_format;
    auto repl=[&](const std::string& tag,const std::string& val){ size_t pos=0; while((pos=p.find(tag,pos))!=std::string::npos){ p.replace(pos,tag.size(),val); pos+=val.size(); } };
    repl("{user}",apply_color(user,"32"));
    repl("{host}",apply_color(host,"34"));
    repl("{cwd}",apply_color(cwd,"33"));
    repl("{status}",apply_color(getenv_or("?","0"),"31"));
    return p;
}

int main(){
    std::signal(SIGINT,sigint_handler);
    std::signal(SIGTSTP,sigtstp_handler);
    load_config();
    struct utsname u; uname(&u);
    std::cout << "\n" << apply_color("AI-AutoShell","1;36") << " (MVP)\n";
    std::cout << "System: " << u.sysname << " " << u.release << " (" << u.machine << ")\n";
    std::cout << "Built-ins: cd pwd exit echo export unset jobs fg bg\n";
    std::cout << "Type 'exit' to quit.\n\n";
    int last_status = 0;
    std::vector<std::string> history;
    while (true) {
        g_interrupted = 0;
        autoshell::LineEditor editor;
        static std::string cached_path_value; static std::vector<std::string> cached_path_execs;
        auto refresh_path_cache = [&](){
            std::string path_val = getenv_or("PATH");
            if (path_val == cached_path_value) return;
            cached_path_value = path_val; cached_path_execs.clear();
            std::stringstream ss(path_val); std::string dir;
            while (std::getline(ss, dir, ':')) {
                if (dir.empty()) continue;
                std::error_code ec; fs::directory_iterator it(dir, ec), end;
                while (!ec && it!=end) {
                    std::error_code sec;
                    if (it->is_regular_file(sec) && !sec) {
                        auto p = it->path();
                        if (access(p.c_str(), X_OK)==0) cached_path_execs.push_back(p.filename().string());
                    }
                    ++it;
                }
            }
            std::sort(cached_path_execs.begin(), cached_path_execs.end());
            cached_path_execs.erase(std::unique(cached_path_execs.begin(), cached_path_execs.end()), cached_path_execs.end());
        };
        static std::unordered_map<std::string,std::vector<std::pair<std::string,bool>>> dir_cache;
        auto list_dir_cached = [&](const fs::path& p)->const std::vector<std::pair<std::string,bool>>& {
            std::string key = p.string();
            auto it = dir_cache.find(key);
            if (it!=dir_cache.end()) return it->second;
            std::vector<std::pair<std::string,bool>> entries;
            std::error_code ec; fs::directory_iterator dit(p, ec), end;
            while (!ec && dit!=end) {
                std::error_code sec; bool isdir = dit->is_directory(sec) && !sec;
                entries.emplace_back(dit->path().filename().string(), isdir);
                ++dit;
            }
            return dir_cache.emplace(key, std::move(entries)).first->second;
        };
        autoshell::CompletionOptions comp{ .provider = [&](const std::string& buffer,const std::string& prefix){
            std::vector<std::string> matches; autoshell::g_completion_colors.clear();
            std::string trimmed = buffer; while (!trimmed.empty() && std::isspace((unsigned char)trimmed.front())) trimmed.erase(trimmed.begin());
            std::istringstream iss(trimmed); std::string first; iss >> first;
            bool cd_mode = (first == "cd");
            if (cd_mode) {
                size_t slash_pos = prefix.find_last_of('/');
                std::string base = (slash_pos==std::string::npos)?"":prefix.substr(0,slash_pos+1);
                std::string name = (slash_pos==std::string::npos)?prefix:prefix.substr(slash_pos+1);
                fs::path resolved;
                if (base.empty()) resolved = fs::current_path();
                else if (base=="./"||base==".") resolved = fs::current_path();
                else if (base=="../"||base=="..") resolved = fs::current_path().parent_path();
                else if (base=="~/"||base=="~") resolved = fs::path(getenv_or("HOME"));
                else if (base.rfind("./",0)==0) resolved = fs::current_path()/base.substr(2);
                else if (base.rfind("../",0)==0) resolved = fs::current_path().parent_path()/base.substr(3);
                else if (!base.empty() && base.front()=='/') resolved = fs::path(base);
                else resolved = fs::current_path()/base;
                auto &entries = list_dir_cached(resolved);
                bool show_hidden = (!name.empty() && name[0]=='.');
                for (auto &e : entries) {
                    if (!e.second) continue;
                    if (!show_hidden && !name.empty() && e.first[0]=='.' && name[0] != '.') continue;
                    if (name.empty() || e.first.rfind(name,0)==0) {
                        std::string out = base + e.first + "/";
                        matches.push_back(out); autoshell::g_completion_colors[out] = "\033[34m";
                    }
                }
            } else {
                refresh_path_cache();
                static const char* builtins[] = {"cd","pwd","exit","echo","export","unset","jobs","fg","bg"};
                size_t first_space = buffer.find(' '); bool first_token = (first_space==std::string::npos || buffer.size()==first_space+1);
                if (first_token) {
                    for (auto b: builtins) if (std::string(b).rfind(prefix,0)==0) { matches.push_back(b); autoshell::g_completion_colors[b] = "\033[36m"; }
                    if (prefix.find('/')==std::string::npos) {
                        for (auto &n : cached_path_execs) if (n.rfind(prefix,0)==0) { matches.push_back(n); autoshell::g_completion_colors[n] = "\033[32m"; }
                    }
                } else {
                    size_t last_space = buffer.find_last_of(' ');
                    std::string file_prefix = (last_space==std::string::npos)?buffer:buffer.substr(last_space+1);
                    auto &entries = list_dir_cached(fs::current_path()); bool show_hidden = (!file_prefix.empty() && file_prefix[0]=='.');
                    for (auto &e : entries) {
                        if (!show_hidden && !file_prefix.empty() && e.first[0]=='.' && file_prefix[0] != '.') continue;
                        if (!file_prefix.empty() && e.first.rfind(file_prefix,0)==0) {
                            std::string out = e.first + (e.second?"/":"");
                            matches.push_back(out); autoshell::g_completion_colors[out] = e.second?"\033[34m":"";
                        }
                    }
                }
            }
            std::sort(matches.begin(), matches.end());
            matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
            return matches;
        } };
        std::string line = editor.read_line(make_prompt(), comp, history);
        if (line.empty()) {
            if (std::cin.eof()) break;
            if (g_interrupted) continue;
        }
        if (g_interrupted || g_tstp) {
            std::cout << "\n"; g_tstp = 0; continue;
        }
        auto notspace = [](int ch){ return !std::isspace(ch); };
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), notspace));
        line.erase(std::find_if(line.rbegin(), line.rend(), notspace).base(), line.end());
        if (line.empty()) continue;
        history.push_back(line);
        autoshell::Lexer lexer(line);
        auto token_stream = lexer.run();
        autoshell::AST ast = autoshell::parse_tokens(token_stream);
        static autoshell::ExecContext exec_ctx;
        autoshell::ExecutorPOSIX executor(exec_ctx);
        last_status = executor.run(ast);
        char buf[16]; std::snprintf(buf, sizeof(buf), "%d", last_status); setenv("?", buf, 1);
    }
    return last_status;
}