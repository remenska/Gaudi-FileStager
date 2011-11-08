#define _LARGEFILE64_SOURCE
#include "StageManager.h"
#include <fcntl.h>
#include "gfal_api.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <fstream>
#include <libgen.h>
#include <assert.h>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/Service.h"
#include "GaudiKernel/GaudiException.h"

#include <boost/range.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string.hpp>

#define _XOPEN_SOURCE 600
#include <sys/time.h>

extern "C" {
#include "lcg_util.h"
}
StagerInfo StageManager::s_stagerInfo;

namespace ba = boost::algorithm;
using boost::iterator_range;


//====================================================

StageManager::StageManager()
    : m_submittedGarbageCollector(false)
    , m_keepLogfiles(true)
, m_msg(0) {
  m_stageMap.clear();
  m_toBeStagedList.clear();
  MsgStream log(m_msg, "StageManager");
}


//====================================================

void StageManager::setOutputLevel(int new_level) {
  m_outputLevel = new_level;
}


StageManager::~StageManager() {
  print();
  releaseAll();

  if (s_stagerInfo.tmpdir.compare(s_stagerInfo.baseTmpdir)!=0)
    rmdir(s_stagerInfo.tmpdir.c_str());
  m_toBeStagedList.clear();
  m_stageMap.clear();
}


//====================================================

StageManager&
StageManager::instance() {
  static StageManager _instance;
  return _instance;
}


void
StageManager::addToList(const std::string& filename) {
  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  string input(filename);
  trim(input);

  log << MSG::DEBUG << "addToList() : <" << input << ">"<< endmsg;

  if (m_stageMap.find(input)==m_stageMap.end()) {
    list<string>::iterator itrF = find(m_toBeStagedList.begin(),m_toBeStagedList.end(),input);
    if (itrF==m_toBeStagedList.end()) {
      m_toBeStagedList.push_back(input);
    }
  } // adding filename to m_toBeStagedList, if not present in m_stageMap

  stageNext();
}


//====================================================

void
StageManager::releaseFile(const std::string& fname) {
  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  std::string tmpname(fname);
  trim(tmpname);
  fixRootInPrefix(tmpname);
  const std::string filename(tmpname);

  // first update status
  updateStatus();

  list<string>::iterator itrF = find(m_toBeStagedList.begin(),m_toBeStagedList.end(),filename);
  if(itrF!=m_toBeStagedList.end()) {
    m_toBeStagedList.erase(itrF);
    return;
  }

  if(m_stageMap.find(filename)==m_stageMap.end())
    return;

  if (m_stageMap[filename].status==StageFileInfo::RELEASED) {
    return;
  }
  log << MSG::DEBUG << "releaseFile() : " << filename << endmsg;

  if (m_stageMap[filename].status==StageFileInfo::STAGING) {
    // kill process first
    int  killReturn = kill(m_stageMap[filename].pid, SIGKILL);

    if( killReturn == ESRCH) {
      // pid does not exist
      log << MSG::ERROR << "Group does not exist!"<< filename << endmsg;

      // assume file is done staging
      // fill statistics first
      stat(m_stageMap[filename].outFile.c_str(),&(m_stageMap[filename].statFile));
      m_stageMap[filename].status=StageFileInfo::RELEASED;
    } else if( killReturn == EPERM) {
      // No permission to send kill signal
      // This shouldn't happen
      log << MSG::ERROR << "No permission to send kill signal for child process!"<< endmsg;

      m_stageMap[filename].status=StageFileInfo::RELEASED;
    } else {
      log << MSG::INFO << "Kill signal sent. All Ok!"<< endmsg;
      m_stageMap[filename].status=StageFileInfo::RELEASED;
    }
  }

  if (m_stageMap[filename].status==StageFileInfo::ERRORSTAGING) {
    m_stageMap[filename].status=StageFileInfo::RELEASED;
  }

  if (m_stageMap[filename].status==StageFileInfo::STAGED) {
    m_stageMap[filename].status=StageFileInfo::RELEASED;
  }

  removeFile(m_stageMap[filename].outFile);

}


void StageManager::releaseAll() {
  m_toBeStagedList.clear();

  map<string,StageFileInfo>::iterator itr = m_stageMap.begin();
  for (; itr!=m_stageMap.end(); itr=m_stageMap.begin()) {
    string first = (itr->first);
    releaseFile(first.c_str());
    m_stageMap.erase(itr);
  }
}


//====================================================

void
StageManager::getFile(const std::string& fname) {
  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  std::string tmpname(fname);
  trim(tmpname);
  fixRootInPrefix(tmpname);
  const std::string filename(tmpname);

  string name(filename);
  trim(name);
  removePrefixOf(name);

  log << MSG::DEBUG << "getFile() : " << filename << endmsg;

  // first update status
  updateStatus();

  // file not found ->  start staging immediately
  list<string>::iterator itrF = find(m_toBeStagedList.begin(),m_toBeStagedList.end(),filename);
  if (m_stageMap.find(filename)==m_stageMap.end() &&
      (itrF==m_toBeStagedList.end()) ) {
    log << MSG::DEBUG << "getFile() : " << filename
    << " not found. Start immediate staging." << endmsg;

    m_toBeStagedList.push_front(filename);
  } // filename not in m_stageMap and not in m_toBeStagedList

  // file needs to be staged
  itrF = find(m_toBeStagedList.begin(),m_toBeStagedList.end(),filename);
  if (itrF!=m_toBeStagedList.end() &&
      m_stageMap.find(filename)==m_stageMap.end()) {
    // move file to front
    m_toBeStagedList.erase(itrF);
    m_toBeStagedList.push_front(filename);

    log << MSG::DEBUG << "getFile() : " << filename
    << ". Forced staging." << endmsg;
    stageNext(true); // forced stage
  } //forced staging

  //child(pID) exists
  if (m_stageMap.find(filename)!=m_stageMap.end()) {
    log << MSG::INFO << "getFile() : Checking staging status of " << filename  << endmsg;

    // file still staging
    if (m_stageMap[filename].status==StageFileInfo::STAGING) {

      // check status
      pid_t pID = m_stageMap[filename].pid;
      int childExitStatus;

      // wait till staging is done

      log << MSG::INFO << "getFile()   : Waiting till <"
      << filename << "> is staged." << endmsg;

      waitpid( pID, &childExitStatus, 0);

      //       log << MSG::DEBUG << "Passed the waitpid(,,0)" << "for "<< filename << endmsg;

      if( !WIFEXITED(childExitStatus) ) {

        log << MSG::WARNING << "getFile()::waitpid() "<<pID
        <<" exited with status= "<< WEXITSTATUS(childExitStatus) << endmsg;
        m_stageMap[filename].status = StageFileInfo::ERRORSTAGING;
      } else if( WIFSIGNALED(childExitStatus) ) {
        log << MSG::WARNING << "getFile()::waitpid() " <<pID
        <<" exited with signal: " << WTERMSIG(childExitStatus)<< endmsg;
        m_stageMap[filename].status = StageFileInfo::ERRORSTAGING;
      } else {
        //lcg-rep ends up always here
        // child exited okay
        log << MSG::DEBUG << "getFile()::waitpid() okay for file "
        <<filename<<". WIFEXITED = "<<WEXITSTATUS(childExitStatus)
        <<", exitStatus="<<childExitStatus<< endmsg;
      }

      int ret = stat(m_stageMap[filename].outFile.c_str(),&(m_stageMap[filename].statFile));
      if( 0 == ret) {
        //      bool fexists = fileExists(m_stageMap[filename].outFile.c_str()); //TODO: remove function
        log << MSG::INFO << "Local file size:"
        << m_stageMap[filename].statFile.st_size << endmsg;

        log << MSG::INFO << "Original file size:"
        << m_stageMap[filename].originalFileSize << endmsg;

        if (m_stageMap[filename].originalFileSize > m_stageMap[filename].statFile.st_size) {
          m_stageMap[filename].status = StageFileInfo::ERRORSTAGING;
          log << MSG::ERROR << "File only partialy staged, probably "
          << " due to lack of free disk space in the process of staging. " << endmsg;
          // 	  stageNext(true); //force staging again
        } else {
          m_stageMap[filename].status = StageFileInfo::STAGED;
          // TODO: replicating turned off temporarily
          //       else
          //         replicateNext(true);
        }
      } else {
        log << MSG::ERROR << "File does not exist on local storage. "<< endmsg;
        m_stageMap[filename].status = StageFileInfo::ERRORSTAGING;
      }

    } else if(m_stageMap[filename].status==StageFileInfo::REPLICATING) {
      // check status
      pid_t pID = m_stageMap[filename].pid;
      int childExitStatus;
      // wait till staging is done
      log << MSG::INFO << "getFile()   : Waiting till <"
      << filename << "> is replicated." << endmsg;
      waitpid( pID, &childExitStatus, 0);

      if( !WIFEXITED(childExitStatus) ) {
        log << MSG::WARNING << "getFile()::waitpid() "
        <<pID<<" exited with status= "<< WEXITSTATUS(childExitStatus) << endmsg;
        m_stageMap[filename].status = StageFileInfo::ERRORREPLICATION;
      } else if( WIFSIGNALED(childExitStatus) ) {

        log << MSG::WARNING << "getFile()::waitpid() "
        <<pID <<" exited with signal: " << WTERMSIG(childExitStatus)<< endmsg;
        m_stageMap[filename].status = StageFileInfo::ERRORREPLICATION;

      } else {
        //lcg-rep ends up always here
        // child exited okay
        log << MSG::INFO << "getFile()::waitpid() okay for file "
        <<filename<<". WIFEXITED = "<<WEXITSTATUS(childExitStatus)
        <<", exitStatus="<<childExitStatus<< endmsg;

      }

      log << MSG::INFO << "before replicaExists " << endmsg;
      bool replicaexists = replicaExists(filename);
      if (replicaexists)
        m_stageMap[filename].status = StageFileInfo::REPLICATED;
      else
        m_stageMap[filename].status = StageFileInfo::ERRORREPLICATION;
    } // filename status == REPLICATING

    // file is staged
    if (m_stageMap[filename].status == StageFileInfo::STAGED) {
      log << MSG::INFO << "getFile() : <"
      << filename << "> finished staging." << endmsg;

      //       stat(m_stageMap[filename].outFile.c_str(),&(m_stageMap[filename].statFile));
      // start next stage
      stageNext();
      return;
    } else if(m_stageMap[filename].status == StageFileInfo::REPLICATED) {
      log << MSG::INFO << "getFile() : <" << filename
      << "> finished replicating." << endmsg;
      stageNext();
      return;
    } else {
      log << MSG::ERROR << "getFile() : ERROR : staging/replicating of <"
      << filename << "> failed. Giving up. Original file location will be used."<< endmsg;
    }
    stageNext();
  } // child exists, waitpid - to finish staging
}

//====================================================


const string
StageManager::getTmpFilename(const std::string& filename) {
  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  if (m_stageMap.find(filename)!=m_stageMap.end())
    if (!m_stageMap[filename].outFile.empty())
      return m_stageMap[filename].outFile.c_str();

  string infile(filename);
  trim(infile);
  string tmpfile(filename);
  trim(tmpfile);
  string::size_type pos = tmpfile.find_last_of("/:");

  string dir;
  if(m_stageMap[filename].fallbackStrategy == StageFileInfo::NONE)
    dir = s_stagerInfo.tmpdir;
  else
    dir = s_stagerInfo.fallbackDir;

  tmpfile = dir + "/tcf_" +  tmpfile.substr(pos+1,tmpfile.size()-pos-1);
  log << MSG::DEBUG << "getTmpFilename() : <"
  << tmpfile << "> <"<< tmpfile.c_str() << ">"<< endmsg;
  return tmpfile;
}

//====================================================


StageFileInfo::Status
StageManager::getStatusOf(const std::string& filename, bool update) {
  if (update)
    updateStatus();

  list<string>::iterator itrF = find(m_toBeStagedList.begin(),m_toBeStagedList.end(),filename);
  if(itrF!=m_toBeStagedList.end()) {
    return StageFileInfo::TOBESTAGED;
  }

  if(m_stageMap.find(filename)!=m_stageMap.end())
    return m_stageMap[filename].status;

  return StageFileInfo::UNKNOWN;
}

//====================================================


void
StageManager::print() {

  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  updateStatus();
  StageFileInfo::Status status;
  string filename;
  long sumsize(0);
  long sumfiles(0);

  list<string>::iterator itrb = m_toBeStagedList.begin();
  for (; itrb!=m_toBeStagedList.end(); ++itrb) {
    filename = *itrb;
    status = getStatusOf(filename.c_str(),false);
    string input(filename);
    trim(input);

    log << MSG::ALWAYS << "Status <" << filename << "> : " << status << endmsg;

  }

  map<string,StageFileInfo>::iterator itr = m_stageMap.begin();
  for (; itr!=m_stageMap.end(); ++itr) {
    filename = (itr->first);
    status = getStatusOf(filename.c_str(),false);
    log << MSG::ALWAYS << "Status <" << filename << "> : " << status << endmsg;
    if (status == StageFileInfo::RELEASED ||
        status == StageFileInfo::STAGED) {

      log << MSG::ALWAYS << (itr->second).statFile.st_atime << " "
      << (itr->second).statFile.st_mtime << " "
      << (itr->second).statFile.st_ctime << " "
      << (itr->second).statFile.st_size  << " "<< endmsg;

      sumsize += (itr->second).statFile.st_size ;
      sumfiles++;
    }
  }

  log << MSG::ALWAYS << "print() : Successfully staged "
  << sumsize/(1024*1024)<< " mb over "<< sumfiles<< " files."<< endmsg;

}

//====================================================
int StageManager::getNstaging() {
  int nentries(0);

  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  map<string,StageFileInfo>::iterator itr = m_stageMap.begin();
  for (; itr!=m_stageMap.end(); ++itr) {
    if ((itr->second).status == StageFileInfo::STAGING || (itr->second).status == StageFileInfo::REPLICATING)
      nentries++;
  }

  log <<  MSG::DEBUG << "getNstaging() = " << nentries << endmsg;

  return nentries;
}

//====================================================
StatusCode StageManager::getLocalHandle(const std::string& dataset, std::string & dataset_local) {
  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  log << MSG::DEBUG << "Searching for a local handle for "<< dataset << endmsg;
  std::map<string,StageFileInfo>::iterator iCollection = m_stageMap.find( dataset );

  if(iCollection==m_stageMap.end())
    return StatusCode::FAILURE;

  log << MSG::DEBUG << " Exists in a stagemap! "<< endmsg;
  log << MSG::DEBUG <<"Status: " << m_stageMap[dataset].status <<endmsg;

  if(m_stageMap[dataset].status == StageFileInfo::REPLICATED)
    dataset_local = s_stagerInfo.infilePrefix+m_stageMap[dataset].outFile;
  else if(m_stageMap[dataset].status == StageFileInfo::STAGED)
    dataset_local = s_stagerInfo.outfilePrefix+m_stageMap[dataset].outFile;
  else
    return StatusCode::FAILURE;

  return StatusCode::SUCCESS;
}

//====================================================
void StageManager::replicateNext(bool forceReplication) {

  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  log <<  MSG::INFO << "replicateNext()" << endmsg;
  // update status
  updateStatus();

  if (( (getNstaging()<s_stagerInfo.pipeLength) || forceReplication) &&
      (!m_toBeStagedList.empty()) ) {

    string cf = *(m_toBeStagedList.begin());

    if (forceReplication) {
      log <<  MSG::DEBUG << "replicateNext() : "
      << " forcing replication of <" << cf << ">." << endmsg;
    }

    log <<  MSG::DEBUG << "replicateNext() : "
    << "Now replicating  <" << cf << ">."  << endmsg;

    m_toBeStagedList.erase(m_toBeStagedList.begin());

    m_stageMap[cf] = StageFileInfo();
    m_stageMap[cf].status = StageFileInfo::REPLICATING;
    string inFile(cf);
    trim(inFile);

    removePrefixOf(inFile);
    m_stageMap[cf].inFile = inFile;

    log <<  MSG::DEBUG << "replicateNext():about to fork"  << endmsg;
    iterator_range< string::iterator > inFileCorrected;
    //lcg-rep does not like LFN: just lfn:....
    if ( inFileCorrected = ba::find_first( m_stageMap[cf].inFile , "LFN:" ) )
      ba::to_lower( inFileCorrected );

    ba::replace_first(m_stageMap[cf].inFile , "lfn:/lhcb", "lfn:/grid/lhcb");

    char* src_file = new char[1024];
    strcpy(src_file,m_stageMap[cf].inFile.c_str());
    char *error_buf = new char[s_stagerInfo.errbufsz];

    log <<MSG::INFO<<"About to call lcg-rep "<<endmsg;

    if( 0 == (m_stageMap[cf].pid=fork()) ) {
      // Code only executed by child process
      log <<  MSG::DEBUG << "replicateNext:this is child process "
      << getpid() << " "<< m_stageMap[cf].pid << endmsg;
      log <<MSG::INFO<<"LCG_REP Timeout: "<<s_stagerInfo.timeout <<endmsg;
      //lcg-rep
      int rc = lcg_repxt(src_file,
                         s_stagerInfo.dest_file,
                         s_stagerInfo.vo,
                         0,
                         s_stagerInfo.gridFTPstreams,
                         0,
                         0,
                         s_stagerInfo.verbose,
                         s_stagerInfo.timeout,
                         error_buf,
                         s_stagerInfo.errbufsz);

      if(rc==0) {  //lcg_rep exited without errors

        log << MSG::INFO <<"lcg-rep succesful for file: " << src_file <<endmsg;
        _exit(0);
      } else { //lcg_rep exited with error!
        log << MSG::FATAL << "Error with lcg_rep utility!" << endmsg;
        log <<MSG::ERROR << " Error message: " << error_buf <<endmsg;
        m_stageMap[cf].status = StageFileInfo::ERRORREPLICATION;
        _exit(1);
      }
    } else { // code executed by parent
      log <<  MSG::DEBUG << "replicateNext:this is parent process"
      << ", pid of child = " << m_stageMap[cf].pid << endmsg;
    }
  }
}


//====================================================
void
StageManager::stageNext(bool forceStage) {

  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  log <<  MSG::DEBUG << "stageNext()" << endmsg;
  // update status
  updateStatus();

  if (!m_submittedGarbageCollector) {
    submitGarbageCollector();
    m_submittedGarbageCollector=true;
  }

  if (( (getNstaging()<s_stagerInfo.pipeLength) || forceStage) &&
      (!m_toBeStagedList.empty()) ) {
    string cf = *(m_toBeStagedList.begin());

    if (forceStage) {
      log <<  MSG::DEBUG << "stageNext() : forcing stage of <" << cf << ">." << endmsg;
    }

    log <<  MSG::DEBUG << "stageNext() : Now staging  <" << cf << ">."  << endmsg;


    m_stageMap[cf] = StageFileInfo();

    if(!checkLocalSpace()) { // local space not sufficient
      log <<  MSG::INFO << "Can't use local disk space. "
      << "/*Switching to shared storage...*/" << endmsg;
      m_stageMap[cf].fallbackStrategy = StageFileInfo::SHARED_DIR;
      if (!checkLocalSpace()) { //shared NFS storage not sufficient
        log << MSG::INFO << "Can't use shared disk space. "
        <<"/*Switching to replication...*/" <<endmsg;
        m_stageMap[cf].fallbackStrategy = StageFileInfo::REPLICATION;
        replicateNext();
        return;
      }
    }

    m_stageMap[cf].status = StageFileInfo::STAGING;
    m_toBeStagedList.erase(m_toBeStagedList.begin());

    string inFile(cf);
    trim(inFile);
    removePrefixOf(inFile);
    m_stageMap[cf].inFile = inFile;
    m_stageMap[cf].outFile = getTmpFilename(cf.c_str());

    log <<  MSG::DEBUG << "stageNext() : outFile = <"
    << m_stageMap[cf].outFile << ">." << endmsg;
    log <<  MSG::DEBUG << "stageNext():about to fork"  << endmsg;


    if( 0 == (m_stageMap[cf].pid=fork()) ) {
      // Code only executed by child process
      log <<  MSG::DEBUG << "stageNext:this is child process "
      << getpid() << " "<< m_stageMap[cf].pid << endmsg;

      log <<  MSG::DEBUG << "stageNext:child process : outFile = <"
      << m_stageMap[cf].outFile << ">."  << endmsg;
      log <<  MSG::DEBUG << "stageNext:child process : tmpFile = <"
      << getTmpFilename(cf.c_str()) << ">."  << endmsg;

      int nargs = 4 + int(s_stagerInfo.cparg.size());
      const int argsc = 14;
      pchar args[argsc];

      for (int i=0; i<nargs-1; ++i) {
        args[i] = new char[1024];
      }

      strcpy(args[0],s_stagerInfo.cpcommand.c_str());
      for (int i=1; i<=int(s_stagerInfo.cparg.size()); ++i)
        strcpy(args[i],(s_stagerInfo.cparg[i-1]).c_str());


      iterator_range< string::iterator > inFileCorrected;

      //lcg-cp does not like LFN: just lfn:....
      if ( inFileCorrected = ba::find_first( m_stageMap[cf].inFile , "LFN:" ) )
        ba::to_lower( inFileCorrected );

      ba::replace_first(m_stageMap[cf].inFile , "lfn:/lhcb", "lfn:/grid/lhcb");

      strcpy(args[nargs-3],m_stageMap[cf].inFile.c_str());

      string outTmpfile = s_stagerInfo.outfilePrefix + m_stageMap[cf].outFile;
      strcpy(args[nargs-2],outTmpfile.c_str());
      strcpy(args[nargs-2],outTmpfile.c_str());

      log <<  MSG::DEBUG << "stageNext:child processs <"
      << outTmpfile.c_str() << "> <" << outTmpfile << ">"  << endmsg;
      //       args[nargs-1] = (char *) 0;

      log <<  MSG::DEBUG << "stageNext:child processs is calling lcg-cp " << endmsg;
      //       << s_stagerInfo.cpcommand.c_str()
      //       << " with args: \n" ;

      //       for (int i=0; i<nargs-1; ++i) {
      //         if (args[i])
      //           log <<  MSG::DEBUG << args[i] << endmsg;
      //       }
      char* src_file = args[nargs-3];
      char* dest_file = args[nargs-2];
      char *error_buf = new char[s_stagerInfo.errbufsz];

      log <<MSG::DEBUG<<"About to call lcg-cp "<<endmsg;
      // lcg-cp
      int rc = lcg_cpxt(src_file,
                        dest_file,
                        s_stagerInfo.vo,
                        s_stagerInfo.gridFTPstreams,
                        0, 0,
                        s_stagerInfo.verbose,
                        s_stagerInfo.timeout,
                        error_buf,
                        s_stagerInfo.errbufsz);

      if(rc==0) {
        log << MSG::INFO << "File "<<args[nargs-3] <<" correctly copied to "
        << "local filesystem: " <<args[nargs-2]<< endmsg;

        _exit(0);
      } else {
        log << MSG::FATAL << "Error with lcg_cp utility!" << endmsg;
        log <<MSG::ERROR << " Error message: " << error_buf <<endmsg;

        _exit(1);
      }

    } else { //child process doing replication


      log <<  MSG::DEBUG << "stageNext:this is parent process"
      << ", pid of child = " << m_stageMap[cf].pid << endmsg;
    }
  }
}


//====================================================
void
StageManager::updateStatus() {

  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  log <<  MSG::DEBUG << "updateStatus()"<< endmsg;

  map<string,StageFileInfo>::iterator itr = m_stageMap.begin();
  for (; itr!=m_stageMap.end(); ++itr) {
    if ((itr->second).status == StageFileInfo::STAGING) {

      pid_t pID = (itr->second).pid;

      int childExitStatus;
      waitpid( pID, &childExitStatus, WNOHANG);

      if( WIFEXITED(childExitStatus) ) {
        // done staging
        log <<  MSG::DEBUG << "updateStatus::waitpid() "
        <<pID
        <<" still staging. Returned with: Status= "
        << WEXITSTATUS(childExitStatus)
        << " and Exit= "
        << WIFEXITED(childExitStatus)<< endmsg;
      } else if( WIFSIGNALED(childExitStatus) ) {
        log <<  MSG::DEBUG << "updateStatus::waitpid() "
        <<pID
        <<" still staging. Returned with: Signal= "
        << WTERMSIG(childExitStatus)
        << " and Exit= "
        << WIFSIGNALED(childExitStatus)<< endmsg;

      }
    } // files in m_stageMap with status==STAGING
  }
}


//====================================================
void
StageManager::submitGarbageCollector() {

  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  log <<  MSG::DEBUG << "submitGarbageCollector()" << endmsg;
  if (s_stagerInfo.gc_command.empty())
    return;

  // pass parent pid to stagemonitor for monitoring
  int ppid = getpid();

  int cpid(0);
  if( (cpid=fork() == 0) ) {
    // Code only executed by child process
    log <<  MSG::DEBUG << "GarbageCollector:this is child process "
    << getpid() << endmsg;

    //The setsid() function creates a new session;

    pid_t pgid = setsid(); // or setpgid(getpid(), getpid()); ?
    if( pgid < 0) {
      log <<  MSG::FATAL << "Failed to create a new session"<< endmsg;
      _exit(0);
    }

    const int nargs = 6;
    pchar args[nargs];
    for (int i=0; i<nargs-1; ++i) {
      args[i] = new char[1024];
    }

    strcpy(args[0],s_stagerInfo.gc_command.c_str());
    sprintf(args[1],"%d",ppid);
    strcpy(args[2],s_stagerInfo.tmpdir.c_str());
    strcpy(args[3],s_stagerInfo.baseTmpdir.c_str());
    if (m_keepLogfiles)
      strcpy(args[4],"1");
    else
      strcpy(args[4],"0");
    args[5]=(char *) 0;


    log <<  MSG::DEBUG << "GarbageCollector::child processs is executing execv "
    << s_stagerInfo.gc_command
    << " with args "<< endmsg;

    for (int i=0; i<nargs-1; ++i) {
      if (args[i]) {
        log << MSG::DEBUG<< args[i] << " "<< endmsg;
      }
    }

    int ret = execvp(s_stagerInfo.gc_command.c_str(), args);
    // execvp should never return -- if it does, we couldn't find the command!!
    log << MSG::ERROR << " execvp failed " << strerror(errno) << endmsg;
    ::abort();
  } else {
    // Code only executed by parent process
    log <<  MSG::DEBUG << "GarbageCollector::this is parent process"
    << ", pid of child = " << cpid  << endmsg;

  }

}

//====================================================
void
StageManager::trim(string& input) {
  // trim leading and trailing whitespace
  string::size_type position = input.find_first_not_of(" \t\n");
  if ( position == std::string::npos )
    return; // skip, it's all whitespace
  input.erase(0, position);
  position= input.find_last_not_of(" \t\n");
  if ( position != std::string::npos)
    input.erase( position+1 );
}

//====================================================
void
StageManager::removePrefixOf(string& filename) {
  string::size_type ipsize(s_stagerInfo.infilePrefix.size());
  string::size_type opsize(s_stagerInfo.outfilePrefix.size());

  if (strncmp(filename.c_str(),s_stagerInfo.infilePrefix.c_str(),ipsize)==0)
    filename = filename.substr(ipsize,filename.size()-ipsize);
  else if (strncmp(filename.c_str(),s_stagerInfo.outfilePrefix.c_str(),opsize)==0)
    filename = filename.substr(opsize,filename.size()-opsize);
}


//====================================================
void
StageManager::removeFile(string filename) {

  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  log <<  MSG::DEBUG << "removeFile() : " << filename << endmsg;

  if (remove
      (filename.c_str())==-1) {
    log <<  MSG::ERROR << "removeFile() Could not remove file: <"
    << filename
    << ">."<< endmsg;

  }
}

//====================================================
void StageManager::setBaseTmpdir(const std::string& baseTmpdir) {
  string basedir = baseTmpdir;

  string::size_type position;
  while( (position=basedir.find_last_of("/")) == basedir.size() )
    basedir.replace(position,1,""); // remove any last slashes

  s_stagerInfo.baseTmpdir = basedir;
  s_stagerInfo.setTmpdir();
}


//====================================================
void StageManager::setCpCommand(const std::string& cpcommand) {
  s_stagerInfo.cpcommand = cpcommand;
}


//====================================================
void StageManager::addCpArg(const std::string& cpargm) {
  s_stagerInfo.cparg.push_back(cpargm);
}


//====================================================
void StageManager::addRepArg(const std::string& repargm) {
  s_stagerInfo.reparg.push_back(repargm);
}

//====================================================
void StageManager::setPipeLength(const int& pipeLength) {
  s_stagerInfo.pipeLength = pipeLength;
}


//====================================================
void StageManager::setInfilePrefix(const std::string& infilePrefix) {
  s_stagerInfo.infilePrefix = infilePrefix;
}

//====================================================
void StageManager::setOutfilePrefix(const std::string& outfilePrefix) {
  s_stagerInfo.outfilePrefix = outfilePrefix;
}


//====================================================
void StageManager::setLogfileDir(const std::string& logfileDir) {
  s_stagerInfo.logfileDir = logfileDir;
}

//====================================================
void StageManager::addPrefixFix(const std::string& in, const std::string& out) {
  if (s_stagerInfo.inPrefixMap.find(in)==s_stagerInfo.inPrefixMap.end())
    s_stagerInfo.inPrefixMap[in] = out;
}

//====================================================
void StageManager::fixRootInPrefix(string& tmpname) {
  map<string,string>::iterator itr = s_stagerInfo.inPrefixMap.begin();
  for (; itr!=s_stagerInfo.inPrefixMap.end(); ++itr) {
    if(tmpname.find(itr->first)!=tmpname.npos) {
      tmpname.replace(tmpname.find(itr->first),(itr->first).size(),itr->second);
      break;
    }
  }
}

StagerInfo StageManager::getStagerInfo() {
  return s_stagerInfo;
}

//====================================================
bool StageManager::replicaExists(std::string filename) {
  MsgStream log(m_msg, "StageManager");
  char ***pfns = new char**[1024];
  int errbufsz = 1024;
  char *error_buf = new char[errbufsz];
  char* src_file = new char[1024];
  bool status = false;
  strcpy(src_file,m_stageMap[filename].inFile.c_str());
  // locate the replica, by listing all available ones in pfns array
  int rc1 = lcg_lr3(src_file,
                    0,
                    s_stagerInfo.vo,
                    pfns,
                    s_stagerInfo.verbose,
                    error_buf,
                    errbufsz);

  if(rc1==0) { //lcg_lr exited without errors, pfns should contain all replica strings
    log << MSG::INFO << "replicaExists: lcg-lr success"<< endmsg;
    int i = 0;
    while ( pfns[0][i]!=NULL) {
      m_stageMap[filename].outFile = pfns[0][i];
      if(m_stageMap[filename].outFile.find(s_stagerInfo.dest_file)!=string::npos) { //substring match for "tbn18.nikhef.nl"
        log << MSG::INFO << "Local replica found: "
        <<  m_stageMap[filename].outFile << endmsg; //this is our "local" replica winner
        status = true;
        break;
      }
      i++;
    }
  } else { //lcg_lr exited with error
    log << MSG::ERROR << " error with lcg-lr: "
    << error_buf << "errno: " <<strerror(errno) << endmsg;
    log << MSG::ERROR << " Try using lcg-lr "
    << filename << " manually to diagnoze the problem "
    <<endmsg;
    m_stageMap[filename].status = StageFileInfo::ERRORREPLICATION;
    status = false;
  }
  delete [] pfns;
  delete [] error_buf;
  delete [] src_file;
  return status;
}

//====================================================
// bool StageManager::fileExists(const std::string& fileName) {
//   struct stat info;
//   int ret = -1;
//   const char* name = fileName.c_str();
//   //get the file attributes
//   ret = stat(name, &info);
//   //TODO: ovde provererka
//   if( 0 == ret) {
//     MsgStream log(m_msg, "StageManager");
//     log.setLevel(m_outputLevel);
//     log <<  MSG::INFO << "fileExists(): "
//     << info.st_size/(1024*1024)<< " MB"
//     << endmsg;
//
//     if (0 == info.st_size )
//       return false;
//     else
//       return true;
//   } else {
//     return false;
//   }
// }

//====================================================
bool StageManager::checkLocalSpace() {
  struct statvfs info;
  struct stat64 statbuf;
  MsgStream log(m_msg, "StageManager");
  log.setLevel(m_outputLevel);
  if(m_toBeStagedList.empty())
    return false;
  string fileToStage = *(m_toBeStagedList.begin());
  // gfal_stat does not accept PFN, strip it off. LFN is converted to lfn.
  iterator_range< string::iterator > result;

  if ( result = ba::find_first( fileToStage, "LFN:" ) ) {
    ba::to_lower( result );
  } else if ( result = ba::find_first( fileToStage, "PFN:" ) ) {
    ba::erase_range( fileToStage, result );
  }
  ba::replace_first(fileToStage , "lfn:/lhcb", "lfn:/grid/lhcb");

  removePrefixOf(fileToStage);
  log << MSG::INFO << "Checking file size for: " << fileToStage << endmsg;

  int ret = -1;
  //check free user space of the tempdir before staging
  if (m_stageMap[*(m_toBeStagedList.begin())].fallbackStrategy == StageFileInfo::SHARED_DIR)
    ret = statvfs ( s_stagerInfo.fallbackDir.c_str(), &info);
  else
    ret = statvfs( s_stagerInfo.tmpdir.c_str(), &info);
  if( ret ==0 ) {
    // check size of remote file
    if (gfal_stat64 (fileToStage.c_str(), &statbuf) < 0) {
      log <<  MSG::ERROR << "Checking file size:File does not exist, or"
      << " problems with gfal library " << endmsg;
      log << MSG::ERROR <<" gfal_stat() failed with error: " << strerror(errno) << endmsg;
      return false;
    }

    log.setLevel(m_outputLevel);
    log <<  MSG::INFO << "Available disk space: "
    << (info.f_bavail*info.f_bsize)/(1024*1024)
    << " MB" << endmsg;
    log <<  MSG::INFO << "Necessary disk space: "
    << statbuf.st_size/(1024*1024) << " MB" << endmsg;

    // ----------allocate space to prevent half-staged files to fail because of insufficient disk space--------
    // Daniela: works OK, but disabled because the call takes a few seconds to write the file
    // might hamper performance

    //     int tempfd = open(getTmpFilename(fileToStage).c_str(), O_RDWR|O_CREAT, 0777);
    //     log << MSG::INFO << "About to allocate space:  "<< endmsg;
    //
    //     // Time stamp before the computations < - remove this section
    //     struct timeval tp;
    //     double sec, usec;
    //     double m_beginInputFile, m_endInputFile;
    //     double m_totalWaitTime, m_totalProcessingTime;
    //     gettimeofday( &tp, NULL );
    //     sec   = static_cast<double>( tp.tv_sec );
    //     usec = static_cast<double>( tp.tv_usec )/1E6;
    //     m_beginInputFile = sec + usec;
    //     int ret1 = posix_fallocate(tempfd,0,statbuf.st_size); //allocate space, so comparisson is not necessary any more!
    //     gettimeofday( &tp, NULL );
    //     sec = static_cast<double>( tp.tv_sec );
    //     usec = static_cast<double>( tp.tv_usec )/1E6;
    //     m_endInputFile = sec + usec;
    //     log << MSG::INFO <<"Time to allocate space = " <<(m_endInputFile-m_beginInputFile)<<" seconds"<< endmsg;
    //     // END - remove this section
    //
    //     log << MSG::INFO << "Return code: "<<ret1 << endmsg;
    //     if (ret1==0) {
    //       log << MSG::INFO << "Allocated "<< statbuf.st_size/(1024*1024) <<"MB for the next file " <<endmsg;
    //       return true;
    //     } else {
    //       log << MSG::ERROR << "Couldn't allocate enough space (" << statbuf.st_size/(1024*1024)<<" MB) for staging the next file " << endmsg;
    //       log << MSG::ERROR << "Failed with error number: "<< ret1 << endmsg;
    //       return false;
    //     }
    // END----allocate space to prevent half-staged files to fail because of insufficient disk space--------
    m_stageMap[*(m_toBeStagedList.begin())].originalFileSize = statbuf.st_size;
    return (statbuf.st_size/(1024*1024)) < (info.f_bavail*info.f_bsize)/(1024*1024) ;
  } else {
    log <<  MSG::ERROR << "Directory not available or no permission to write." << endmsg;
    //     throw GaudiException( "Checking space in " + s_stagerInfo.tmpdir + " failed.",
    //                           "filesystem exception", StatusCode::FAILURE );
    return false;
  }
  return false;
}


