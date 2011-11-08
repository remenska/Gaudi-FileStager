#ifndef IINPUTSTREAMPARSER_H
#define IINPUTSTREAMPARSER_H 1

#include "GaudiKernel/IAlgTool.h"


/** @class InputStreamParser InputStreamParser.h FileStager/InputStreamParser.h
 *
 *   Interface for extracting the file URLs from the Input property of the EventSelector in a format
 *   convenient for staging by the FileStagerSvc. This interface is used by the StagedDataStreamTool.
 *   The impelementation of the tool should support all the current input file types used by the LHCb experiment.
 *   @author Daniela Remenska
 *   @version 1.0
 *
*/

class IInputStreamParser: virtual public IAlgTool {
public:

  typedef std::vector<std::string>               StreamSpecs;
  /// InterfaceID
  DeclareInterfaceID(IInputStreamParser,2,0);
  
    /**  Extracts the input streams from the StreamSpecs of the EventSelector 
      *  and loads the m_inCollection vector of this service with the original input stream handle
      *  If the input stream is of type COLLECTION, it will extract the original file references (GUIDs)
      *  used when creating the Event Tag Collection.
      *  
      * @param inputs the original input dataset
      * @param parsedInputs an array of strings for storing the parsed input streams
      * @return Status code indicating success or failure.
      */
  virtual StatusCode extractStreams(const StreamSpecs & inputs, StreamSpecs & parsedInputs) = 0;
 
protected:
  virtual StatusCode extractStream(const std::string & input) = 0;
};
#endif // IINPUTSTREAMPARSER_H
