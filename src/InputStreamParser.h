#ifndef INPUTSTREAMPARSER_H
#define INPUTSTREAMPARSER_H 1

// Include files

#include <vector>
#include "IInputStreamParser.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/IMessageSvc.h"
#include <boost/regex.hpp>

class IMessageSvc;
/**  @class InputStreamParser  InputStreamParser.h
 *   
 *   Extends the AlgTool class and implements IInputStreamParser 
 *   This component is responsible for extracting the file URLs from the Input property 
 *   of the FileStagerSvc service, and mapping them to the appropriate 
 *   local URLs of staged copies.
 *   The tool supports the current input file types used by the LHCb experiment.
 *   @author Daniela Remenska
 *   @version 1.0
 *   
 */

class InputStreamParser: public extends1<AlgTool,IInputStreamParser> {
public:

  //   typedef std::map<std::string,std::string> CollectionMap;
  typedef std::vector<std::string>               StreamSpecs;
  /// Standard constructor
  InputStreamParser( const std::string& type,
                     const std::string& name,
                     const IInterface* parent);
  InputStreamParser(const InputStreamParser &);
  InputStreamParser & operator= (const InputStreamParser &);

  virtual ~InputStreamParser(); 

  virtual StatusCode finalize();

  /**  Overrides the AlgTool::initialize() function.
    *  Initializes the neccessary services for the LinkParserTool implementation.
    *  
    *  @see AlgTool
    */
  virtual StatusCode initialize();


  /** Implementation of IInputStreamParser::extractStreams(..)
  *  @see InputStreamParser
  */
  virtual StatusCode extractStreams(const StreamSpecs & inputs,
                                    StreamSpecs & parsedInputs);
protected:
  StreamSpecs* m_inCollection;

  virtual StatusCode extractStream(const std::string & input);
private:

  /** In case the input file is of type COLLECTION (Event Tag Collection),
   * this method will extract the file references from the (currently root) file
   * implementation and add them to the m_inCollection list. This is a "quick-and-dirty"
   * implementation that uses the ROOT library to locate the appropriate branch carrying 
   * the information about the original input files used for creating the ETC.
   * A more elegant solution can be provided by implementing the ILinkParserTool interface
   * This function is called from LinkParserTool::extractStream() 
   * 
   * @see LinkParserTool::extractStream()
   */
  virtual StatusCode extractFileReferences(std::string input);
  boost::regex m_regex;

};
#endif //INPUTSTREAMPARSER_H
