#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

#include <string.h>
class SmallShell;
class JobsList;

class Command {
protected:
   char cmd_line[80];
 public:
  Command(const char* cmd_line);
  virtual ~Command() {}
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  char* getCmdLine() { return this->cmd_line;}
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
private:
public:
  ChangeDirCommand(const char* cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
  public:
    ChangePromptCommand(const char* cmd_line);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
public:
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};




class JobsList {
 public:
  class JobEntry {
  public:
   int job_id;
   Command* cmd;
   int pid;
   time_t work_time;
   time_t start_time;
   bool isStopped;
   JobEntry(int job_id, Command* cmd, int pid, time_t work_time, time_t start_time, bool isStopped) 
           : job_id(job_id), cmd(cmd), pid(pid), work_time(work_time), start_time(start_time), isStopped(isStopped) {}
  };
  std::vector<JobEntry*> jobslist;
  JobsList() { jobslist = std::vector<JobEntry*>();}
  void addJob(Command* cmd, pid_t pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob();
  JobEntry *getLastStoppedJob();
  int getNextJobId();
};

class JobsCommand : public BuiltInCommand {
 public:  
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {

 public:
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {

 public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {

 public:
  BackgroundCommand(const char* cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TailCommand : public BuiltInCommand {
 public:
  TailCommand(const char* cmd_line);
  virtual ~TailCommand() {}
  void execute() override;
};

class TouchCommand : public BuiltInCommand {
 public:
  TouchCommand(const char* cmd_line);
  virtual ~TouchCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  SmallShell();
 public:
  char* plastPwd[1];
  char prompt[80];
  JobsList* joblist;
  pid_t fg_pid;
  pid_t my_pid;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  void setPrompt(const char* new_p);
};

bool isFgRunning(); // for signals

#endif //SMASH_COMMAND_H_
