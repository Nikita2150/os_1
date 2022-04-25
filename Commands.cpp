#include <unistd.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

int _charStoInt(char* val)
{
  int num = 0;
  int neg = 1;
  for(int i = 0; i < 80; i++)
  {
    if(val[i] == '\0') break;
    if(val[i] == '-')
    { 
      neg = -1;
      continue;
    }
    num *= 10;
    num += val[i] - 48;
  }
  return num * neg;
}


bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
  strcpy(this->prompt, "smash> ");
  *this->plastPwd = nullptr;
  this->joblist = new JobsList();
}

SmallShell::~SmallShell() {
  delete joblist;
}
void SmallShell::setPrompt(const char* new_p)
{
  strcpy(this->prompt, new_p);
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
  

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0)
  {
    return new ChangePromptCommand(cmd_line, this);
  }
  else if(firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line, plastPwd, this);
  }
  else if(firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line, this->joblist);
  }
  else if(firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line, this->joblist);
  }
  /*else {
    return new ExternalCommand(cmd_line);
  }*/
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
}

/* JOBBBBBBBBBBBB */
void JobsList::printJobsList()
{
  //stopped?
}
void JobsList::removeFinishedJobs() {}


/* Command Implementations */
/* COMMAND */
Command::Command(const char* cmd_line)
{
  strcpy(this->cmd_line, cmd_line);
}
/* BUILTIN */
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

/* CHPROMPT */
ChangePromptCommand::ChangePromptCommand(const char* cmd_line, SmallShell* instance) : BuiltInCommand(cmd_line)
{
  this->instance = instance;
}
void ChangePromptCommand::execute()
{
  char* args[20];
  args[0] = NULL;
  int nParams = _parseCommandLine(this->cmd_line, args);
  if(nParams == 1)
  {
    this->instance->setPrompt("smash> ");
  }
  else
  {
    char new_p[82];
    strcpy(new_p, args[1]);
    int i = 0;
    for(i = 0; i < 80; i++)
    { if (new_p[i] == '\0') break;}
    new_p[i] = '>';
    new_p[i+1] = ' ';
    new_p[i+2] = '\0';
    this->instance->setPrompt(new_p);
  }
}

/* SHOWPID */
ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute()
{
  cout << "smash pid is " << getpid() << std::endl;
}

/*  GETTCURRDIRCOMMAND */
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute()
{
  cout << getcwd(NULL, 0) << std::endl;
  //memory leak
}

/* CHANGECURRDIRCOMMAND */
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd, SmallShell* instance) : BuiltInCommand(cmd_line), plastPwd(plastPwd), instance(instance) {}
void ChangeDirCommand::execute()
{
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  if(nArgs != 1)
  {
    cerr << "smash error: cd: too many arguments" << std::endl;
  }
  else
  {
    if(strcmp(args[1], "-") == 0)
    {
      if(*this->plastPwd == nullptr)
      {
        cerr << "smash error: OLDPWD not set" << std::endl;
      }
      else
      {
        char* temp;
        temp = getcwd(NULL, 0);
        chdir(*this->plastPwd);
        //memory leak
        *this->instance->plastPwd = temp;
      }
    }
    else
    {
      //memory leak
      *this->instance->plastPwd = getcwd(NULL, 0);
      if(chdir(args[1]) == -1)
      {
        cerr << "smash error: chdir failed" << std::endl; //TODO chdir or cd?
      }
    }
  }
}

/* JOBS */
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void JobsCommand::execute()
{
  this->jobs->removeFinishedJobs();
  this->jobs->printJobsList();
}

/* KILL */
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void KillCommand::execute()
{
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  int jid;
  if(nArgs != 2 || _charStoInt(args[1]) > 0)
  {
    cerr << "smash error: kill: invalid arguments" << std::endl;
  }
  else
  {
    int signum = (-1) * _charStoInt(args[1]); //TODO: send negative or positive value??
    jid = _charStoInt(args[2]); 
    for (auto job : jobs->jobslist)
    {
      if(job.job_id == jid)
      {
        if(kill(job.pid, signum) == -1)
        {
          cerr << "smash error: kill failed" << std::endl;
        }
        return;
      }
    }
    cerr << "smash error: kill: job-id " << jid << " does not exist"  << std::endl;
  }
}