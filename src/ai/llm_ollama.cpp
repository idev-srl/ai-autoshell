#include <ai-autoshell/ai/llm.hpp>
#include <curl/curl.h>
#include <sstream>
#include <string>
#include <optional>
#include <memory>

namespace autoshell::ai {
// Callback comune per tutti i client basati su curl in questo file
static size_t curl_write_cb_ollama(char* ptr, size_t size, size_t nmemb, void* userdata){
    auto* out = static_cast<std::string*>(userdata); out->append(ptr, size*nmemb); return size*nmemb;
}

// Implementazioni aggiuntive (Claude, Gemini) incluse qui per evitare problemi di tipo incompleto.
// Claude client minimale
class ClaudeLLMClient : public LLMClient {
public:
    explicit ClaudeLLMClient(const LLMConfig& cfg):m_cfg(cfg){}
    std::optional<LLMCompletion> complete(const std::string& prompt) override {
        const char* env_key=nullptr; if(!m_cfg.api_key_env.empty()) env_key=std::getenv(m_cfg.api_key_env.c_str()); std::string key=(env_key && *env_key)?env_key:m_cfg.api_key; if(key.empty()) return LLMCompletion{"(no-key-direct)","error"};
        std::string endpoint=m_cfg.endpoint.empty()?"https://api.anthropic.com/v1/messages":m_cfg.endpoint;
        CURL* curl=curl_easy_init(); if(!curl) return LLMCompletion{"(curl-init-fail)","error"}; std::string response; curl_easy_setopt(curl,CURLOPT_URL,endpoint.c_str()); curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,curl_write_cb_ollama); curl_easy_setopt(curl,CURLOPT_WRITEDATA,&response); curl_easy_setopt(curl,CURLOPT_TIMEOUT,m_cfg.timeout_seconds);
        auto esc=[&](const std::string& in){ std::string out; out.reserve(in.size()+16); for(char c: in){ switch(c){ case '"': out+="\\\""; break; case '\\': out+="\\\\"; break; case '\n': out+="\\n"; break; case '\r': out+="\\r"; break; case '\t': out+="\\t"; break; default: out.push_back(c);} } return out; };
        std::ostringstream body; body<<"{\"model\":\""<<(m_cfg.model.empty()?"claude-3-haiku-20240307":m_cfg.model)<<"\",\"max_tokens\":256,\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\""<<esc(prompt)<<"\"}]}]}";
        struct curl_slist* headers=nullptr; headers=curl_slist_append(headers,"Content-Type: application/json"); headers=curl_slist_append(headers,(std::string("x-api-key: ")+key).c_str()); headers=curl_slist_append(headers,"anthropic-version: 2023-06-01"); curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers); curl_easy_setopt(curl,CURLOPT_POST,1L); std::string b=body.str(); curl_easy_setopt(curl,CURLOPT_POSTFIELDS,b.c_str());
        auto res=curl_easy_perform(curl); long code=0; curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code); curl_slist_free_all(headers); curl_easy_cleanup(curl);
        if(res!=CURLE_OK || code/100!=2) return LLMCompletion{"(claude error code="+std::to_string(code)+")","error"};
        std::string text; size_t pos=response.find("\"text\""); if(pos!=std::string::npos){ pos=response.find(':',pos); if(pos!=std::string::npos){ ++pos; while(pos<response.size() && response[pos]==' ') ++pos; if(pos<response.size() && response[pos]=='"'){ ++pos; bool esc2=false; for(size_t i=pos;i<response.size();++i){ char c=response[i]; if(esc2){ if(c=='n') text+="\n"; else if(c=='r') text+="\r"; else if(c=='t') text+="\t"; else text.push_back(c); esc2=false; continue;} if(c=='\\'){ esc2=true; continue;} if(c=='"') break; text.push_back(c);} } } }
        if(text.empty()) text="(parse-empty)";
        auto extract_int=[&](const std::string& key){ size_t p=response.find(key); if(p==std::string::npos) return -1; p=response.find(':',p); if(p==std::string::npos) return -1; ++p; while(p<response.size() && response[p]==' ') ++p; size_t e=p; while(e<response.size() && isdigit((unsigned char)response[e])) ++e; if(e==p) return -1; return std::stoi(response.substr(p,e-p)); };
        int prompt_tokens=extract_int("\"input_tokens\""); int completion_tokens=extract_int("\"output_tokens\""); int total_tokens=(prompt_tokens>=0 && completion_tokens>=0)?(prompt_tokens+completion_tokens):-1;
        double p_cost=-1.0,c_cost=-1.0,t_cost=-1.0; if(prompt_tokens>=0 && m_cfg.prompt_price_per_1k>0) p_cost=(prompt_tokens/1000.0)*m_cfg.prompt_price_per_1k; if(completion_tokens>=0 && m_cfg.completion_price_per_1k>0) c_cost=(completion_tokens/1000.0)*m_cfg.completion_price_per_1k; if(p_cost>=0||c_cost>=0) t_cost=(p_cost<0?0:p_cost)+(c_cost<0?0:c_cost);
        return LLMCompletion{text,"claude",prompt_tokens,completion_tokens,total_tokens,p_cost,c_cost,t_cost};
    }
private: LLMConfig m_cfg; };

class GeminiLLMClient : public LLMClient {
public:
    explicit GeminiLLMClient(const LLMConfig& cfg):m_cfg(cfg){}
    std::optional<LLMCompletion> complete(const std::string& prompt) override {
        const char* env_key=nullptr; if(!m_cfg.api_key_env.empty()) env_key=std::getenv(m_cfg.api_key_env.c_str()); std::string key=(env_key && *env_key)?env_key:m_cfg.api_key; if(key.empty()) return LLMCompletion{"(no-key-direct)","error"};
        std::string model=m_cfg.model.empty()?"gemini-1.5-flash":m_cfg.model; std::string base=m_cfg.endpoint.empty()?"https://generativelanguage.googleapis.com/v1/models/":m_cfg.endpoint; std::string endpoint=base+model+":generateContent?key="+key;
        CURL* curl=curl_easy_init(); if(!curl) return LLMCompletion{"(curl-init-fail)","error"}; std::string response; curl_easy_setopt(curl,CURLOPT_URL,endpoint.c_str()); curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,curl_write_cb_ollama); curl_easy_setopt(curl,CURLOPT_WRITEDATA,&response); curl_easy_setopt(curl,CURLOPT_TIMEOUT,m_cfg.timeout_seconds);
        struct curl_slist* headers=nullptr; headers=curl_slist_append(headers,"Content-Type: application/json"); curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
        auto esc=[&](const std::string& in){ std::string out; out.reserve(in.size()+16); for(char c: in){ switch(c){ case '"': out+="\\\""; break; case '\\': out+="\\\\"; break; case '\n': out+="\\n"; break; case '\r': out+="\\r"; break; case '\t': out+="\\t"; break; default: out.push_back(c);} } return out; };
        std::ostringstream body; body<<"{\"contents\":[{\"parts\":[{\"text\":\""<<esc(prompt)<<"\"}]}]}"; std::string b=body.str(); curl_easy_setopt(curl,CURLOPT_POST,1L); curl_easy_setopt(curl,CURLOPT_POSTFIELDS,b.c_str());
        auto res=curl_easy_perform(curl); long code=0; curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code); curl_slist_free_all(headers); curl_easy_cleanup(curl);
        if(res!=CURLE_OK || code/100!=2) return LLMCompletion{"(gemini error code="+std::to_string(code)+")","error"};
        std::string text; size_t pos=response.find("\"text\""); if(pos!=std::string::npos){ pos=response.find(':',pos); if(pos!=std::string::npos){ ++pos; while(pos<response.size() && response[pos]==' ') ++pos; if(pos<response.size() && response[pos]=='"'){ ++pos; bool esc2=false; for(size_t i=pos;i<response.size();++i){ char c=response[i]; if(esc2){ if(c=='n') text+="\n"; else if(c=='r') text+="\r"; else if(c=='t') text+="\t"; else text.push_back(c); esc2=false; continue;} if(c=='\\'){ esc2=true; continue;} if(c=='"') break; text.push_back(c);} } } }
        if(text.empty()) text="(parse-empty)";
        auto extract_int=[&](const std::string& key){ size_t p=response.find(key); if(p==std::string::npos) return -1; p=response.find(':',p); if(p==std::string::npos) return -1; ++p; while(p<response.size() && response[p]==' ') ++p; size_t e=p; while(e<response.size() && isdigit((unsigned char)response[e])) ++e; if(e==p) return -1; return std::stoi(response.substr(p,e-p)); };
        int prompt_tokens=extract_int("\"promptTokenCount\""); int completion_tokens=extract_int("\"candidatesTokenCount\""); int total_tokens=extract_int("\"totalTokenCount\"");
        double p_cost=-1.0,c_cost=-1.0,t_cost=-1.0; if(prompt_tokens>=0 && m_cfg.prompt_price_per_1k>0) p_cost=(prompt_tokens/1000.0)*m_cfg.prompt_price_per_1k; if(completion_tokens>=0 && m_cfg.completion_price_per_1k>0) c_cost=(completion_tokens/1000.0)*m_cfg.completion_price_per_1k; if(p_cost>=0||c_cost>=0) t_cost=(p_cost<0?0:p_cost)+(c_cost<0?0:c_cost);
        return LLMCompletion{text,"gemini",prompt_tokens,completion_tokens,total_tokens,p_cost,c_cost,t_cost};
    }
private: LLMConfig m_cfg; };
// Ollama client

class OllamaLLMClient : public LLMClient {
public:
    explicit OllamaLLMClient(const LLMConfig& cfg) : m_cfg(cfg) {}
    std::optional<LLMCompletion> complete(const std::string& prompt) override {
        std::string endpoint = m_cfg.endpoint.empty() ? "http://localhost:11434/api/generate" : m_cfg.endpoint;
        CURL* curl = curl_easy_init(); if(!curl) return LLMCompletion{"(curl-init-fail)", "error"};
        std::string response; curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str()); curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb_ollama); curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response); curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_cfg.timeout_seconds);
        struct curl_slist* headers=nullptr; headers=curl_slist_append(headers, "Content-Type: application/json"); curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        // Body conforme API generate: {"model":"<model>","prompt":"...","stream":false}
        auto escape_json=[&](const std::string& in){ std::string out; out.reserve(in.size()+16); for(char c: in){ switch(c){ case '"': out+="\\\""; break; case '\\': out+="\\\\"; break; case '\n': out+="\\n"; break; case '\r': out+="\\r"; break; case '\t': out+="\\t"; break; default: out.push_back(c); } } return out; };        
        std::ostringstream body; body << "{\"model\":\"" << (m_cfg.model.empty()?"llama2":m_cfg.model) << "\",\"prompt\":\"" << escape_json(prompt) << "\",\"stream\":false}"; std::string b=body.str(); curl_easy_setopt(curl, CURLOPT_POST, 1L); curl_easy_setopt(curl, CURLOPT_POSTFIELDS, b.c_str());
        auto res = curl_easy_perform(curl); long code=0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,&code); curl_slist_free_all(headers); curl_easy_cleanup(curl);
        if(res!=CURLE_OK || code/100!=2){ return LLMCompletion{"(ollama error code="+std::to_string(code)+")","error"}; }
        // Estrarre campo "response":"..." (semplice parser)
        std::string text; std::string key="\"response\""; size_t pos=response.find(key); if(pos!=std::string::npos){ pos=response.find(':',pos); if(pos!=std::string::npos){ ++pos; while(pos<response.size() && (response[pos]==' ')) ++pos; if(pos<response.size() && response[pos]=='"'){ ++pos; bool esc=false; for(size_t i=pos;i<response.size();++i){ char c=response[i]; if(esc){ if(c=='n') text+="\n"; else if(c=='r') text+="\r"; else if(c=='t') text+="\t"; else text.push_back(c); esc=false; continue;} if(c=='\\'){ esc=true; continue;} if(c=='"') break; text.push_back(c); } } } }
        if(text.empty()) text="(parse-empty)";
        // Ollama non fornisce token usage diretto qui -> lasciamo -1
        LLMCompletion comp{text, "ollama", -1,-1,-1,-1.0,-1.0,-1.0}; return comp;
    }
private:
    LLMConfig m_cfg;
};

// Extend factory (wrapped in anonymous helper to avoid ODR issues)
std::unique_ptr<LLMClient> make_llm_provider_extended(const LLMConfig& cfg){
    if(cfg.enabled){
        if(cfg.provider=="openai") return std::make_unique<OpenAILLMClient>(cfg);
        if(cfg.provider=="ollama") return std::make_unique<OllamaLLMClient>(cfg);
        if(cfg.provider=="claude") return std::make_unique<ClaudeLLMClient>(cfg); // definita in llm_claude.cpp
        if(cfg.provider=="gemini") return std::make_unique<GeminiLLMClient>(cfg); // definita in llm_gemini.cpp
    }
    return std::make_unique<StubLLMClient>(cfg);
}

} // namespace autoshell::ai
