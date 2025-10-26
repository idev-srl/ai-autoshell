// Planner stub implementation
#include <ai-autoshell/ai/planner.hpp>
#include <sstream>
#include <algorithm>

namespace autoshell::ai {

bool Planner::is_dangerous(const std::string& cmd) const {
    std::string lower = cmd; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto has = [&](const char* s){ return lower.find(s)!=std::string::npos; };
    if(has("sudo")) return true;
    if(has("rm -rf")||has("rm -fr")) return true;
    if(has("rm ")) return true; // generic rm
    if(has("chmod 777")) return true;
    if(has("chown ")||has("chgrp ")) return true;
    if(has("dd ")) return true;
    if(has("mkfs")||has("fdisk")||has("wipefs")) return true;
    if(has("curl ") && has("| sh")) return true;
    if(has("wget ") && has("| sh")) return true;
    if(has("> /dev/sd")||has("> /dev/nvme")||has("> /dev/disk")) return true;
    return false;
}

std::vector<PlanStep> Planner::rule_expand(const std::string& request) const {
    std::vector<PlanStep> steps; int idx=1;
    auto add=[&](const std::string& desc,const std::string& cmd){ PlanStep s; s.id = "s" + std::to_string(idx++); s.description=desc; s.command=cmd; s.confirm = is_dangerous(cmd); steps.push_back(s); };
    std::string lower=request; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("list files")!=std::string::npos || lower.find("show files")!=std::string::npos) {
        add("List files in current directory","ls -la");
    }
    if (lower.find("show current directory")!=std::string::npos ||
        lower.find("current directory")!=std::string::npos ||
        lower.find("print working directory")!=std::string::npos ||
        lower.find("show pwd")!=std::string::npos ||
        lower.find("where am i")!=std::string::npos ||
        lower == "pwd" ) {
        add("Show current directory path","pwd");
    }
    if (lower.find("find text")!=std::string::npos) {
        add("Search for pattern in *.txt","grep -R 'TODO' .");
    }
    if (lower.find("remove build")!=std::string::npos || lower.find("clean build")!=std::string::npos) {
        add("Remove build directory","rm -rf build");
    }
    // Regola italiana: crea elenco.txt con i file ordinati per dimensione
    if (lower.find("elenco.txt")!=std::string::npos && lower.find("dimensione")!=std::string::npos) {
        add("List files by size into elenco.txt","ls -laS > elenco.txt");
    }
        if (lower.find("crea un elenco dei file")!=std::string::npos) {
            add("Crea elenco file in elenco.txt","ls -laS > elenco.txt");
        }
        if (lower.find("mostra la data")!=std::string::npos || lower.find("visualizza la data")!=std::string::npos) {
            add("Mostra data corrente","date");
        }
    // Regola italiana: crea un file <nome> con scritto "contenuto"
    if (lower.find("crea un file")!=std::string::npos && lower.find("con scritto")!=std::string::npos) {
        // Estrai nome file tra 'crea un file' e 'con scritto'
        size_t start = lower.find("crea un file");
        size_t after_name = lower.find("con scritto", start);
        if(after_name != std::string::npos){
            std::string between = lower.substr(start + strlen("crea un file"), after_name - (start + strlen("crea un file")));
            auto trim=[&](std::string s){ while(!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin()); while(!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back(); return s; };
            between = trim(between);
            // contenuto tra virgolette dopo 'con scritto'
            size_t quote1 = lower.find('"', after_name);
            size_t quote2 = (quote1==std::string::npos)?std::string::npos: lower.find('"', quote1+1);
            std::string content;
            if(quote1!=std::string::npos && quote2!=std::string::npos && quote2>quote1) {
                content = request.substr(quote1+1, quote2-quote1-1); // usa request originale per preservare maiuscole
            }
            if(!between.empty() && !content.empty()) {
                std::string safeFile = between; // non sanitizziamo oltre per MVP
                std::string cmd = "echo \"" + content + "\" > " + safeFile;
                add("Crea file " + safeFile + " con contenuto", cmd);
            }
        }
    }
    // Regola italiana: mostra(mi) il file <nome>
    if (lower.find("mostrami il file")!=std::string::npos || lower.find("mostra il file")!=std::string::npos) {
        size_t posShow = (lower.find("mostrami il file")!=std::string::npos)? lower.find("mostrami il file") : lower.find("mostra il file");
        if(posShow!=std::string::npos){
            size_t startName = posShow + (lower.find("mostrami il file")==posShow ? strlen("mostrami il file") : strlen("mostra il file"));
            std::string name = lower.substr(startName);
            auto trim=[&](std::string s){ while(!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin()); while(!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back(); return s; };
            name = trim(name);
            if(!name.empty()) {
                // stop at first space if multiple words
                std::istringstream iss(name); std::string fname; iss >> fname;
                if(!fname.empty()) add("Mostra contenuto file " + fname, "cat " + fname);
            }
        }
    }
    // Regola italiana: mostrami/mostra il contenuto della directory <path>
    if (lower.find("mostrami il contenuto della directory")!=std::string::npos || lower.find("mostra il contenuto della directory")!=std::string::npos) {
        size_t posDir = (lower.find("mostrami il contenuto della directory")!=std::string::npos)? lower.find("mostrami il contenuto della directory") : lower.find("mostra il contenuto della directory");
        if(posDir!=std::string::npos){
            size_t startPath = posDir + (lower.find("mostrami il contenuto della directory")==posDir ? strlen("mostrami il contenuto della directory") : strlen("mostra il contenuto della directory"));
            std::string path = lower.substr(startPath);
            auto trim=[&](std::string s){ while(!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin()); while(!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back(); return s; };
            path = trim(path);
            if(!path.empty()) {
                // Sanitizza: consenti solo [A-Za-z0-9_./-]
                std::string safe; for(char c: path){ if(std::isalnum((unsigned char)c) || c=='_'||c=='.'||c=='/'||c=='-') safe.push_back(c); }
                if(!safe.empty()) add("Lista contenuto directory " + safe, "ls -la " + safe);
            }
        }
    }
    // Regola italiana: rimuovi/elimina/cancella la cartella <nome>
    if ( (lower.find("rimuovi la cartella")!=std::string::npos) || (lower.find("elimina la cartella")!=std::string::npos) || (lower.find("cancella la cartella")!=std::string::npos) ) {
        size_t posRem = std::string::npos; const char* keys[] = {"rimuovi la cartella","elimina la cartella","cancella la cartella"};
        for(auto k: keys){ size_t p=lower.find(k); if(p!=std::string::npos){ posRem=p; break; } }
        if(posRem!=std::string::npos){
            size_t startName = posRem; for(auto k: keys){ if(lower.find(k)==posRem){ startName += strlen(k); break; } }
            std::string name = lower.substr(startName);
            auto trim=[&](std::string s){ while(!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin()); while(!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back(); return s; };
            name = trim(name);
            if(!name.empty()) {
                // primo token
                std::istringstream iss(name); std::string folder; iss >> folder;
                if(!folder.empty()) {
                    PlanStep s; s.id="s"+std::to_string(idx++); s.description="Rimuovi cartella " + folder; s.command="rm -rf " + folder; s.confirm=true; steps.push_back(s);
                }
            }
        }
    }
    // Regola italiana: crea / crea una cartella <nome>
    if (lower.find("crea una cartella")!=std::string::npos || lower.find("crea cartella")!=std::string::npos) {
        size_t posC = std::string::npos; const char* keysC[] = {"crea una cartella","crea cartella"};
        for(auto k: keysC){ size_t p=lower.find(k); if(p!=std::string::npos){ posC=p; break; } }
        if(posC!=std::string::npos){
            size_t startName = posC; for(auto k: keysC){ if(lower.find(k)==posC){ startName += strlen(k); break; } }
            std::string name = lower.substr(startName);
            auto trim=[&](std::string s){ while(!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin()); while(!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back(); return s; };
            name = trim(name);
            if(!name.empty()) { std::istringstream iss(name); std::string folder; iss >> folder; if(!folder.empty()) add("Crea cartella " + folder, "mkdir -p " + folder); }
        }
    }
    // create <file>.txt file with the list of files in current directory
    if (lower.find("create ")!=std::string::npos && lower.find("list of file")!=std::string::npos) {
        // estrai nome file semplice .txt
        size_t pos = lower.find("create ");
        std::string after = lower.substr(pos+7); // dopo 'create '
        std::istringstream iss(after); std::string fname; iss >> fname;
        if (!fname.empty() && fname.find(".txt")!=std::string::npos) {
            add("List files into " + fname, "ls -la > " + fname);
        }
    }
    if (steps.empty()) {
        add("Echo request (no rule matched)","echo 'AI: " + request + "'");
    }
    return steps;
}

Plan Planner::plan(const std::string& request) const {
    Plan p; p.request = request; p.steps = rule_expand(request);
    p.dangerous = false; std::ostringstream rs;
    for (auto &s : p.steps) { if (s.confirm) { p.dangerous = true; rs << "Step " << s.id << " requires confirmation; "; } }
    p.risk_summary = rs.str();
    return p;
}

} // namespace autoshell::ai
