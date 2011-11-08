#ifndef FILESTAGERSVC_H
#define FILESTAGERSVC_H 1
#include "GaudiKernel/Service.h"
#include "GaudiKernel/IIncidentListener.h"
#include "GaudiKernel/MsgStream.h"
#include "FileStager/IFileStagerSvc.h"
#include "StageManager.h"
#include "IInputStreamParser.h"
#include "GaudiKernel/IInterface.h"
#include <string>
#include <vector>

class StoreGateSvc;
class TStopwatch;
class TH1D;
class IDataStreamTool;
class IEvtSelector;
class IIncidentSvc;
class IChronoStatSvc;
class IDataManagerSvc;
class IInputStreamParser;
class IToolSvc;

namespace Gaudi {
  class IIncidentSvc;

  class IToolSvc;

}

/**  @class FileStagerSvc  FileStagerSvc.h
 *  File Stager Service:
 *   Implements the IFileStagerSvc interface and provides the
 *   basic staging functionality when used with a particular job. It implements the IIncidentListener
 *   in order to provide a handle() method for handling incidents of type 
 *   BeginInputFile, EndInputFile, and COLLECTION_INPUT_FILE. It is responsible for communicating with the StageManager
 *   for releasing the previously read file, and setting up the next file for staging, when such incidents are fired  
 *
 *   @author Daniela Remenska
 *   @version 1.0
 *   
 */

class FileStagerSvc : public extends2<Service, IFileStagerSvc, IIncidentListener> {
public:
  typedef std::vector<std::string>               StreamSpecs;

  /**  Default constructor.
   *   @param nam service instance name
   *   @param svcLoc pointer to servcie locator
   */
  FileStagerSvc(const std::string& nam, ISvcLocator* svcLoc);

  /**  Overrides the Service::initialize() function.
    *  Initializes the Incident Service, Tool Service, InputStreamParser service.
    *  Configures the stager, loads the input/output file name collections.
    *  Registers itself (adds Listeners) for BeginInputFile, EndInputFile and COLLECTION_INPUT_FILE incidents.
    *  
    *  @see Service
    */
  virtual StatusCode initialize();

  /** Implementation of IIncidentListener::handle method.
   *  Handles incidents of type EndInputFile and COLLECTION_INPUT_FILE. 
   *  Calls the StageManager methods for releasing the previous file and 
   *  setting up the next file on the list of files to be staged.
   *  @see IIncidentListener
   */
  void handle(const Incident& inc);

  /** Implementation of IFileStagerSvc::getLocalDataset
  *  @see IFileStagerSvc
  */
  virtual StatusCode getLocalDataset(const std::string& dataset,
                                     std::string & local);

  /**
   *  Overrides the Service::finalize() function.
   *  @see Service
   */
  virtual StatusCode finalize();

 /** Implementation of IFileStagerSvc::setStreams
  *  @see IFileStagerSvc
  */
  virtual StatusCode setStreams(const StreamSpecs & inputs);

  /**
   *  Standard destructor
   */
  virtual ~FileStagerSvc();

protected:
  ///used for declaring the "Input" Property for the service in the options of a particular job where this service is used.
  ///The input from the EventSelector should be passed to this property.

  StreamSpecs           m_streamSpecs;
  /// InputStream parser tool for extracting the file path+names from the "Input" property
  IInputStreamParser*      m_linktool;

  /// The IInputStreamParser implementation to be used (currently "InputStreamParser")
  std::string           m_streamManager;


private:

  /** Configures the stager by obtaining a reference to the StageManager and
   * setting up: its input/output file prefix, set of arguments for the copying 
   * command line tool, the temporary directory to store the staged files.
   */
  void configStager();

  /** Adds the input file name collection (m_inCollection) to the list of files-to-be-staged of the StageManager
    * Adds the output file names to the m_outCollection collection
    */
  void loadStager();

  /** Releases the file referenced by _prevFile from the list of
    * files-to-be-staged maintained by the StageManager
    */
  void releasePrevFile();

  /** Handles the next file name referenced by _fItr to be staged by the StageManager
    * _fItr will point to the next file name in the m_inCollection list.
    */
  void setupNextFile();

  ///avoid initializing this service more than once
  bool m_initialized;

  ///the fallback shared NFS directory to store the staged files, in case local WN diskspace is not sufficient
  /// default value is /project/bfys/<<username>>/. The value can be changed by setting:
  /// FileStagerSvc().FallbackDir = "/some/other/accesible/directory" in the job options
  std::string m_fallbackDir;

  /// By default, the StageManager stages one file ahead.
  /// This value can be changed in the job options file by
  /// setting FileStagerSvc().PipeSize = N
  /// to make N files available ahead, on local disk space
  int  m_pipeSize;

  /// Flag for keeping the log files created by the StageManager during child process execvp call
  bool m_keepLogfiles;

  /// Pointer to ToolSvc
  IToolSvc*     m_toolSvc;

  /// Pointer to IncidentSvc
  IIncidentSvc* m_incidentSvc;

  bool m_is_collection;
  bool m_firstFileInStream;

  ///flag for keeping the staged files, or removing them after being processed
  bool m_releaseFiles;

  ///Input file prefix used in the FileStagerSvc.Input file descriptors
  ///Removed from the input file handle before issuing the staging command (lcg-cp)
  ///default value = "gfal:"
  std::string m_infilePrefix;

  ///Output file prefix used for the staged files.
  ///default value = "file:"
  std::string m_outfilePrefix;

  ///The copy command issued by the child processes for invoking the tool for staging the files.
  ///default value = "lcg-cp"
  std::string m_cpCommand;

  ///Base temporary directory used for staging the files on a local storage.
  std::string m_baseTmpdir;
  ///If specified, the log files will be kept in a (possibly) different log file directory.
  std::string m_logfileDir;
  
  ///Additional arguments to pass to the copy command given above
  std::vector< std::string > m_cpArg;
  ///Additional arguments to pass to a replicate command given above
  std::vector< std::string > m_repArg;
  ///Vector of input file names as specified in the FileStagerSvc.Input file descriptor list
  StreamSpecs m_inCollection;

  ///Vector of output file names refering to staged files on local storage.
  StreamSpecs m_outCollection;

  ///Iterator used for looping over the m_inCollection vector and releasing/staging the appropriate files
  std::vector< std::string >::iterator m_fItr;

  ///Keeps the previous file name for releasing the appropriate file after the event data is processed
  std::string m_prevFile;

  ///Number of parallel streams to use for each file staged with the gridFTP protocol; default value is 1
  int m_parallelStreams;
};

#endif
