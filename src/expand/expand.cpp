/*
 * AI-AutoShell Expansion Implementation
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 * Description: Implements ~, $VAR, ${VAR} expansions (no command substitution).
 */
#include <cstdlib>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <system_error>
#include <ai-autoshell/expand/expand.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>

namespace autoshell {

static std::string getenv_or(const std::string& key) {
    const char* v = std::getenv(key.c_str());
    return v ? std::string(v) : std::string();
}

static std::string expand_tilde(const std::string& s) {
    if (!s.empty() && s[0] == '~') {
        std::string home = getenv_or("HOME");
        if (!home.empty()) {
            if (s.size()==1) return home;
            if (s[1]=='/') return home + s.substr(1);
        }
    }
    return s;
}

static std::string expand_env_vars(const std::string& in) {
    std::string out; out.reserve(in.size());
    for (size_t i=0;i<in.size();) {
        if (in[i]=='$') {
            if (i+1 < in.size() && in[i+1]=='{') {
                size_t end = in.find('}', i+2);
                if (end != std::string::npos) {
                    std::string key = in.substr(i+2, end-(i+2));
                    out += getenv_or(key);
                    i = end+1; continue;
                }
            }
            size_t j=i+1;
            if (j < in.size() && (std::isalpha(static_cast<unsigned char>(in[j])) || in[j]=='_')) {
                ++j; while (j<in.size() && (std::isalnum(static_cast<unsigned char>(in[j])) || in[j]=='_')) ++j;
                std::string key = in.substr(i+1, j-(i+1));
                out += getenv_or(key);
                i=j; continue;
            }
        }
        out.push_back(in[i++]);
    }
    return out;
}

bool has_glob_chars(const std::string& s) {
    return s.find_first_of("*?[") != std::string::npos; // '[' start of char class
}

static std::string glob_to_regex(const std::string& pat) {
    std::string rx; rx.reserve(pat.size()*2);
    rx += '^';
    bool in_class=false;
    for (size_t i=0;i<pat.size();++i) {
        char c = pat[i];
        if (in_class) {
            if (c==']') { in_class=false; rx.push_back(c); }
            else rx.push_back(c);
            continue;
        }
        switch(c) {
            case '*': rx += ".*"; break;
            case '?': rx += '.'; break;
            case '[': in_class=true; rx.push_back('['); break;
            case '.': case '(': case ')': case '+': case '{': case '}': case '^': case '$': case '|': case '\\':
                rx.push_back('\\'); rx.push_back(c); break;
            default: rx.push_back(c); break;
        }
    }
    rx += '$';
    return rx;
}

static std::vector<std::string> run_glob(const std::string& pattern) {
    // Only support patterns without directory separators for MVP; otherwise return literal
    if (pattern.find('/') != std::string::npos) return {pattern};
    std::string rx = glob_to_regex(pattern);
    std::regex re(rx);
    std::vector<std::string> matches;
    std::error_code ec;
    for (auto it = std::filesystem::directory_iterator(std::filesystem::current_path(), ec);
         !ec && it != std::filesystem::directory_iterator(); ++it) {
        const std::string name = it->path().filename().string();
        if (std::regex_match(name, re)) matches.push_back(name);
    }
    if (matches.empty()) return {pattern};
    std::sort(matches.begin(), matches.end());
    return matches;
}

static std::string command_substitute(const std::string& body) {
    // Esegue body come comando shell semplice: usa /bin/sh -c per MVP
    int pipefd[2];
    if (pipe(pipefd) != 0) return "";
    pid_t pid = fork();
    if (pid < 0) { close(pipefd[0]); close(pipefd[1]); return ""; }
    if (pid == 0) {
        // child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO); // semplificazione
        execl("/bin/sh", "sh", "-c", body.c_str(), (char*)nullptr);
        _exit(127);
    }
    close(pipefd[1]);
    std::string output; char buf[256]; ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) output.append(buf, buf+n);
    close(pipefd[0]);
    int st=0; while (waitpid(pid,&st,0)<0 && errno==EINTR) {}
    // Trim trailing newlines
    while (!output.empty() && (output.back()=='\n' || output.back()=='\r')) output.pop_back();
    return output;
}

std::string expand_word(const std::string& in) {
    // Gestione semplice di una singola sostituzione $(...) non annidata
    std::string tmp = expand_tilde(in);
    tmp = expand_env_vars(tmp);
    size_t pos = tmp.find("$(");
    if (pos != std::string::npos) {
        size_t end = tmp.find(')', pos+2);
        if (end != std::string::npos) {
            std::string body = tmp.substr(pos+2, end-(pos+2));
            std::string sub = command_substitute(body);
            tmp = tmp.substr(0,pos) + sub + tmp.substr(end+1);
        }
    }
    return tmp;
}

std::vector<std::string> expand_words(const std::vector<std::string>& words) {
    std::vector<std::string> out; out.reserve(words.size());
    for (auto &w : words) {
        std::string base = expand_word(w);
        if (has_glob_chars(base)) {
            auto globbed = run_glob(base);
            out.insert(out.end(), globbed.begin(), globbed.end());
        } else {
            out.push_back(base);
        }
    }
    return out;
}

} // namespace autoshell
