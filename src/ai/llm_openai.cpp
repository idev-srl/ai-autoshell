#include <ai-autoshell/ai/llm.hpp>
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <optional>
#include <string>

namespace autoshell::ai {

static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

std::optional<LLMCompletion> OpenAILLMClient::complete(const std::string& prompt) {
    auto escape_json = [](const std::string& in)->std::string {
        std::string out; out.reserve(in.size()+32);
        for(char c: in){
            switch(c){
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if(static_cast<unsigned char>(c) < 0x20) {
                        char buf[8]; std::snprintf(buf,sizeof(buf),"\\u%04x", (unsigned char)c); out += buf;
                    } else out.push_back(c);
            }
        }
        return out;
    };
    const char* env_key = nullptr;
    if (!m_cfg.api_key_env.empty()) env_key = std::getenv(m_cfg.api_key_env.c_str());
    std::string key = (env_key && *env_key) ? env_key : m_cfg.api_key;
    if (key.empty()) {
        std::string reason;
        if(!m_cfg.api_key_env.empty()) {
            // If user mistakenly used the actual key as env var name (starts with sk-) signal misconfiguration
            if(m_cfg.api_key_env.rfind("sk-",0)==0) {
                reason = "(misconfigured-env-key-name)"; // they used the key as variable name
            } else {
                const char* raw = std::getenv(m_cfg.api_key_env.c_str());
                if(raw==nullptr) reason = "(env-missing:" + m_cfg.api_key_env + ")"; else if(!*raw) reason = "(env-empty:" + m_cfg.api_key_env + ")"; else reason = "(env-unreadable)"; // improbabile
            }
        } else {
            reason = "(no-key-direct)";
        }
        return std::optional<LLMCompletion>{LLMCompletion{reason, "error"}};
    }
    std::string endpoint = m_cfg.endpoint.empty() ? "https://api.openai.com/v1/chat/completions" : m_cfg.endpoint;
    CURL* curl = curl_easy_init();
    if(!curl) return std::optional<LLMCompletion>{LLMCompletion{"(curl-init-fail)", "error"}};
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_cfg.timeout_seconds);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth = std::string("Authorization: Bearer ") + key;
    headers = curl_slist_append(headers, auth.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // Minimal JSON body (no streaming)
    std::string system_content = "You are a shell assistant. Reply ONLY with valid JSON (no text before or after). Schema: {request:string, steps:[{id:string, description:string, command:string, confirm:boolean}]}. 'confirm' must be true only for dangerous commands (rm, sudo, chmod 777). Example:\n{\n  \"request\": \"create listing file\",\n  \"steps\":[\n    {\n      \"id\": \"s1\", \"description\": \"List files by size\", \"command\": \"ls -laS > listing.txt\", \"confirm\": false\n    }\n  ]\n}\nEnd example. Now answer.";
    std::ostringstream body;
    body << "{\"model\":\"" << (m_cfg.model.empty()?"gpt-4o-mini":m_cfg.model) << "\","
         << "\"messages\":[{\"role\":\"system\",\"content\":\"" << escape_json(system_content) << "\"},{\"role\":\"user\",\"content\":\"" << escape_json(prompt) << "\"}],"
         << "\"temperature\":" << m_cfg.temperature << ",\"max_tokens\":" << m_cfg.max_tokens << "}";
    std::string body_str = body.str();
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
    auto res = curl_easy_perform(curl);
    long code = 0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if(res != CURLE_OK || code/100 != 2) {
        // Prova a estrarre "message" dal body error JSON
        std::string msg;
        size_t mpos = response.find("\"message\"");
        if(mpos != std::string::npos){
            size_t colon = response.find(':', mpos);
            if(colon!=std::string::npos){
                size_t q1 = response.find('"', colon+1);
                size_t q2 = response.find('"', q1+1);
                if(q1!=std::string::npos && q2!=std::string::npos){ msg = response.substr(q1+1, q2-q1-1); }
            }
        }
        std::string combined = "(openai error code=" + std::to_string(code) + (msg.empty()?"":" msg="+msg) + ")";
        return std::optional<LLMCompletion>{LLMCompletion{combined, "error"}};
    }
    // Improved parsing: find 'choices' array and first occurrence of message.content
    // Note: avoid external JSON dependency to keep build minimal.
    auto find_key = [&](const std::string& src, size_t start, const std::string& key)->size_t {
        return src.find(key, start);
    };
    std::string raw = response; // copia per logging
    std::string content;
    size_t choices_pos = find_key(response, 0, "\"choices\"");
    if(choices_pos != std::string::npos) {
        size_t arr_start = response.find('[', choices_pos);
        if(arr_start != std::string::npos) {
            size_t first_obj = response.find('{', arr_start+1);
            if(first_obj != std::string::npos) {
                // all'interno del primo oggetto cerchiamo "message" poi "content"
                size_t message_pos = response.find("\"message\"", first_obj);
                if(message_pos != std::string::npos) {
                    size_t msg_obj_start = response.find('{', message_pos);
                    // grezzo: cerchiamo la chiusura '}' più vicina prima di '"role" dell'assistente successivo
                    if(msg_obj_start != std::string::npos) {
                        msg_obj_end = response.find('}', msg_obj_start+1);
                    }
                    size_t content_key = response.find("\"content\"", msg_obj_start);
                    if(content_key != std::string::npos) {
                        size_t colon = response.find(':', content_key);
                        if(colon != std::string::npos) {
                            // salto spazi
                            size_t val_start = colon+1; while(val_start < response.size() && (response[val_start]==' ')) ++val_start;
                            if(val_start < response.size() && response[val_start]=='"') {
                                ++val_start; bool escape=false; std::string buf;
                                for(size_t i=val_start; i<response.size(); ++i){ char c=response[i]; if(escape){
                                        if(c=='n') buf.push_back('\n'); else if(c=='r') buf.push_back('\r'); else if(c=='t') buf.push_back('\t'); else buf.push_back(c); escape=false; continue; }
                                    if(c=='\\'){ escape=true; continue; }
                                    if(c=='"'){ content=buf; break; }
                                    buf.push_back(c);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(content.empty()) {
        // Fallback: last occurrence of "content" if not found through canonical path
        std::string marker = "\"content\":";
        auto pos = response.rfind(marker);
        if(pos != std::string::npos) {
            pos += marker.size(); while(pos < response.size() && response[pos]==' ') ++pos; if(pos < response.size() && response[pos]=='"') ++pos; bool escape=false; for(size_t i=pos;i<response.size();++i){ char c=response[i]; if(escape){ if(c=='n') content.push_back('\n'); else if(c=='r') content.push_back('\r'); else if(c=='t') content.push_back('\t'); else content.push_back(c); escape=false; continue; } if(c=='\\'){ escape=true; continue; } if(c=='"') break; content.push_back(c);} }
    }
    int prompt_tokens=-1, completion_tokens=-1, total_tokens=-1;
    // Grep-like extraction for usage fields
    auto extract_int = [&](const std::string& key)->int{
        size_t pos = response.find(key);
        if (pos == std::string::npos) {
            return -1;
        }
        pos = response.find(':', pos);
        if (pos == std::string::npos) {
            return -1;
        }
        ++pos;
        while (pos < response.size() && (response[pos] == ' ' || response[pos] == '\n')) {
            ++pos;
        }
        size_t end = pos;
        while (end < response.size() && isdigit((unsigned char)response[end])) {
            ++end;
        }
        if (end == pos) {
            return -1;
        }
        return std::stoi(response.substr(pos, end - pos));
    };
    prompt_tokens = extract_int("\"prompt_tokens\"");
    completion_tokens = extract_int("\"completion_tokens\"");
    total_tokens = extract_int("\"total_tokens\"");
    if(!content.empty()) {
        size_t endtrim = content.find_last_not_of(" \t\n\r"); if(endtrim!=std::string::npos) content.erase(endtrim+1);
        double p_cost=-1.0, c_cost=-1.0, t_cost=-1.0;
        if(prompt_tokens>=0 && m_cfg.prompt_price_per_1k>0.0) p_cost = (prompt_tokens/1000.0)*m_cfg.prompt_price_per_1k;
        if(completion_tokens>=0 && m_cfg.completion_price_per_1k>0.0) c_cost = (completion_tokens/1000.0)*m_cfg.completion_price_per_1k;
        if(p_cost>=0.0 || c_cost>=0.0) t_cost = (p_cost<0?0:p_cost) + (c_cost<0?0:c_cost);
        LLMCompletion comp{content, "openai", prompt_tokens, completion_tokens, total_tokens, p_cost, c_cost, t_cost};
        return comp;
    }
    // No content extracted: return truncated raw body for debug
    std::string truncated = raw.substr(0, std::min<size_t>(raw.size(), 2048));
    double p_cost=-1.0, c_cost=-1.0, t_cost=-1.0;
    if(prompt_tokens>=0 && m_cfg.prompt_price_per_1k>0.0) p_cost = (prompt_tokens/1000.0)*m_cfg.prompt_price_per_1k;
    if(completion_tokens>=0 && m_cfg.completion_price_per_1k>0.0) c_cost = (completion_tokens/1000.0)*m_cfg.completion_price_per_1k;
    if(p_cost>=0.0 || c_cost>=0.0) t_cost = (p_cost<0?0:p_cost) + (c_cost<0?0:c_cost);
    return LLMCompletion{"(parse-empty) RAW:" + truncated, "error", prompt_tokens, completion_tokens, total_tokens, p_cost, c_cost, t_cost};
}

} // namespace autoshell::ai
