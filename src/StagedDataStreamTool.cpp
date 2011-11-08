// $Id: DataStreamTool.cpp,v 1.5 2008/04/04 15:12:19 marcocle Exp $
// Include files

// from Gaudi
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/xtoa.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/Incident.h"
#include "GaudiKernel/Tokenizer.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/ISvcManager.h"
#include "GaudiKernel/IAddressCreator.h"
#include "GaudiKernel/PropertyMgr.h"
#include "GaudiKernel/EventSelectorDataStream.h"
#include "StagedDataStreamTool.h"
#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/SvcFactory.h"
#include "FileStagerSvc.h"
#include "FileStager/IFileStagerSvc.h"

using std::string;

DECLARE_TOOL_FACTORY( StagedDataStreamTool )

//====================================================
StagedDataStreamTool::StagedDataStreamTool( const std::string& type,
    const std::string& name,
    const IInterface* parent )
    : DataStreamTool ( type, name , parent )
    , m_stagerSvc(0)
    , m_toolSvc(0)
    , m_parsedInputs(0)
, m_linktool(0) {}

//====================================================
StatusCode StagedDataStreamTool::initialize() {
  MsgStream log(msgSvc(), name());
  StatusCode status = DataStreamTool::initialize();
  m_stagerSvc = serviceLocator()->service("FileStagerSvc");
  m_toolSvc = serviceLocator()->service("ToolSvc");
  if (!status.isSuccess()) {
    log << MSG::ERROR << " Could not locate the Tool Service! " << endmsg;
    return StatusCode::FAILURE;
  }

  status = m_toolSvc->retrieveTool("InputStreamParser", m_linktool);

  if (!status.isSuccess()) {
    log << MSG::ERROR << " Could not locate the LinkParserTool Service! " << endmsg;
    return StatusCode::FAILURE;
  }

  if( !m_stagerSvc.isValid() )  {
    log << MSG::FATAL << "Error retrieving FileStagerSvc." << endmsg;
    return StatusCode::FAILURE;
  } else {
    log << MSG::INFO << "Retrieved FileStagerSvc." << endmsg;
  }

  log << MSG::INFO << "Successfully initialized StagedDataStream." << endmsg;
  return StatusCode::SUCCESS;
}

//====================================================
StatusCode StagedDataStreamTool::addStreams(const StreamSpecs & inputs) {
  StatusCode status = DataStreamTool::addStreams(inputs);
  status = m_linktool->extractStreams(inputs, m_parsedInputs);
  m_stagerSvc->setStreams(m_parsedInputs);
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << " addStreams succesfull. " << endmsg;
  return status;

}

//====================================================

StatusCode StagedDataStreamTool::finalize() {
 return DataStreamTool::finalize();
}

//====================================================
StagedDataStreamTool::~StagedDataStreamTool() {

}

