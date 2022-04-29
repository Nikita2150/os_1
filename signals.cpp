#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  cout << "smash: got ctrl-Z" << std::endl;
  if(isFgRunning())
  {
    SmallShell& instance = SmallShell::getInstance();
    for(auto job : instance.joblist->jobslist)
    {
      if(job->pid == instance.fg_pid)
      {
        job->isStopped = true;
        job->work_time += difftime(time(NULL), job->start_time);
        //TODO: remember, init again start_time when running second time
      }
    }
    kill(instance.fg_pid, SIGSTOP);
    cout << "smash: process " << instance.fg_pid << " was stopped" << std::endl;
  }
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << std::endl;
  if(isFgRunning())
  {
    SmallShell& instance = SmallShell::getInstance();
    kill(instance.fg_pid, SIGKILL);
    cout << "smash: process " << instance.fg_pid << " was killed" << std::endl;
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

