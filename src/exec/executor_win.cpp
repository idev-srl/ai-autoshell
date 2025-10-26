#ifdef _WIN32
#include <ai-autoshell/exec/executor_win.hpp>
#include <ai-autoshell/expand/expand.hpp>
#include <ai-autoshell/exec/builtins.hpp>
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace autoshell {

int ExecutorWindows::run(const AST& ast){ if(!ast.list) return 0; return run_list(*ast.list); }

int ExecutorWindows::run_list(const ListNode& list){ int status=0; for(auto &seg : list.segments){ status = run_andor(*seg.and_or); } m_ctx.last_status=status; return status; }

int ExecutorWindows::run_andor(const AndOrNode& node){ int status=0; for(size_t i=0;i<node.segments.size();++i){ auto &seg=node.segments[i]; if(i>0){ if(seg.op=="&&" && status!=0) return status; if(seg.op=="||" && status==0) return status; } status = run_pipeline(*seg.pipeline); } return status; }

int ExecutorWindows::run_pipeline(const PipelineNode& pipeline){ // basic pipeline support
 if(pipeline.elements.empty()) return 0;
 if(pipeline.elements.size()==1){
    return std::visit([&](auto &ptr)->int {
        using T=std::decay_t<decltype(ptr)>; if constexpr(std::is_same_v<T,std::unique_ptr<CommandNode>>){ return run_command(*ptr); } else if constexpr(std::is_same_v<T,std::unique_ptr<SubshellNode>>){ if(ptr->list) return run_list(*ptr->list); return 0; } return 0; }, pipeline.elements[0]);
 }
 // Pipeline N>1
 size_t n = pipeline.elements.size();
 std::vector<HANDLE> readEnds, writeEnds;
 readEnds.reserve(n-1); writeEnds.reserve(n-1);
 for(size_t i=0;i<n-1;++i){
    HANDLE readPipe, writePipe; SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    if(!CreatePipe(&readPipe, &writePipe, &sa, 0)){ std::cerr << "CreatePipe failed" << std::endl; return 1; }
    // We align: write of i -> read of i
    readEnds.push_back(readPipe); writeEnds.push_back(writePipe);
 }
 std::vector<PROCESS_INFORMATION> procs(n);
 for(size_t i=0;i<n;++i){
    HANDLE inHandle = (i==0)? GetStdHandle(STD_INPUT_HANDLE) : readEnds[i-1];
    HANDLE outHandle = (i==n-1)? GetStdHandle(STD_OUTPUT_HANDLE) : writeEnds[i];
    STARTUPINFOA si{}; si.cb=sizeof(si); si.hStdInput=inHandle; si.hStdOutput=outHandle; si.hStdError=GetStdHandle(STD_ERROR_HANDLE); si.dwFlags |= STARTF_USESTDHANDLES;
    std::string cmdLine;
    int status_build=0;
    std::visit([&](auto &ptr){ using T=std::decay_t<decltype(ptr)>; if constexpr(std::is_same_v<T,std::unique_ptr<CommandNode>>){
        auto argv_expanded = expand_words(ptr->argv);
        if(argv_expanded.empty()){ status_build=0; return; }
        if(is_builtin(argv_expanded[0])){
            // Built-in inline: (simplified) executed in parent; intermediate pipe output not captured yet
            auto r = run_builtin(argv_expanded,&m_ctx); status_build = r? r->exit_code : 0; return; }
        for(size_t k=0;k<argv_expanded.size();++k){ if(k) cmdLine.push_back(' '); const std::string &a=argv_expanded[k]; bool needQ=a.find(' ')!=std::string::npos; if(needQ) cmdLine.push_back('"'); cmdLine+=a; if(needQ) cmdLine.push_back('"'); }
    } else if constexpr(std::is_same_v<T,std::unique_ptr<SubshellNode>>){ cmdLine="cmd /c echo subshell-non-supportata"; } }, pipeline.elements[i]);
    if(!cmdLine.empty()){
        PROCESS_INFORMATION pi{};
        BOOL ok = CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
        if(!ok){ std::cerr << "CreateProcess failed for segment "<<i<<" cmd='"<<cmdLine<<"'" << std::endl; return 1; }
        procs[i]=pi;
    } else {
    // Built-in already executed (if not last, pipe may remain empty - future improvement)
        procs[i]=PROCESS_INFORMATION{}; // marker
    }
    // Chiudere write end subito nel processo padre per fasi precedenti (per lettura corretta downstream)
    if(i < n-1){ CloseHandle(writeEnds[i]); }
 }
 // Chiudi tutti readEnds non più necessari (restano STDIN STDOUT).
 for(auto h : readEnds) CloseHandle(h);
 int lastStatus=0;
 for(size_t i=0;i<n;++i){ auto &pi=procs[i]; if(pi.hProcess){ WaitForSingleObject(pi.hProcess, INFINITE); DWORD code=0; GetExitCodeProcess(pi.hProcess,&code); lastStatus = (int)code; CloseHandle(pi.hThread); CloseHandle(pi.hProcess); } }
 return lastStatus;
}

int ExecutorWindows::run_command(const CommandNode& cmd){ auto argv_expanded = expand_words(cmd.argv); if(argv_expanded.empty()) return 0; if(is_builtin(argv_expanded[0])){ auto r=run_builtin(argv_expanded,&m_ctx); if(r && r->should_exit) std::exit(r->exit_code); return r? r->exit_code : 0; }
 // Costruisce comando unico concatenando argomenti (semplice quoting)
 std::string full; for(size_t i=0;i<argv_expanded.size();++i){ if(i) full.push_back(' '); const std::string &a=argv_expanded[i]; bool needQ=a.find(' ')!=std::string::npos; if(needQ) full.push_back('"'); full+=a; if(needQ) full.push_back('"'); }
 int rc = system(full.c_str()); return rc; }

} // namespace autoshell
#endif // _WIN32

// TODO: Windows pipeline improvements
// 1. Use CreatePipe for each link (done basic)
// 2. Support built-ins inside pipeline with custom capture
// 3. Proper error propagation & exit status of last command
// 4. Background execution via Job Objects
