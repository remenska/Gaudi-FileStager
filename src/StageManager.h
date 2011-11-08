
#ifndef STAGEMANAGER_H
#define STAGEMANAGER_H 1

#include "StageFileInfo.h"
#include "StagerInfo.h"

#include "GaudiKernel/MsgStream.h"
#include <set>
#include <list>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "GaudiKernel/IMessageSvc.h"
#include "GaudiKernel/ISvcLocator.h"

using namespace std ;

class StagerInfo;
class IMessageSvc;
typedef char* pchar;

/**  @class StageManager  StageManager.h
 *   The core component performing the actual staging:
 *   It's a singleton class in charge of downloading the remote files specified as input by the job 
 *   by creating children processes that asynchronously call the middleware tools 
 *   responsible for downloading the files from a particular SE
 *
 *   @author Daniela Remenska
 *   @version 1.0
 */
class StageManager {
public:
  typedef std::map<std::string,std::string> CollectionMap;

  /**  Function for accessing the single instance of this class,
   *  created statically on first request
   */
  static StageManager& instance();

  /**
    *  Adds a remote file name in the _toBeStagedList list used for staging
    *  @see FileStagerSvc::loadStager()
    *  @param filename pointer to the file name
    */
  void addToList(const std::string& filename);

  /**
   *  Called by the FileStagerSvc after the job has finished processing the event data in the 
   *  corresponding input file. It releases the file by first killing the child process
   *  responsible for downloading it, in case it is still alive, and updating the status 
   *  of the file. Finaly, it removes the temporary file from the local directory, 
   *  and the log files associated with it, unless it is configured to keep them. 
   *  @see FileStagerSvc::releasePrevFile() and FileStagerSvc::handle()
   */
  void releaseFile(const std::string& fname);


  /**
   *  Releases all local temporary files by looping through the m_stageMap structure
   *  and removing each local file individually
   */
  void releaseAll();

  /** Called by the FileStagerSvc when the appropriate incident is thrown, in order to
   * setup the next local file copy for processing.
   *  If the file is not already in the _toBeStagedList list, it is immediately pushed 
   *  to the front of the list and staging begins. Otherwise it checks the status of 
   *  the staging file and waits for the child process to terminate if the file is 
   *  still staging.
   *  @param fname filename handle for the next file for processing
   */
  void getFile(const std::string& fname);

  /** Creates and returns a temporary file name handle containing
   * the path+name of the file, so that it can be refferenced locally instead
   * of its original remote location
   * @param filename filename handle of the original remote input file 
   * @return the temporary file name handle
   */
  const string getTmpFilename(const std::string& filename);

  /** Interrogates the status of a particular file with respect to staging.
    * It loops through the _toBeStagedList list and if the file name is present,
    * it returns a TOBESTAGED status. Otherwise, if the file name is already present
    * in the m_stageMap structure, the corresponding status kept in that structure
    * is returned. An UNKNOWN status is returned in case the file name is not 
    * present in any of the two structures
    * if the "update" parameter is set to "true", the status child process 
    * performing the staging will be determined with a non-waiting call.
    * 
    * @param filename remote filename handle of the file to be checked
    * @param update flag indicating weather the child process current
    * status should be checked additionally. 
    * @return the status of the interrogated file
    */
  StageFileInfo::Status getStatusOf(const std::string& filename, bool update=true);

  /** Setter method for the temporary local directory used for staging files
  *  Removes any unneccessary slashes from the file path.
  *  @param baseTmpdir the location of the base temporary directory 
  *  where the staged files will be stored. Usually /tmp or $TMPDIR
  */
  void setBaseTmpdir(const std::string& baseTmpdir);
  ///The copy command issued by the child processes for invoking the tool for staging the files.
  ///default value = "lcg-cp"

  /** Setter method for the copy command issued by the child processes
   *  for invoking the tool for staging the files.
   *  @param cpcommand the command for invoking the middleware tool for staging the files
   *  Default value is "lcg-cp"
   */

  void setCpCommand(const std::string& cpcommand);


  /** Setter method for additional command-line arguments to the copy command
   *  Can include the "-v" for verbosity, "--vo" for the virtual organization etc.
   *  @param cparg the command argument to be added to the list
   */
  void addCpArg(const std::string& cparg);

  void addRepArg(const std::string& reparg);
  /** Setter method for the pipe length indicating the number of files
   * to be staged concurrently. By default, the StageManager stages
   * one file ahead.
   *  @param pipeLength integer indicating the desired pipe length of the stage manager.
   *  @see FileStagerSvc::configStager
   */

  void setPipeLength(const int& pipeLength);

  /** Setter method for the input file prefix used in the FileStagerSvc.Input file descriptors
   *  Default value is "gfal:"
   *  @param infilePrefix string indicating the input file prefix used in the job configuration
   *  @see FileStagerSvc::configStager
   */

  void setInfilePrefix(const std::string& infilePrefix);

  /** Setter method for the output file prefix used for the locally staged files
   *  Default value is "file:"
   *  @param outfilePrefix string indicating the output file prefix to be used when opening
   *  the locally staged files for processing 
   *  @see FileStagerSvc::configStager
   */
  void setOutfilePrefix(const std::string& outfilePrefix);

  /** Setter method for the directory where the log files containing output and error messages
   * from the child processes responsible for staging.
   *  
   *  @param logfileDir string indicating the local directory for storing log files
   *  If the logFileDir value is not set by calling this function, 
   *  the base temporary directory will be used for storing the log files.
   *  @see FileStagerSvc::configStager
   */
  void setLogfileDir(const std::string& logfileDir);

  /** A workaround for incorrectly interpreted input file prefixes.
    * Adds another in/out mapping to the s_stagerInfo structure
    *  
    *  @param in the incorrect (to be fixed) protocol specification
    *  in the file name handle 
    *  @param out the corresponding correct protocol specification
    *  in the file name handle
    */
  void addPrefixFix(const std::string& in, const std::string& out);

  /// Flag for keeping the log files created by the StageManager during child process execvp call

  /** Setter method stating whether the log files should be kept after the job
   * is completed.
   *  @param k the flag 
   *  @see FileStagerSvc::configStager
   */
  void keepLogfiles(bool k=true) {
    m_keepLogfiles=k;
  }

  /** Prints a summary of the number or successfully staged files and the total size.
    */
  void print();

  /** Getter method for the s_stagerInfo member
   *  @return s_stagerInfo member reference
   */
  StagerInfo getStagerInfo();

  /** Setter method for the MsgStream output level threshold for printing information
   * to the standard output
   *  @param new_level the new level: 2=DEBUG, 3=INFO, 4=WARNING, 5=ERROR, 6=FATAL 
   *  @see MessageSvc
   */
  void setOutputLevel(int new_level);

  /** Get the local "staged" file handle mapped for a certain dataset specified in the job input.
    *  The mapping exists only if the staging is successful.
    *  Called by IFileStagerSvc::getLocalDataset
    *  If the dataset is in m_stageMap and has a STAGED (or REPLICATED) status, dataset_local will
    *   get the local file handle and StatusCode::SUCCESS will be returned. 
    *  Otherwise the function returns StatusCode::FAILURE 
    * @param dataset the original dataset
    * @param dataset_local the mapped local handle of the staged file
    * @return Status code indicating success or failure.
    */
  StatusCode getLocalHandle(const std::string& dataset,
                            std::string & dataset_local);

  void setDefaultTmpdir() {
    s_stagerInfo.setTmpdir();
  }
  void setFallbackDir(const std::string& m_fallbackDir) {
    s_stagerInfo.fallbackDir = m_fallbackDir;
  }
  void setParallelStreams(const int m_parallelStreams) {
    s_stagerInfo.gridFTPstreams = m_parallelStreams;
  }
protected:

  /// pointer to MessageSvc
  IMessageSvc* m_msg;
  //   CollectionMap m_collection_map;
private:

  /// default constructor is private (Singleton implementation)
  /// clears the m_stageMap and _toBeStaged lists
  StageManager();
  virtual ~StageManager();

  StageManager& operator= (StageManager&);

  /** Checks if the file is present in the local disk storage
   * @param fileName full file name path
   * @return bool value, true if the file size > 0
   */
//   bool fileExists(const std::string& fileName);

  bool replicaExists(std::string filename);
  /** Checks if the local temporary directory has enough disk space
   * to store the next file
   * @return bool value, true if available disk space > maxFileSize
   */
  bool checkLocalSpace();

  /** Creates a child process responsible for downloading the next file
   * from the _toBeStagedList list. It uses the copy command and arguments 
   * configured during initialization
   * @param forceStage bool value indicating whether the file should be immediatelly staged
   * instead of queued.  
   */
  void stageNext(bool forceStage=false);

  /** Issues a non-blocking call to check the status of the child processes responsible for
   * staging the files. Iterates through the m_stageMap structure to get the process id's 
   * of all child processes that are still staging files.
   */
  void updateStatus();

  void replicateNext(bool forceReplication=false);
  /**
   * Removes any leading/trailing tabs/empty spaces from a string 
   * @param input input string
   */
  void trim(string& input);

  /**
    * Removes the in/out file prefix of a file name 
    * @param filename the file name string
    */
  void removePrefixOf(string& filename);

  /**
   * Removes the file stored on local disk space
   * @param filename the file name string
   */
  void removeFile(string filename);

  /**
  * Fixes an incorrectly set prefix/protocol using the s_stagerInfo.inPrefixMap
  * structure as a rule.
  * @param filename the file name string
  */
  void fixRootInPrefix(string& tmpname);


  /**
  * Creates a child process which executes a Garbage Collector
  * A new session id of this child process is set as independent from the caller (parent),
  * so that it will become the process group leader, thus not controlled by the parent.
  * This is necessary for the Garbage Collector to run independently of the main job, 
  * in case the main job fails/hangs
  */
  void submitGarbageCollector();

  /**
   * Obtains the number of files which are currently in a STAGING state
   * @return integer value of the number of entries
   */
  int  getNstaging();

  /// string vector of the orginal remote input file names containing the files to be staged
  list< string > m_toBeStagedList;

  /// mapping of each input file to its details: status, full input file name, temporary output file name (no protocol info)
  map<string,StageFileInfo> m_stageMap;

  /// static member acting as a data structure for the configuration details of the StageManager
  static StagerInfo s_stagerInfo;
  bool m_submittedGarbageCollector;
  bool m_keepLogfiles;
  int m_outputLevel;

} ;


#endif //STAGEMANAGER_H
