// AI-AutoShell main (clean version with colored completion)
#include <ai-autoshell/lex/lexer.hpp>
#include <ai-autoshell/parse/ast.hpp>
#include <ai-autoshell/parse/tokens.hpp>
#include <ai-autoshell/parse/parser.hpp>
#include <ai-autoshell/expand/expand.hpp>
#include <ai-autoshell/exec/executor_posix.hpp>
#include <ai-autoshell/line/line_editor.hpp>
#include <ai-autoshell/ai/llm.hpp>
#include <ai-autoshell/ai/planner.hpp> // only for Plan/PlanStep structs and to_json; no rule usage
#include <ai-autoshell/ai/json_plan.hpp>

#include <algorithm>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
// <optional> not required after removal of foreground pgid
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <cstdio>
#include <iomanip>
#ifndef _WIN32
#include <sys/utsname.h>
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <thread>
#include <atomic>
#include <chrono>
#include <regex>
namespace fs = std::filesystem;

// Completion colors map defined in line_editor.cpp
namespace autoshell { extern std::map<std::string,std::string> g_completion_colors; }
static volatile sig_atomic_t g_interrupted = 0;
static volatile sig_atomic_t g_tstp = 0;
// TODO: foreground job tracking disabled in this simplified build

struct ShellConfig {
    std::string prompt_format = "{user}@{host} {cwd}$ ";
    bool color = true;
    // AI / planner configuration
    bool ai_enabled = false;
    bool ai_debug = false; // show full JSON and details only if true
    std::string planner_mode = "suggest"; // suggest|auto
    std::string llm_provider = "none";
    std::string llm_model;
    std::string llm_endpoint;
    std::string llm_api_key_env;
    bool llm_enabled = true; // enabled by default
    std::string llm_stub_file; // stub responses path
    std::string llm_api_key; // direct key (NOT recommended; prefer env)
    bool llm_spinner = true; // show LLM progress spinner
    double llm_prompt_price_per_1k = 0.0; // USD per 1K prompt tokens
    double llm_completion_price_per_1k = 0.0; // USD per 1K completion tokens
};
static ShellConfig g_cfg;

static std::string getenv_or(const char* k, const std::string& def="") { const char* v = std::getenv(k); return v?std::string(v):def; }
static std::string apply_color(const std::string& s, const char* code){ if(!g_cfg.color) return s; return std::string("\x1b[")+code+"m"+s+"\x1b[0m"; }
static void load_config(){
    std::string home=getenv_or("HOME"); if(home.empty()) return; std::ifstream in(home+"/.ai-autoshellrc"); if(!in) return; std::string line; while(std::getline(in,line)){
        if(line.empty()||line[0]=='#') continue; auto eq=line.find('='); if(eq==std::string::npos) continue; auto key=line.substr(0,eq); auto val=line.substr(eq+1);
        if(key=="prompt_format") g_cfg.prompt_format=val;
        else if(key=="color") g_cfg.color=(val=="1"||val=="true"||val=="on");
        else if(key=="ai_enabled") g_cfg.ai_enabled=(val=="1"||val=="true"||val=="on");
        else if(key=="planner_mode") g_cfg.planner_mode=val;
        else if(key=="llm_provider") g_cfg.llm_provider=val;
        else if(key=="llm_model") g_cfg.llm_model=val;
        else if(key=="llm_endpoint") g_cfg.llm_endpoint=val;
    else if(key=="llm_api_key_env") g_cfg.llm_api_key_env=val;
    else if(key=="llm_api_key") g_cfg.llm_api_key=val;
    else if(key=="llm_enabled") g_cfg.llm_enabled=(val=="1"||val=="true"||val=="on");
    else if(key=="llm_stub_file") g_cfg.llm_stub_file=val;
    else if(key=="llm_spinner") g_cfg.llm_spinner=(val=="1"||val=="true"||val=="on");
    else if(key=="llm_prompt_price_per_1k") { try { g_cfg.llm_prompt_price_per_1k = std::stod(val); } catch(...) {} }
    else if(key=="llm_completion_price_per_1k") { try { g_cfg.llm_completion_price_per_1k = std::stod(val); } catch(...) {} }
    }
}
static void sigint_handler(int){ g_interrupted=1; }
static void sigtstp_handler(int){ g_tstp=1; /* foreground pgid non gestito */ }
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

int main(int argc,char* argv[]){
    std::signal(SIGINT,sigint_handler);
    std::signal(SIGTSTP,sigtstp_handler);
    load_config();
    for(int i=1;i<argc;++i){ std::string a=argv[i]; if(a=="--ai-debug"||a=="-d") g_cfg.ai_debug=true; }
    std::cout << "\n" << apply_color("AI-AutoShell","1;36") << " (MVP)\n";
#ifndef _WIN32
    struct utsname u; uname(&u);
    std::cout << "System: " << u.sysname << " " << u.release << " (" << u.machine << ")\n";
#else
    OSVERSIONINFOEX info{}; info.dwOSVersionInfoSize=sizeof(info); GetVersionEx((OSVERSIONINFO*)&info);
    std::cout << "System: Windows " << info.dwMajorVersion << "." << info.dwMinorVersion << " (build " << info.dwBuildNumber << ")\n";
#endif
    std::cout << "Built-ins: cd pwd exit echo export unset jobs fg bg ai\n";
    // AI / LLM status banner
    if (g_cfg.ai_enabled) {
        std::cout << "AI LLM mode active (model-generated steps only)";
        if (!g_cfg.llm_provider.empty()) std::cout << " | provider=" << g_cfg.llm_provider;
        if (!g_cfg.llm_model.empty()) std::cout << " | model=" << g_cfg.llm_model;
        if (g_cfg.llm_prompt_price_per_1k>0 || g_cfg.llm_completion_price_per_1k>0) {
            std::cout << " | pricing=$" << std::fixed << std::setprecision(4) << g_cfg.llm_prompt_price_per_1k << "/1K(prompt),$" << g_cfg.llm_completion_price_per_1k << "/1K(completion)";
        }
        std::cout << "\nUsage: ai suggest <request> | ai auto <request>";
        if(g_cfg.ai_debug) std::cout << " (debug JSON ON)";
        std::cout << "\n";
    } else {
        std::cout << "AI disabled. Enable by adding 'ai_enabled=true' to ~/.ai-autoshellrc\n";
    }
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
        // AI command intercept (not tokenized like shell grammar yet)
        if (g_cfg.ai_enabled) {
            std::string tmp=line; while(!tmp.empty() && std::isspace((unsigned char)tmp.front())) tmp.erase(tmp.begin());
            if (tmp.rfind("ai ",0)==0) {
                // Format: ai suggest <request> | ai auto <request>
                std::istringstream iss(tmp); std::string ai_kw, mode_kw; iss>>ai_kw>>mode_kw; std::string request; std::getline(iss, request); if(!request.empty() && request.front()==' ') request.erase(request.begin());
                if(mode_kw=="suggest"||mode_kw=="auto") {
                    // Flusso puro LLM (nessun planner locale)
                    autoshell::ai::Plan plan; plan.request = request;
                    if(!g_cfg.llm_enabled){ std::cout << "LLM disabled: cannot generate the plan.\n"; last_status=1; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue; }
                    static std::unordered_map<std::string,std::string> g_plan_cache; auto normalize_req=[&](std::string r){ std::transform(r.begin(),r.end(),r.begin(),[](unsigned char c){ return std::tolower(c); }); return r; };
                    autoshell::ai::LLMConfig lc; lc.enabled=true; lc.provider=g_cfg.llm_provider; lc.model=g_cfg.llm_model; lc.endpoint=g_cfg.llm_endpoint; lc.api_key_env=g_cfg.llm_api_key_env; lc.api_key=g_cfg.llm_api_key; lc.stub_file=g_cfg.llm_stub_file; lc.max_tokens=512; lc.temperature=0.2; lc.timeout_seconds=25; lc.prompt_price_per_1k=g_cfg.llm_prompt_price_per_1k; lc.completion_price_per_1k=g_cfg.llm_completion_price_per_1k;
                    auto client_full = autoshell::ai::make_llm(lc);
                    if(!client_full){ std::cout << "[AI] LLM unavailable (missing provider/key).\n"; last_status=1; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue; }
                    std::string norm=normalize_req(request); std::string llm_text; bool from_cache=false;
                    auto cit=g_plan_cache.find(norm); if(cit!=g_plan_cache.end()){ llm_text=cit->second; from_cache=true; if(g_cfg.ai_debug) std::cout << "[DEBUG] Cache hit\n"; }
                    static std::string llm_source; // mantiene ultimo source
                    if(llm_text.empty()){ std::atomic<bool> done{false}; bool aborted=false; llm_source.clear(); static int usage_prompt=-1, usage_completion=-1, usage_total=-1; static double cost_prompt=-1.0, cost_completion=-1.0, cost_total=-1.0; if(g_cfg.ai_debug){ std::cout << "[DEBUG] LLM config provider="<<lc.provider<<" model="<<lc.model<<" endpoint="<<(lc.endpoint.empty()?"<default>":lc.endpoint)<<" key_present="<<(!lc.api_key.empty()||!lc.api_key_env.empty())<<"\n"; }
                        std::thread worker([&]{ std::string prompt_full="You are a shell assistant. Output ONLY pure JSON with {request, steps:[{id,description,command,confirm}]} and no extra text. Request: "+request; auto r=client_full->complete(prompt_full); if(r){ llm_text=r->text; llm_source=r->source; usage_prompt=r->prompt_tokens; usage_completion=r->completion_tokens; usage_total=r->total_tokens; cost_prompt=r->prompt_cost; cost_completion=r->completion_cost; cost_total=r->total_cost; } done=true; }); std::cout << "LLM planning"; if(g_cfg.llm_spinner) std::cout << "..."; std::cout.flush(); auto start=std::chrono::steady_clock::now(); int max_frames=lc.timeout_seconds*(1000/120); for(int f=0; f<max_frames && !done; ++f){ if(g_interrupted){ std::cout << "\n[AI] Interrupted by user.\n"; aborted=true; break; } auto el=std::chrono::steady_clock::now()-start; if(std::chrono::duration_cast<std::chrono::seconds>(el).count()>=lc.timeout_seconds){ std::cout << "\n[AI] LLM timeout after "<<lc.timeout_seconds<<"s.\n"; aborted=true; break; } if(g_cfg.llm_spinner && f % (1000/120)==0) std::cout << "." << std::flush; std::this_thread::sleep_for(std::chrono::milliseconds(120)); } worker.join(); std::cout << "\n"; if(!aborted && !llm_text.empty() && llm_source=="openai" && llm_text.rfind("(parse-empty)",0)!=0 && llm_text.rfind("(openai error",0)!=0) { g_plan_cache[norm]=llm_text; }
                        if(g_cfg.ai_debug){
                            std::cout << "[AI] Tokens: prompt="<<usage_prompt<<" completion="<<usage_completion<<" total="<<usage_total;
                            if(cost_total>=0.0){ std::cout << " | cost est. $"<<std::fixed<<std::setprecision(6)<<cost_total<<" (prompt $"<<cost_prompt<<" + completion $"<<cost_completion<<")"; }
                            else if(usage_total>=0 && (lc.prompt_price_per_1k<=0 || lc.completion_price_per_1k<=0)) { std::cout << " | pricing not configured"; }
                            std::cout << "\n";
                        }
                        else if(llm_source=="openai" && usage_total>=0){
                            std::cout << "[AI] Token usage: total="<<usage_total<<" (prompt="<<usage_prompt<<", completion="<<usage_completion<<")";
                            if(cost_total>=0.0){ std::cout << " | est. cost $"<<std::fixed<<std::setprecision(6)<<cost_total; }
                            else if(lc.prompt_price_per_1k<=0 || lc.completion_price_per_1k<=0){ std::cout << " | cost unavailable (pricing prompt="<<lc.prompt_price_per_1k<<" completion="<<lc.completion_price_per_1k<<")"; }
                            std::cout << "\n";
                        }
                    }
                    if(g_cfg.ai_debug || llm_text.empty() || llm_text.rfind("(openai error",0)==0 || llm_text.rfind("(parse-empty)",0)==0 || llm_text.rfind("(no-key)",0)==0 || llm_text.rfind("(curl-init-fail)",0)==0){ std::cout << "[AI] Source="<<(llm_source.empty()?"<none>":llm_source)<<"\n"; }
                    if(!llm_text.empty() && llm_source!="openai" && g_cfg.ai_debug){ std::cout << "[DEBUG] Source="<<llm_source<<"\n"; }
                    // Distinzione errori
                    if(llm_text.rfind("(openai error",0)==0){ std::cout << "[AI] OpenAI HTTP error: "<<llm_text<<"\n"; }
                    else if(llm_text.rfind("(parse-empty)",0)==0){ std::cout << "[AI] Parse error: content field missing.\n"; }
                    else if(llm_text.rfind("(env-missing:",0)==0){
                        std::string var = g_cfg.llm_api_key_env;
                        // If the variable name looks like a key (starts with sk-) warn
                        if(var.rfind("sk-",0)==0){ std::string masked = var.substr(0,10) + "..."; std::cout << "[AI] Misconfigured variable name (looks like a key). Use llm_api_key_env=OPENAI_API_KEY and export OPENAI_API_KEY=sk-***. Found: "<<masked<<"\n"; }
                        else { std::cout << "[AI] API key variable not found. Export first: export "<<var<<"=sk-...\n"; }
                    }
                    else if(llm_text.rfind("(env-empty:",0)==0){ std::cout << "[AI] Empty API key variable. Set it: export "<<g_cfg.llm_api_key_env<<"=sk-...\n"; }
                    else if(llm_text.rfind("(no-key-direct)",0)==0){ std::cout << "[AI] No key configured. Add llm_api_key_env=VAR or llm_api_key=sk-... to ~/.ai-autoshellrc\n"; }
                    // Mostra sempre risposta grezza LLM
                    if(g_cfg.ai_debug) std::cout << "[DEBUG] Raw LLM response:\n" << (llm_text.empty()?"<empty>":llm_text) << "\n";
                    else if(llm_text.empty()) {
                        std::cout << "[AI] Empty LLM response (source="<<(llm_source.empty()?"?":llm_source)<<"). Use -d for debug. Possible cause: missing key or unavailable model.\n";
                    }
                    if(!g_cfg.ai_debug && !llm_text.empty() && llm_text.front()!='{' && llm_text.rfind("(openai error",0)!=0 && llm_text.rfind("(parse-empty)",0)!=0){
                        std::string snippet = llm_text.substr(0, std::min<size_t>(llm_text.size(), 400));
                        std::cout << "[AI] Non-JSON response snippet:\n"<<snippet<<"\n";
                    }
                    auto clean=[&](std::string t){ if(t.rfind("```",0)==0){ size_t pos=t.find("```",3); if(pos!=std::string::npos) t=t.substr(3,pos-3); } return t; };
                    std::string jt=clean(llm_text); auto parsed=autoshell::ai::parse_plan_json(jt);
                    if(parsed.valid && !parsed.steps.empty()){ std::vector<autoshell::ai::PlanStep> new_steps; bool dangerous=false; int auto_id=1; for(auto &st: parsed.steps){ autoshell::ai::PlanStep ps; ps.id=st.id.empty()?"s"+std::to_string(auto_id++):st.id; ps.description=st.description.empty()?"LLM step":st.description; ps.command=st.command; std::string low=ps.command; std::transform(low.begin(),low.end(),low.begin(),::tolower); ps.confirm=st.confirm || (low.find("rm ")!=std::string::npos||low.find("sudo")!=std::string::npos||low.find("chmod 777")!=std::string::npos||low.find("chown")!=std::string::npos||low.find("dd ")!=std::string::npos||low.find("mkfs")!=std::string::npos|| (low.find("curl")!=std::string::npos && low.find("| sh")!=std::string::npos)); if(ps.confirm) dangerous=true; new_steps.push_back(ps);} plan.steps=new_steps; plan.dangerous=dangerous; if(g_cfg.ai_debug){ std::cout << "[DEBUG] Parsed LLM JSON steps="<<new_steps.size()<<(from_cache?" (cache)":"")<<"\n"; for(auto &s: new_steps){ std::cout << "  * "<<s.id<<" confirm="<<(s.confirm?"true":"false")<<" cmd="<<s.command<<"\n"; } }
                    } else { std::cout << "[AI] Unparseable response / no steps:\n" << llm_text << "\n"; last_status=1; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue; }
                    if(g_cfg.ai_debug){ std::cout << autoshell::ai::to_json(plan); if(mode_kw=="suggest"){ std::cout << "(suggest mode: not executing)\n"; last_status=0; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue; } } else { std::cout << "AI plan: "<<plan.steps.size()<<" step"<<(plan.steps.size()==1?"":"s"); if(plan.dangerous) std::cout << " (dangerous: confirmation required)"; std::cout << "\n"; for(auto &s: plan.steps){ std::cout << " - "<<s.id<<": "<<s.command<<"\n"; } if(mode_kw=="suggest"){ std::cout << "(suggest mode)\n"; last_status=0; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue; } }
                    if(plan.dangerous){ std::cout << "Dangerous steps detected. Type 'yes' to execute: "; std::string resp; std::getline(std::cin,resp); if(resp!="yes"){ std::cout << "Aborted.\n"; last_status=1; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue; } }
                    for(auto &step: plan.steps){
                        std::cout << "Executing ["<<step.id<<"]: "<<step.command<<"\n";
                        std::string lowcmd=step.command; std::transform(lowcmd.begin(),lowcmd.end(),lowcmd.begin(),::tolower);
                        // Native execution for loops: for VAR in {A..B}; do BODY; done
                        std::function<bool(const std::string&)> exec_native_for; // forward declaration for recursion
                        exec_native_for = [&](const std::string& cmd)->bool {
                            // Base regex: for i in {1..10}; do echo $i; done
                            static const std::regex re(R"(^for\s+([A-Za-z_][A-Za-z0-9_]*)\s+in\s+\{(-?\d+)\.\.(-?\d+)\}\s*;\s*do\s*(.*)\s*;\s*done\s*$)");
                            std::smatch m; if(!std::regex_match(cmd, m, re)) return false;
                            std::string var = m[1]; long start = std::stol(m[2]); long end = std::stol(m[3]); std::string body = m[4];
                            bool ascending = start <= end;
                            // Split body by ';' respecting quotes
                            std::vector<std::string> body_cmds; {
                                std::string cur; bool in_single=false,in_double=false; for(size_t i=0;i<body.size();++i){ char c=body[i]; if(c=='\'' && !in_double){ in_single=!in_single; cur.push_back(c); continue; } if(c=='"' && !in_single){ in_double=!in_double; cur.push_back(c); continue; } if(c==';' && !in_single && !in_double){ if(!cur.empty()){ // trim
                                            size_t a=0; while(a<cur.size() && std::isspace((unsigned char)cur[a])) ++a; size_t b=cur.size(); while(b>a && std::isspace((unsigned char)cur[b-1])) --b; body_cmds.push_back(cur.substr(a,b-a)); cur.clear(); continue; } else { continue; } }
                                    cur.push_back(c);
                                }
                                if(!cur.empty()){ size_t a=0; while(a<cur.size() && std::isspace((unsigned char)cur[a])) ++a; size_t b=cur.size(); while(b>a && std::isspace((unsigned char)cur[b-1])) --b; body_cmds.push_back(cur.substr(a,b-a)); }
                            }
                            if(body_cmds.empty()) return true; // no body
                            auto substitute_var = [&](std::string line, long val){
                                std::string sval = std::to_string(val);
                                // Replace $var and ${var}
                                std::string pattern1 = std::string("$")+var; std::string pattern2 = std::string("${")+var+"}";
                                size_t pos=0; while((pos=line.find(pattern2,pos))!=std::string::npos){ line.replace(pos, pattern2.size(), sval); pos+=sval.size(); }
                                pos=0; while((pos=line.find(pattern1,pos))!=std::string::npos){ line.replace(pos, pattern1.size(), sval); pos+=sval.size(); }
                                return line;
                            };
                            static autoshell::ExecContext ai_exec_ctx_loop; autoshell::ExecutorPOSIX ex(ai_exec_ctx_loop);
                            auto run_one = [&](const std::string& raw)->int {
                                autoshell::Lexer lx(raw); auto ts=lx.run(); autoshell::AST ast_step=autoshell::parse_tokens(ts); return ex.run(ast_step);
                            };
                            auto is_for = [](const std::string& s){ return s.find("for ")==0; };
                            std::function<void(long)> run_iter = [&](long v){ for(auto &bc: body_cmds){ std::string inst=substitute_var(bc,v); if(is_for(inst)){ if(!exec_native_for(inst)) { int st = run_one(inst); if(st!=0) std::cout << "[loop] nested status="<<st<<"\n"; } } else { int st=run_one(inst); if(st!=0) std::cout << "[loop] iteration="<<v<<" status="<<st<<"\n"; } } };
                            if(ascending){ for(long v=start; v<=end; ++v) run_iter(v); } else { for(long v=start; v>=end; --v) run_iter(v); }
                            return true;
                        };
                        if(exec_native_for(step.command)) { std::cout << "[AI] Loop executed natively.\n"; continue; }
                        // TODO: native brace expansion detection here (already handled earlier in expand)
                        autoshell::Lexer lx(step.command); auto ts=lx.run(); autoshell::AST ast_step=autoshell::parse_tokens(ts); static autoshell::ExecContext ai_exec_ctx; autoshell::ExecutorPOSIX ex(ai_exec_ctx); int st=ex.run(ast_step); if(st!=0) std::cout << "Step "<<step.id<<" failed status="<<st<<" (continuing)\n";
                    }
                    last_status=0; char buf2[16]; std::snprintf(buf2,sizeof(buf2),"%d",last_status); setenv("?",buf2,1); continue;
                } else {
                    // Gestione comando speciale 'ai pricing <prompt_per_1k> <completion_per_1k>'
                    if(mode_kw=="pricing") {
                        std::istringstream iss2(request); double p=0.0,c=0.0; iss2>>p>>c; if(!iss2.fail()){
                            g_cfg.llm_prompt_price_per_1k=p; g_cfg.llm_completion_price_per_1k=c;
                            std::cout << "[AI] Updated pricing: prompt=$"<<std::fixed<<std::setprecision(4)<<p<<"/1K completion=$"<<c<<"/1K\n";
                            last_status=0; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue;
                        } else {
                            std::cout << "[AI] Usage: ai pricing <prompt_price_per_1k> <completion_price_per_1k>\n";
                            last_status=1; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue;
                        }
                    }
                    std::cout << "Invalid ai mode. Use: ai suggest <req> | ai auto <req> | ai pricing <p> <c>\n"; last_status = 1; char buf[16]; std::snprintf(buf,sizeof(buf),"%d",last_status); setenv("?",buf,1); continue;
                }
            }
        }
        autoshell::Lexer lexer(line);
        auto token_stream = lexer.run();
        autoshell::AST ast = autoshell::parse_tokens(token_stream);
        static autoshell::ExecContext exec_ctx;
        autoshell::ExecutorPOSIX executor(exec_ctx);
        // Execute normal shell AST
        last_status = executor.run(ast);
        char buf[16]; std::snprintf(buf, sizeof(buf), "%d", last_status); setenv("?", buf, 1);
    } // end while
    return last_status;
} // end main
