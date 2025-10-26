#include <ai-autoshell/ai/llm.hpp>
#include <curl/curl.h>
#include <string>
#include <optional>
#include <sstream>

namespace autoshell::ai {

static size_t curl_write_cb_gemini(char* ptr,size_t size,size_t nmemb,void*userdata){ auto* out=static_cast<std::string*>(userdata); out->append(ptr,size*nmemb); return size*nmemb; }

class GeminiLLMClient : public LLMClient {
public:
    explicit GeminiLLMClient(const LLMConfig& cfg):m_cfg(cfg){}
    std::optional<LLMCompletion> complete(const std::string& prompt) override {
        // Google Generative Language API (Gemini): POST https://generativelanguage.googleapis.com/v1/models/<model>:generateContent?key=API_KEY
        const char* env_key=nullptr; if(!m_cfg.api_key_env.empty()) env_key=std::getenv(m_cfg.api_key_env.c_str()); std::string key=(env_key && *env_key)?env_key:m_cfg.api_key; if(key.empty()) return LLMCompletion{"(no-key-direct)","error"};
        std::string model = m_cfg.model.empty()?"gemini-1.5-flash":m_cfg.model;
        std::string base = m_cfg.endpoint.empty()?"https://generativelanguage.googleapis.com/v1/models/":m_cfg.endpoint; // allow override full prefix
        std::string endpoint = base + model + ":generateContent?key=" + key;
        CURL* curl=curl_easy_init(); if(!curl) return LLMCompletion{"(curl-init-fail)","error"}; std::string response; curl_easy_setopt(curl,CURLOPT_URL,endpoint.c_str()); curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,curl_write_cb_gemini); curl_easy_setopt(curl,CURLOPT_WRITEDATA,&response); curl_easy_setopt(curl,CURLOPT_TIMEOUT,m_cfg.timeout_seconds);
        struct curl_slist* headers=nullptr; headers=curl_slist_append(headers,"Content-Type: application/json"); curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
        auto esc=[&](const std::string& in){ std::string out; out.reserve(in.size()+16); for(char c: in){ switch(c){ case '"': out+="\\\""; break; case '\\': out+="\\\\"; break; case '\n': out+="\\n"; break; case '\r': out+="\\r"; break; case '\t': out+="\\t"; break; default: out.push_back(c);} } return out; };
        // Minimal body: {"contents":[{"parts":[{"text":"..."}]}]}
        std::ostringstream body; body<<"{\"contents\":[{\"parts\":[{\"text\":\""<<esc(prompt)<<"\"}]}]}"; std::string b=body.str(); curl_easy_setopt(curl,CURLOPT_POST,1L); curl_easy_setopt(curl,CURLOPT_POSTFIELDS,b.c_str());
        auto res=curl_easy_perform(curl); long code=0; curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code); curl_slist_free_all(headers); curl_easy_cleanup(curl);
        if(res!=CURLE_OK || code/100!=2){ return LLMCompletion{"(gemini error code="+std::to_string(code)+")","error"}; }
        // Parse first "text" occurrence
        std::string text; std::string marker="\"text\""; size_t pos=response.find(marker); if(pos!=std::string::npos){ pos=response.find(':',pos); if(pos!=std::string::npos){ ++pos; while(pos<response.size() && response[pos]==' ') ++pos; if(pos<response.size() && response[pos]=='"'){ ++pos; bool esc2=false; for(size_t i=pos;i<response.size();++i){ char c=response[i]; if(esc2){ if(c=='n') text+="\n"; else if(c=='r') text+="\r"; else if(c=='t') text+="\t"; else text.push_back(c); esc2=false; continue;} if(c=='\\'){ esc2=true; continue;} if(c=='"') break; text.push_back(c);} } } }
        if(text.empty()) text="(parse-empty)";
        // Gemini usage has 'usageMetadata': {"promptTokenCount":X,"candidatesTokenCount":Y,"totalTokenCount":Z}
        auto extract_int=[&](const std::string& key){ size_t p=response.find(key); if(p==std::string::npos) return -1; p=response.find(':',p); if(p==std::string::npos) return -1; ++p; while(p<response.size() && response[p]==' ') ++p; size_t e=p; while(e<response.size() && isdigit((unsigned char)response[e])) ++e; if(e==p) return -1; return std::stoi(response.substr(p,e-p)); };
        int prompt_tokens=extract_int("\"promptTokenCount\""); int completion_tokens=extract_int("\"candidatesTokenCount\""); int total_tokens=extract_int("\"totalTokenCount\"");
        double p_cost=-1.0,c_cost=-1.0,t_cost=-1.0; if(prompt_tokens>=0 && m_cfg.prompt_price_per_1k>0) p_cost=(prompt_tokens/1000.0)*m_cfg.prompt_price_per_1k; if(completion_tokens>=0 && m_cfg.completion_price_per_1k>0) c_cost=(completion_tokens/1000.0)*m_cfg.completion_price_per_1k; if(p_cost>=0||c_cost>=0) t_cost=(p_cost<0?0:p_cost)+(c_cost<0?0:c_cost);
        return LLMCompletion{text,"gemini",prompt_tokens,completion_tokens,total_tokens,p_cost,c_cost,t_cost};
    }
private: LLMConfig m_cfg; };

std::unique_ptr<LLMClient> make_llm_provider_extended_gemini(const LLMConfig& cfg){ return std::make_unique<GeminiLLMClient>(cfg); }

} // namespace autoshell::ai
