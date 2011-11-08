
#ifndef STAGERINFO
#define STAGERINFO
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <set>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>
#include <map>
  
using namespace std ;

class StagerInfo
{
 public:
  StagerInfo();
  ~StagerInfo();
  void setTmpdir();
  void setDefaultTmpdir();
  int pipeLength;
  int pid;
  int gridFTPstreams;
  string infilePrefix;
  string outfilePrefix;
  string logfileDir;
  string baseTmpdir;
  string fallbackDir;
  string tmpdir;
  string cpcommand;
  int errbufsz;
  string repcommand;
  char* dest_file;
  int timeout;
  int verbose;
  char* vo;
  string gc_command;
  vector<string> cparg;
  vector<string> reparg;
  map<string,string> inPrefixMap;
} ;


#endif //STAGERINFO
