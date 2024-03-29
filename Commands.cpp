#include <unistd.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>
#include <fcntl.h>

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

int _charStoInt(const char* val)
{
  int num = 0;
  int neg = 1;
  for(int i = 0; i < 80; i++)
  {
    if(val[i] == '\0') break;
    if(val[i] == '-' && neg > 0) //so only first '-' will count as minus
    { 
      neg = -1;
      continue;
    }
    if(val[i] < '0' || val[i] > '9') throw std::exception(); //val[i] isn't a number..
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
  if (str.size() == 0 || str[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


SmallShell::SmallShell() {
  strcpy(this->prompt, "smash> ");
  *this->plastPwd = nullptr;
  this->joblist = new JobsList();
  this->fg_pid = 0;
  this->my_pid = getpid();
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
  char temp[80] = {0};
  strcpy(temp, cmd_line);
  _removeBackgroundSign(temp);

  string cmd_s = _trim(string(temp));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0)
  {
    return new ChangePromptCommand(cmd_line);
  }
  else if(firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line);
  }
  else if(firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line);
  }
  else if(firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line);
  }
  else if(firstWord.compare("fg") == 0)
  {
    return new ForegroundCommand(cmd_line);
  }
  else if(firstWord.compare("bg") == 0)
  {
    return new BackgroundCommand(cmd_line);
  }
  else if(firstWord.compare("quit") == 0)
  {
    return new QuitCommand(cmd_line);
  }
  else if(firstWord.compare("tail") == 0)
  {
    return new TailCommand(cmd_line);
  }
  else if(firstWord.compare("touch") == 0)
  {
    return new TouchCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

char** _parseIOPipe(const char* cmd_line)
{
  char** ret_val = new char*[3];
  for(int i = 0; i < 3; i++) ret_val[i] = new char[80];

  int op_size = 0;
  int i = 0;
  for (; cmd_line[i]; i++)
  {
    if(cmd_line[i] == '>')
    {
      op_size = (i < 79 && cmd_line[i+1] == '>') ? 2 : 1;
      break;
    }
    if(cmd_line[i] == '|')
    {
      op_size = (i < 79 && cmd_line[i+1] == '&') ? 2 : 1;
      break;
    }
  }
  if(!op_size)
  {
    for(int i = 0; i < 3; i++) delete[] ret_val[i];
    delete[] ret_val;
    return nullptr;
  }
  
  //command1 seg
  strncpy(ret_val[0], cmd_line, i);
  ret_val[0][i] = '\0';
  //op seg
  strncpy(ret_val[1], cmd_line+i, op_size);
  ret_val[1][op_size] = '\0';
  //command2 seg
  strcpy(ret_val[2], cmd_line+i+op_size);
  string temp = _trim(ret_val[2]);
  strcpy(ret_val[2], temp.c_str());

  return ret_val;
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command* cmd;
  int son_pid = -1, done = 0;
  char** res = _parseIOPipe(cmd_line);
  int std_fd;
  int my_pipe[2], loc = 0;
  int new_fd = 0;
  if(res)
  {
    if(strcmp(res[1], ">") == 0 || strcmp(res[1], ">>") == 0)
    {
      cmd = CreateCommand(string(res[0]).c_str()); //cmd = command1
      std_fd = dup(1);
      close(1);
      if(strcmp(res[1], ">")== 0)
      {
        new_fd = open(res[2], O_CREAT | O_WRONLY | O_TRUNC,S_IROTH | S_IXOTH|S_IRGRP|S_IXGRP|S_IRUSR|S_IWUSR);
      }
      else
      {
        new_fd = open(res[2], O_CREAT | O_WRONLY | O_APPEND, S_IROTH | S_IXOTH|S_IRGRP|S_IXGRP|S_IRUSR|S_IWUSR);
      }
      
      if(new_fd == -1) /*failed*/
      {
        dup2(std_fd, 1);
        perror("smash error: open failed");
        return;
      }
    }
    else
    {
      pipe(my_pipe);
      son_pid = fork();
      if(son_pid < 0) perror("smash error: fork failed");
      else if(son_pid > 0) // father, output is now through the pipe 
      {
        loc = (strcmp(res[1], "|") == 0) ? 1 : 2;
        std_fd = dup(loc);
        close(my_pipe[0]);
        dup2(my_pipe[1], loc);
        cmd = CreateCommand(string(res[0]).c_str());
      }
      else //son_pid == 0, close standard stdin, input is now throgh the pipe
      {
        close(my_pipe[1]);
        dup2(my_pipe[0], 0);
        cmd = CreateCommand(string(res[2]).c_str());
      }
    }

    for(int i = 0; i < 3; i++)
      delete[] res[i];
    delete[] res;
  }
  else
  {
    cmd = CreateCommand(cmd_line);
  }
  cmd->execute();
  
  if(new_fd)
    close(new_fd);

  if(son_pid && std_fd) //if not child and changed the std fd
  {
    if(loc) //if we used pipe
      dup2(std_fd, loc);
    else
      dup2(std_fd, 1);
  }

  
  if(son_pid > 0) //enter only in the father process
  {
    close(my_pipe[1]); //closing the pipe so the fork won't wait for input anymore
    
    do {
      done = waitpid(son_pid, nullptr, WNOHANG | WUNTRACED | WCONTINUED);
    } while(done == 0);
  }

  if(son_pid == 0)
  {
    exit(1); //In case we ran the command on a fork, we want to delete the fork
  }
}

/* JOBBBBBBBBBBBB */
void JobsList::printJobsList()
{
  for(auto job: this->jobslist)
  {
    time_t time_ = difftime(time(NULL), job->start_time) + job->work_time;
    if(job->isStopped)
    {
      time_ = job->work_time;
    }
    std::cout << "["<<job->job_id << "] " << job->cmd->getCmdLine() << " : " << job->pid << " " << time_ << " secs";
    if(job->isStopped)
    {
      std::cout << " (stopped)";
    }
    std::cout << std::endl;
  }
}
void JobsList::removeFinishedJobs() 
{
  std::vector<JobEntry*>& joblist = SmallShell::getInstance().joblist->jobslist;
  for(auto it = joblist.begin(); it != joblist.end(); )
  {
    if(waitpid((*it)->pid, nullptr, WNOHANG )) // | WUNTRACED | WCONTINUED
    {
      joblist.erase(it);
    }
    else
    {
      ++it;
    }
  }
  
}

int JobsList::getNextJobId()
{
  int nextId = 0;
  for(auto entry : jobslist)
  {
    nextId = (nextId > entry->job_id) ? nextId : entry->job_id;
  }
  return nextId + 1;
}
void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped)
{
  this->removeFinishedJobs();
  JobsList::JobEntry* newEntry = new JobsList::JobEntry(this->getNextJobId(), cmd, pid, 0, time(NULL), isStopped);
  this->jobslist.push_back(newEntry);
}
void JobsList::killAllJobs()
{
  for(auto job : jobslist)
  {
    kill(job->pid, SIGKILL);
  }
}
JobsList::JobEntry * JobsList::getJobById(int jobId)
{
  for(auto job : jobslist)
  {
    if(job->job_id == jobId)
    {
      return job;
    }
  }
  return nullptr;
}
JobsList::JobEntry * JobsList::getLastJob()
{
  if(jobslist.size() == 0)
  {
    return nullptr;
  }
  return jobslist.at(jobslist.size() - 1);
}
JobsList::JobEntry * JobsList::getLastStoppedJob()
{
  JobEntry* max = nullptr;
  for(auto job : jobslist)
  {
    if(job->isStopped)
    {
      max = job;
    }
  }
  return max;
}

/* Command Implementations */
/* COMMAND */
Command::Command(const char* cmd_line)
{
  strcpy(this->cmd_line, cmd_line);
}
/* BUILTIN */
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) 
{
  char temp[80] = {0};
  strcpy(temp, cmd_line);
  _removeBackgroundSign(temp);
  strcpy(this->cmd_line, temp);
}

/* CHPROMPT */
ChangePromptCommand::ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ChangePromptCommand::execute()
{
  char* args[20];
  args[0] = NULL;
  int nParams = _parseCommandLine(this->cmd_line, args);
  SmallShell& instance = SmallShell::getInstance();
  if(nParams == 1)
  {
    instance.setPrompt("smash> ");
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
    instance.setPrompt(new_p);
  }
}

/* SHOWPID */
ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute()
{
  cout << "smash pid is " << SmallShell::getInstance().my_pid << std::endl;
}

/*  GETTCURRDIRCOMMAND */
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute()
{
  cout << getcwd(NULL, 0) << std::endl;
  //memory leak
}

/* CHANGECURRDIRCOMMAND */
ChangeDirCommand::ChangeDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
void ChangeDirCommand::execute()
{
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  SmallShell& instance = SmallShell::getInstance();
  char** plastPwd = instance.plastPwd;
  if(nArgs != 1)
  {
    cerr << "smash error: cd: too many arguments" << std::endl;
  }
  else
  {
    if(strcmp(args[1], "-") == 0)
    {
      if(*plastPwd == nullptr)
      {
        cerr << "smash error: cd: OLDPWD not set" << std::endl;
      }
      else
      {
        char* temp;
        temp = getcwd(NULL, 0);
        if(chdir(*plastPwd) == -1)
        {
          perror("smash error: chdir failed");
          return;
        }
        //memory leak
        *instance.plastPwd = temp;
      }
    }
    else
    {
      char* prev_dir = getcwd(NULL, 0);
      //memory leak
      if(chdir(args[1]) == -1)
      {
        perror("smash error: chdir failed");
        return;
      }
      *instance.plastPwd = prev_dir;
    }
  }
}

/* JOBS */
JobsCommand::JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
void JobsCommand::execute()
{
  JobsList* jobs = SmallShell::getInstance().joblist;
  jobs->removeFinishedJobs();
  jobs->printJobsList();
}

/* KILL */
KillCommand::KillCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void KillCommand::execute()
{
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  int jid;
  JobsList* jobs = SmallShell::getInstance().joblist;
  jobs->removeFinishedJobs();
  try
  {
    if(nArgs != 2 || _charStoInt(args[1]) > -1 || _charStoInt(args[1]) < -64)
    { // 1 <= signum <= 64
      cerr << "smash error: kill: invalid arguments" << std::endl;
    }
    else
    {
      int signum = (-1) * _charStoInt(args[1]);
      jid = _charStoInt(args[2]); 
      for (auto job : jobs->jobslist)
      {
        if(job->job_id == jid)
        {
          if(kill(job->pid, signum) == -1)
          {
            perror("smash error: kill failed");
          } 
          else
          {
            cout << "signal number " << signum << " was sent to pid " << job->pid << endl;
          }
          return;
        }
      }
      cerr << "smash error: kill: job-id " << jid << " does not exist"  << std::endl;
    }
  }
  catch(...)
  {
    cerr << "smash error: kill: invalid arguments" << std::endl;
  }
  
  
}

/*  EXTERNALCOMMAND*/

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute()
{
  bool backg = _isBackgroundComamnd(cmd_line);
  
  pid_t son_pid = fork();
  if(son_pid == -1) {
    perror("smash error: fork failed");
  }
  else if (son_pid != 0) //father
  {
    SmallShell::getInstance().joblist->addJob(this, son_pid);
    if(!backg)
    {
      SmallShell::getInstance().fg_pid = son_pid;
      int done = 0;
      do {
        done = waitpid(son_pid, nullptr, WNOHANG | WUNTRACED | WCONTINUED);
      } while(done == 0);
      if(done == -1)
      {
        //err TODO
      }
      SmallShell::getInstance().fg_pid = 0;
    }
  }
  else //son
  {
    setpgrp();
    
  _removeBackgroundSign(cmd_line);
    if(execl("/bin/bash", "bash", "-c", cmd_line, nullptr) == -1)
    {
      perror("smash error: execv failed");
    }
  }
}


/* FG IS RUNNING */
bool isFgRunning()
{
  SmallShell& instance = SmallShell::getInstance();
  pid_t fg_pid = instance.fg_pid;
  instance.joblist->removeFinishedJobs();
  auto joblist = instance.joblist->jobslist;
  for(auto job : joblist)
  {
    if(job->pid == fg_pid && !job->isStopped)
      return true;
  }
  return false;
}

/* QUITCOMMAND */
QuitCommand::QuitCommand(const char* cmd_line) :BuiltInCommand(cmd_line) {}
void QuitCommand::execute()
{
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  SmallShell& instance = SmallShell::getInstance();
  instance.joblist->removeFinishedJobs();
  if (nArgs > 0 && strcmp(args[1], "kill") == 0)
  {
    cout << "smash: sending SIGKILL signal to " << instance.joblist->jobslist.size() <<" jobs:" << endl; 
    for(auto job : instance.joblist->jobslist)
    {
      cout << job->pid << ": " << job->cmd->getCmdLine() << std::endl;
    }
  }
  instance.joblist->killAllJobs();
  exit(1); // TODO 1?
}

/* FOREGROUNDCOMMAND */
ForegroundCommand::ForegroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ForegroundCommand::execute()
{
  SmallShell& instance = SmallShell::getInstance();
  instance.joblist->removeFinishedJobs();
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  if(nArgs > 1)
  {
    cerr << "smash error: fg: invalid arguments" << endl;
    return;
  }
  JobsList::JobEntry* job;
  try
  {
    if(nArgs == 0)
    {
      job = instance.joblist->getLastJob();
      if(job == nullptr)
      {
        cerr << "smash error: fg: jobs list is empty" << endl;
        return;
      }
    }
    else
    {
      job = instance.joblist->getJobById(_charStoInt(args[1]));
      if(job == nullptr)
      {
        cerr << "smash error: fg: job-id " << _charStoInt(args[1]) <<" does not exist" << endl;
        return;
      }
    }
    cout << job->cmd->getCmdLine() << " : " << job->pid << endl;
    if(job->isStopped)
    {
      kill(job->pid, SIGCONT);
      job->start_time = time(NULL);
      job->isStopped = false;
    }
    instance.fg_pid = job->pid;
    int done = 0;
    do {
      done = waitpid(job->pid, nullptr, WNOHANG | WUNTRACED); //  | WCONTINUED???
    } while(done == 0);
    if(done == -1)
    {
      //err
    }
    instance.fg_pid = 0;
  }
  catch(...)
  {
    cerr << "smash error: fg: invalid arguments" << endl;
  }
}

/* BACKGROUNDCOMMAND */
BackgroundCommand::BackgroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
void BackgroundCommand::execute()
{
  SmallShell& instance = SmallShell::getInstance();
  instance.joblist->removeFinishedJobs();
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  if(nArgs > 1)
  {
    cerr << "smash error: bg: invalid arguments" << endl;
    return;
  }
  JobsList::JobEntry* job;
  if(nArgs == 0)
  {
    job = instance.joblist->getLastStoppedJob();
    if(job == nullptr)
    {
      cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
      return;
    }
  }
  else 
  { 
    job = instance.joblist->getJobById(_charStoInt(args[1]));
    if(job == nullptr)
    {

      cerr << "smash error: bg: job-id " << _charStoInt(args[1]) << " does not exist" << endl;
      return;   
    }
    if(job->isStopped == false)
    {
      cerr << "smash error: bg: job-id " << _charStoInt(args[1]) << " is already running in the background" << endl;
      return;
    }
  }
  kill(job->pid, SIGCONT);
  job->start_time = time(NULL);
  job->isStopped = false;
  cout << job->cmd->getCmdLine() << " : " << job->pid << endl;
}
/* TAILCOMMAND */
TailCommand::TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void TailCommand::execute()
{
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  int N = 10;
  try
  {
    if(nArgs > 2 || nArgs == 0)
    {
      throw std::exception();
    }
    else if(nArgs == 2)
    {
      N = _charStoInt(args[1]) *(-1);
      if(N < 0 || args[1][0] != '-')
      {
        throw std::exception();
      }
    }
    char file_name[80];
    strcpy(file_name, _trim(string(args[nArgs])).c_str());
    ifstream desc(file_name);
    if(!(desc.is_open()))
    {
      perror("smash error: open failed");
      return;
    }
    int nLines = 0;
    std::string temp;
    while(std::getline(desc, temp))
    {
      nLines ++;
    }
    if(desc.bad()) //the while ended before eof
    {
      perror("smash error: read failed");
      return;
    }
    ifstream desc2(file_name);
    if(!(desc2.is_open()))
    {
      perror("smash error: open failed");
      return;
    }
    int i = (nLines > N) ? nLines - N : 0;
    int new_N = nLines-i; //number of lines we should actually print
    for(int j = 0; j <= i; j++ && std::getline(desc2, temp)) {}
    if(new_N != 0) //when new_N = 0 it does wierd things
      cout << desc2.rdbuf() << flush;
  }
  catch(...)
  {
    cerr << "smash error: tail: invalid arguments" << std::endl;

  }
}

/* TOUCHCOMMAND */

TouchCommand::TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void TouchCommand::execute()
{
  char* args[20];
  int nArgs = _parseCommandLine(cmd_line, args) - 1;
  if(nArgs != 2)
  {
    cerr << "smash error: touch: invalid arguments" << endl;
    return;
  }
  
  std::string array[6];
  int i=0;
  std::string input(args[2]);
  std::istringstream time_args(input);
  while(std::getline(time_args,array[i++],':')) {}
  struct tm* time_time = (struct tm*)malloc(sizeof(*time_time));
  time_time->tm_sec = _charStoInt(array[0].c_str());
  time_time->tm_min = _charStoInt(array[1].c_str());
  time_time->tm_hour = _charStoInt(array[2].c_str());
  time_time->tm_mday = _charStoInt(array[3].c_str());
  time_time->tm_mon = _charStoInt(array[4].c_str())-1;
  time_time->tm_year = _charStoInt(array[5].c_str())-1900;
  time_time->tm_isdst = -1;
  time_time->tm_wday = 0;
  time_time->tm_yday = 0;
  time_t Time = mktime(time_time);
  struct utimbuf* times = (struct utimbuf*)malloc(sizeof(*times));
  times->actime = Time;
  times->modtime = Time;
  if(utime(_trim(string(args[1])).c_str(), times) == -1)
  {
    perror("smash error: utime failed");
  }
}