#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <ai-autoshell/exec/job.hpp>

// NOTE: Advanced job control integration normally requires executor context; here we unit-test JobTable reactions to stopped processes.

using namespace autoshell;

TEST(JobsAdvanced, StopAndResumeSingleProcess) {
    JobTable jt;
    pid_t pid = fork();
    ASSERT_GE(pid,0);
    if (pid==0) {
        // Child: spin then pause via raise(SIGTSTP)
        for (volatile int i=0;i<10000000;++i) {}
        raise(SIGTSTP);
        // If continued: sleep briefly then exit
        sleep(1);
        _exit(0);
    }
    // Put child in its own process group
    setpgid(pid,pid);
    int id = jt.add(pid, "spin-and-stop", true);

    // Wait until stopped
    bool sawStopped=false; for (int tries=0; tries<50 && !sawStopped; ++tries) {
        usleep(20000);
        jt.reap();
        for (auto &j: jt.list()) if (j.id==id && j.stopped) sawStopped=true;
    }
    ASSERT_TRUE(sawStopped) << "Process never reported stopped";

    // Resume via SIGCONT
    ASSERT_EQ(kill(-pid, SIGCONT), 0);

    // Clear stopped flag after resume (reap will detect running children); we poll for completion.
    bool finished=false; for (int tries=0; tries<200 && !finished; ++tries) {
        usleep(20000);
        jt.reap();
        for (auto &j: jt.list()) if (j.id==id && !j.running) finished=true;
    }
    ASSERT_TRUE(finished) << "Job did not finish after resume";
}
