#ifndef FILESTAGER_IFILESTAGERSVC_H
#define FILESTAGER_IFILESTAGERSVC_H

#include "GaudiKernel/IInterface.h"
#include <string>

// Forward declarations
class StageManager;


/** @class IFileStagerSvc IFileStagerSvc.h FileStager/IFileStagerSvc.h
 *
 *   The interface implemented by the FileStagerSvc service.
 *
 *   @author Daniela Remenska
 *   @version 1.0
 *
*/
class GAUDI_API IFileStagerSvc: virtual public IInterface
{
public:
  /// InterfaceID
  DeclareInterfaceID(IFileStagerSvc,1,0);
  typedef std::vector<std::string>                         StreamSpecs;
  /** Get the local "staged" file handle mapped for a certain dataset specified in the job input
   *  The IODataManager uses this method to switch "on the fly" to a local handle for reading event data, if one is available
   *  If the dataset has been extracted from an ETC, a "COLLECTION_OPEN_FILE" incident should be fired.
   *
   * @param dataset the original dataset
   * @param dataset_local the mapped local handle of the staged file
   * @return Status code indicating success or failure of the operation.
   */
  virtual StatusCode getLocalDataset(const std::string& dataset, std::string & dataset_local) = 0;

  /**  Overrides the Service::initialize() function.
   *  Initializes the neccessary services for the FileStagerSvc implementation.
   *  
   *  @see Service
   */
  virtual StatusCode initialize() = 0;

  /**
   *  Overrides the Service::finalize() function.
   *  @see Service
   */
  virtual StatusCode finalize() = 0;

  /**
   *  Set the StreamSpecs to the InputStream specification of the job, to make the File Stager 
   *  aware of them.
   *  @return StatusCode indicating success or failure of the operation.
   */
  virtual StatusCode setStreams(const StreamSpecs & inputs) = 0;
};

#endif // FILESTAGER_IFILESTAGERSVC_H
