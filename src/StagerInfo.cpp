#include "StagerInfo.h"
#include <stdlib.h>


StagerInfo::StagerInfo()
    : pipeLength(1)
    , pid(getpid())
    , baseTmpdir("/tmp")
    , tmpdir("/tmp")
    , timeout(500)
    , verbose(2)
    , dest_file("tbn18.nikhef.nl")
    , errbufsz(1024)
    , vo("lhcb")
    , gridFTPstreams(1)
    , gc_command("GarbageCollector.exe") {
  setDefaultTmpdir();

  inPrefixMap["lfn/"] = "lfn:/";
  inPrefixMap["srm/"] = "srm:/";
  inPrefixMap["sfn/"] = "sfn:/";
  inPrefixMap["gsiftp/"] = "gsiftp:/";
  inPrefixMap["rfio/"] = "rfio:/";
  inPrefixMap["http/"] = "http:/";
  inPrefixMap["file/"] = "file:/";
  inPrefixMap["ftp/"] = "ftp:/";
}


StagerInfo::~StagerInfo() {}


void StagerInfo::setTmpdir() {
  tmpdir = baseTmpdir;

  char pidchar[25];
  sprintf(pidchar,"%d",pid);
  string testtmpdir = baseTmpdir+"/"+getenv("USER")+"_pID"+pidchar;
  int errnum = mkdir(testtmpdir.c_str(),0700);

  // test the tmpdir ...
  if ( !(errnum==0 || errnum==2) ) {
    // no permissions to write in base directory, switching to /tmp
    cerr << "StagerInfo::setTmpdir() : No permission to write in temporary directory <"
    << baseTmpdir
    << ">. Switching back to default <$TMPDIR>, or else </tmp>."
    << endl;
    setDefaultTmpdir();
  } else {
    tmpdir = testtmpdir;
   cout << "Tmpdir set to " << tmpdir <<endl;
   cout << "Shared-dir set to " << fallbackDir <<endl;
  }
}


void StagerInfo::setDefaultTmpdir() {
  baseTmpdir = "/tmp";
  tmpdir = "/tmp";
  const char *vars[] = { "TMPDIR", "EDG_WL_SCRATCH","OSG_WN_TMP","WORKDIR", 0};

  std::vector<std::string> env_vars;
  env_vars.push_back("WORKDIR");
  env_vars.push_back("EDG_WL_SCRATCH");
  env_vars.push_back("OSG_WN_TMP");
  env_vars.push_back("TMPDIR");
  
  
  std::vector<std::string>::iterator varItr;
  varItr = env_vars.begin();
  std::string check_var;

 for (int i=0; i<int(env_vars.size()); i++) {
   check_var = *varItr;
    const char* val = getenv(varItr->c_str());
    if(val!=0) {
      baseTmpdir = val;
      tmpdir = val;
      break;
    }
    varItr++;
   
  }
}
