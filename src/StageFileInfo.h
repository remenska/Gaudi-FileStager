
#ifndef STAGEFILEINFO_H
#define STAGEFILEINFO_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <string>
#include <map>

using namespace std ;

class IMessageSvc;
/**  @class StageFileInfo  StageFileInfo.h
 *   Passive component used by the StageManager for keeping track of information
 *   about each input file stream processed by the job.
 *   @version 1.0
 */
class StageFileInfo {
public:

  ///enumeration for the possible states of a file handled by the StageManager

  enum Status { UNKNOWN, TOBESTAGED, STAGING, STAGED,
                RELEASED, ERRORSTAGING, TOBEREPLICATED,
                REPLICATING, REPLICATED, ERRORREPLICATION};
  enum FallbackStrategy { NONE, SHARED_DIR, REPLICATION};
  StageFileInfo() : pid(-999),fallbackStrategy(NONE) {}
  ;
  ~StageFileInfo() {}
  ;

  ///pid of the child process responsible for staging the file
  int  pid;

  /// original input file handle, prefix removed
  string inFile;

  /// local "staged" input file handle
  string outFile;
  bool fromETC;
  /// value from StageFileInfo::Status enumeration
  Status status;
  FallbackStrategy fallbackStrategy;
  struct stat statFile;
  unsigned long originalFileSize;

  ///standard output used for redirection of stream in the child process
  string stout;

  ///standard error used for redirection of stream in the child process
  string sterr;
} ;


#endif
