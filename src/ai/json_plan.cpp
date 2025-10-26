#include <ai-autoshell/ai/json_plan.hpp>
#include <algorithm>
#include <cctype>

namespace autoshell::ai {

static std::string trim(const std::string& s){ size_t a=0; while(a<s.size() && std::isspace((unsigned char)s[a])) ++a; size_t b=s.size(); while(b>a && std::isspace((unsigned char)s[b-1])) --b; return s.substr(a,b-a); }

ParsedPlan parse_plan_json(const std::string& input) {
    ParsedPlan out; std::string json = input;
    // crude isolate outer braces
    size_t a=json.find('{'); size_t b=json.find_last_of('}'); if(a==std::string::npos||b==std::string::npos||b<=a) return out; json = json.substr(a,b-a+1);
    auto extract_string=[&](const std::string& key){ std::string k = '"'+key+'"'; size_t p=json.find(k); if(p==std::string::npos) return std::string(); size_t colon=json.find(':',p); if(colon==std::string::npos) return std::string(); size_t q1=json.find('"',colon+1); if(q1==std::string::npos) return std::string(); size_t q2=json.find('"',q1+1); if(q2==std::string::npos) return std::string(); return json.substr(q1+1,q2-q1-1); };
    out.request = extract_string("request");
    // steps array parsing robusto contro braces dentro stringhe (es: {1..100})
    size_t sp = json.find("\"steps\""); if(sp==std::string::npos) return out; size_t arr = json.find('[',sp); if(arr==std::string::npos) return out;
    // Trova la chiusura corretta dell'array tenendo conto di [] annidati improbabili
    int bracket_depth=0; size_t arr_end=std::string::npos; bool in_str=false; bool escape=false;
    for(size_t i=arr; i<json.size(); ++i){ char c=json[i]; if(!in_str){ if(c=='[') bracket_depth++; else if(c==']'){ bracket_depth--; if(bracket_depth==0){ arr_end=i; break; } } else if(c=='"'){ in_str=true; } }
        else { if(escape){ escape=false; } else if(c=='\\'){ escape=true; } else if(c=='"'){ in_str=false; } }
    }
    if(arr_end==std::string::npos) return out; std::string arr_content = json.substr(arr+1, arr_end-arr-1);
    size_t pos=0; int idx=1; while(pos < arr_content.size()){
        // trova inizio object
        size_t obj_start=std::string::npos; bool in_s=false; bool esc=false; for(size_t i=pos;i<arr_content.size();++i){ char c=arr_content[i]; if(!in_s){ if(c=='"') in_s=true; else if(c=='{'){ obj_start=i; break; } else if(c==']') { pos=arr_content.size(); break; } }
            else { if(esc) esc=false; else if(c=='\\') esc=true; else if(c=='"') in_s=false; } }
        if(obj_start==std::string::npos) break;
        // trova fine object bilanciando
        int depth=0; size_t obj_end=std::string::npos; in_s=false; esc=false; for(size_t i=obj_start;i<arr_content.size();++i){ char c=arr_content[i]; if(!in_s){ if(c=='"') in_s=true; else if(c=='{'){ depth++; } else if(c=='}'){ depth--; if(depth==0){ obj_end=i; break; } } }
            else { if(esc) esc=false; else if(c=='\\') esc=true; else if(c=='"') in_s=false; } }
        if(obj_end==std::string::npos) break;
        std::string obj = arr_content.substr(obj_start, obj_end-obj_start+1);
        auto field=[&](const std::string& k){ std::string key='"'+k+'"'; size_t p=obj.find(key); if(p==std::string::npos) return std::string(); size_t c=obj.find(':',p); if(c==std::string::npos) return std::string(); // trova primo '"' dopo : ignorando spazi
            size_t scan=c+1; while(scan<obj.size() && std::isspace((unsigned char)obj[scan])) ++scan; if(scan>=obj.size()||obj[scan]!='"') return std::string(); size_t q1=scan; size_t q2=obj.find('"',q1+1); if(q2==std::string::npos) return std::string(); return obj.substr(q1+1,q2-q1-1); };
        ParsedStep ps; ps.id = field("id"); if(ps.id.empty()) ps.id = "s"+std::to_string(idx++); else idx++; ps.description = field("description"); ps.command = field("command");
        if(obj.find("\"confirm\"")!=std::string::npos){ size_t kp=obj.find("\"confirm\""); size_t colon=obj.find(':',kp); if(colon!=std::string::npos){ std::string tail=obj.substr(colon+1); tail=trim(tail); ps.confirm = (tail.rfind("true",0)==0); } }
        if(!ps.command.empty()) out.steps.push_back(ps);
        pos = obj_end + 1;
    }
    out.valid = true; // consider syntactically parsed even if zero steps
    return out;
}

} // namespace autoshell::ai
