#include <csignal>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <dirent.h>
//#include <sys/signal.h>

#include <string>
#include <vector>

using namespace std ;

// global dirs
string tmpdir;
string baseTmpdir;
bool keepLogfiles;
//====================================================
void term(int sig) {
  //..necessary cleanup operations before terminating
  cout << "GarbageCollector : now handling signal : " << sig << endl;

  // get dir contents ...
  vector<string> files;
  DIR *dp;
  struct dirent *dirp;
  if((dp  = opendir(tmpdir.c_str())) == NULL) {
    //cout << "Error(" << errno << ") opening " << tmpdir << endl;
  } else {
    while ((dirp = readdir(dp)) != NULL) {
      string ifile = (dirp->d_name) ;
      if (ifile.find("tcf_")!=ifile.npos)
        files.push_back(tmpdir+"/"+ifile);
    }
    closedir(dp);
  }

  // remove staged orphan files, and optionally remove log files
  for (unsigned int i=0; i<files.size(); i++) {
    if (keepLogfiles) {
      if ((files[i].rfind(".out")==files[i].size()-4) ||
          (files[i].rfind(".err")==files[i].size()-4))
        continue;
    }
    cout << "Removing: "<<files[i] << endl;
    remove
      (files[i].c_str());

  }
  if (tmpdir.compare(baseTmpdir)!=0)
    rmdir(tmpdir.c_str());
  exit(EXIT_SUCCESS);
}

//====================================================
int main(int argc, const char** argv)
//-----------------------------------
{
  if ( argc<4 ) {
    std::cout << "GarbageCollector usage: " << argv[0] << " <pid> <tmpdir> <tmpdirbase> [<keepLogfiles>]" << std::endl ;
    return 1 ;
  }

  signal(SIGTERM, term); // register a SIGTERM handler

  bool doCleanup(false);
  pid_t pID = atoi(argv[1]);
  tmpdir = argv[2];
  baseTmpdir = argv[3];
  if (argc>=5)
    keepLogfiles = (bool)atoi(argv[4]);
  else
    keepLogfiles = false;

  //cout << argv[0] << " : Monitoring process with id = " << pID << endl;
  int killReturn;

  // poll for the main process by sending a signal
  while (1) {
    killReturn = kill(pID,0);
    // it has no permission to kill that process, so it will poll until the process is gone (killReturn ==-1)
    //cout <<  argv[0] << " : " << killReturn << endl;
    if( killReturn == -1 ) {
      // pid does not exist any more --> main process killed
      doCleanup=true;
      break;
    }
    sleep(10);
  }

  if (doCleanup)
    raise(SIGTERM); // will cause term() to run
  exit(EXIT_SUCCESS);
  //    kill(getpid(),-9);

}

