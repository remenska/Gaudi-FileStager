#ifndef STAGEDDATASTREAMTOOL_H
#define STAGEDDATASTREAMTOOL_H 1
// Include files
#include <vector>
// from Gaudi
#include "GaudiKernel/Service.h"
#include "GaudiKernel/DataStreamTool.h"
#include "FileStager/IFileStagerSvc.h"
// class IIncidentSvc;
class IToolSvc;
class IInputStreamParser;
/** @class StagedDataStreamTool StagedDataStreamTool.h
 * A new DataStreamTool implementation which overrides the addStreams method to make the
 * File Stager aware of the Input Streams. It uses an instance of IInputStreamParser to extract the 
 * streams in a format convenient for staging.
 */


class GAUDI_API StagedDataStreamTool: public DataStreamTool {
public:
  StagedDataStreamTool( const std::string& type,
                        const std::string& name,
                        const IInterface* parent );

   /**  Overrides the DataStreamTool::addStreams() and sets the input streams for the FileStager service as well. 
    *  Uses an InputStreamParser implementation to extract the streams in a format convenient for 
    *  staging for the FileStager service.
    *  
    *  @see DataStreamTool
    */
  virtual StatusCode addStreams(const StreamSpecs & inputs);
  
  /// standard destructor
   virtual ~StagedDataStreamTool( );
 
   /**  Overrides the DataStream::initialize() function.
    *  Initializes the neccessary services for the StagedDataStreamTool implementation.
    *  
    *  @see DataStreamTool
    */
  virtual StatusCode initialize();
  
  /**  Overrides the DataStream::finalize() function.
    *  Finalizes the InputStreamParser tool used for extracting the streams 
    *  from the InputStream field of the EventSelector
    *  
    *  @see DataStreamTool
    */
  virtual StatusCode finalize();
protected:

  /// pointer to the FileStagerSvc interface, for adding the input streams to the File Stager
  SmartIF<IFileStagerSvc>     m_stagerSvc;
  /// pointer to the ToolSvc interface, necessary to locate the InputStreamParser
  SmartIF<IToolSvc>     m_toolSvc;
  /// pointer for the InputStreamParser interface, called for extracting the streams
  /// before passing them to the FileStagerSvc
  IInputStreamParser*      m_linktool;
private:
  StreamSpecs m_parsedInputs;

};
#endif // STAGEDDATASTREAMTOOL_H
