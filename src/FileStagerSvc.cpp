#include "FileStagerSvc.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/Incident.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/IToolSvc.h"
#include "GaudiKernel/Service.h"
#include "GaudiKernel/IIncidentListener.h"
#include "StageManager.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "GaudiKernel/IDataStreamTool.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <stdlib.h>


#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string.hpp>


using namespace Gaudi;
DECLARE_SERVICE_FACTORY(FileStagerSvc)

namespace ba = boost::algorithm;


FileStagerSvc::FileStagerSvc(const std::string& nam, ISvcLocator* svcLoc) :
    base_class(nam,svcLoc)
    , m_pipeSize(1)
    , m_infilePrefix("gfal:")
    , m_outfilePrefix("file:")
    , m_keepLogfiles(false)
    , m_releaseFiles(true)
    , m_toolSvc(0)
    , m_prevFile("")
    , m_incidentSvc(0)
    , m_inCollection(0)
    , m_is_collection(false)
    , m_firstFileInStream(true)
    , m_parallelStreams(1)
/*, m_fallbackDir("/project/bfys/")*/ {
  //------------------------------------------------------------------------------
  m_initialized = false;

  declareProperty("PipeSize", m_pipeSize);
  declareProperty("InfilePrefix", m_infilePrefix);
  declareProperty("OutfilePrefix", m_outfilePrefix);
  declareProperty("BaseTmpdir", m_baseTmpdir);
  declareProperty("InputCollections", m_inCollection, "vector of input files");
  declareProperty("OutputCollections", m_outCollection, "vector of output files");
  declareProperty("LogfileDir", m_logfileDir);
  declareProperty("KeepLogfiles", m_keepLogfiles);
  declareProperty("ReleaseFiles", m_releaseFiles);
  declareProperty( "StreamManager",  m_streamManager="StagedDataStreamTool");
  declareProperty( "FallbackDir", m_fallbackDir);
  declareProperty( "ParallelStreams", m_parallelStreams);
}

//====================================================

StatusCode
FileStagerSvc::initialize() {
  StatusCode status = Service::initialize();

  if( m_initialized )
    return StatusCode::SUCCESS;
  MsgStream log(msgSvc(), name());
  log << MSG::DEBUG << "Initializing....FileStagerSvcNEW" << endmsg;

  status = service("IncidentSvc", m_incidentSvc, true);
  if (!status.isSuccess()) {
    log << MSG::ERROR << "Failed to access incident service." << endmsg;
    return StatusCode::FAILURE;
  }

  status = service("ToolSvc", m_toolSvc, true);
  if (!status.isSuccess()) {
    log << MSG::ERROR << " Could not locate the Tool Service! " << endmsg;
    return StatusCode::FAILURE;
  }

  m_incidentSvc->addListener(this, IncidentType::BeginInputFile);
  m_incidentSvc->addListener(this, IncidentType::EndInputFile);

  log << MSG::DEBUG << "Added listeners on begin and end of input files." << endmsg;


  m_initialized = true;

  log << MSG::DEBUG << "Configuring File Stager Service." << endmsg;
  configStager();
  log << MSG::DEBUG << "File Stager Service configured!" << endmsg;
  loadStager();
  log << MSG::DEBUG << "Stager loaded..." << endmsg;
  setupNextFile();
  log << MSG::DEBUG << name() << ":Initialize() successful" << endmsg;

  return StatusCode::SUCCESS;
}


//====================================================
void
FileStagerSvc::handle(const Incident& inc) {
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "Handling incident '" << inc.type() << "'" << endmsg;
  log << MSG::INFO << "Incident source '" << inc.source() << "'" << endmsg;

  if (inc.type() == IncidentType::BeginInputFile) {
    if (boost::contains(inc.source(), ".root"))
      m_is_collection = true;
    else
      m_is_collection = false;
  } else if (inc.type() == IncidentType::EndInputFile) {
    if(m_releaseFiles)
      releasePrevFile();
    setupNextFile();
  }
}

//====================================================

FileStagerSvc::~FileStagerSvc() {}

//====================================================

StatusCode FileStagerSvc::finalize() {
  // /------------------------------------------------------------------------------
  MsgStream log(msgSvc(), name());
  m_toolSvc = 0;
  m_incidentSvc = 0;
  log << MSG::DEBUG << name() << ": Finalize() successful" << endmsg;
  return StatusCode::SUCCESS;
}


//====================================================

StatusCode FileStagerSvc::getLocalDataset(const std::string& dataset, std::string & dataset_local) {
  //check if the file is available
  MsgStream log(msgSvc(), name());
  StageManager& manager(StageManager::instance());

   if(m_is_collection) {
    if(!m_firstFileInStream) {
      if(m_releaseFiles)
        releasePrevFile();
      setupNextFile();
    } else {
      m_firstFileInStream=false;
    }
  }
 // StatusCode sc = StatusCode::FAILURE;
 // sc = manager.getLocalHandle(dataset, dataset_local);
  StatusCode sc = StatusCode::FAILURE;
  string dset = dataset;
  ba::replace_first(dset , "FID:", "gfal:guid:");
  log << MSG::DEBUG << "Trying to find a local handle for: " << dset << endmsg;
  sc = manager.getLocalHandle(dset, dataset_local);
  if (!sc.isSuccess()) {
    log << MSG::INFO << " could not obtain local dataset for " << dset << endmsg;
    return sc;
  }

  log << MSG::DEBUG<<" obtained local handle :" << dataset_local <<endmsg;


  return sc;
}

//====================================================
void FileStagerSvc::configStager() {
  MsgStream log(msgSvc(), name());
  log << MSG::DEBUG << "configFileStagerSvc()" << endmsg;

  StageManager& manager(StageManager::instance());
  manager.setOutputLevel(outputLevel());
  manager.setPipeLength(m_pipeSize);
  manager.setParallelStreams(m_parallelStreams);
  manager.keepLogfiles(m_keepLogfiles);

  if (!m_infilePrefix.empty())
    manager.setInfilePrefix(m_infilePrefix);
  if (!m_outfilePrefix.empty())
    manager.setOutfilePrefix(m_outfilePrefix);

  if (!m_baseTmpdir.empty())
    manager.setBaseTmpdir(m_baseTmpdir);
  else
    manager.setDefaultTmpdir();

  if (!m_logfileDir.empty())
    manager.setLogfileDir(m_logfileDir);

  if (!m_fallbackDir.empty())
    manager.setFallbackDir(m_fallbackDir);
  else
    manager.setFallbackDir("/project/bfys/"+string(getenv("USER")));

  log << MSG::INFO << "Fallback dir: " << manager.getStagerInfo().fallbackDir << endmsg;
  m_fItr = m_inCollection.begin();

}


//====================================================
void FileStagerSvc::releasePrevFile() {
  MsgStream log(msgSvc(), name());
  if (!m_prevFile.empty()) {
    StageManager& manager(StageManager::instance());
    manager.releaseFile(m_prevFile.c_str());
  }

  if (m_fItr!=m_inCollection.end()) {
    m_prevFile = *m_fItr;
  }

  log << MSG::DEBUG << "end of releasePrevFile()" << endmsg;
}


//====================================================
void FileStagerSvc::loadStager() {
  MsgStream log(msgSvc(), name());
  log << MSG::DEBUG << "loadStager()" << endmsg;

  StageManager& manager(StageManager::instance());

  m_outCollection.clear();
  std::vector< std::string >::iterator itr = m_inCollection.begin();

  // ensure deletion of first staged file
  if (!m_inCollection.empty())
    m_prevFile = m_inCollection[0].c_str();

  // add files and start staging ...

  for (; itr!=m_inCollection.end(); ++itr) {
    manager.addToList(itr->c_str());
    std::string outColl = manager.getTmpFilename(itr->c_str());
    m_outCollection.push_back( outColl );
  }
}

//====================================================

void FileStagerSvc::setupNextFile() {
  MsgStream log(msgSvc(), name());
  log << MSG::DEBUG << "setupNextFile()" << endmsg;
  if (m_fItr!=m_inCollection.end()) {
    StageManager& manager(StageManager::instance());
    // wait till file finishes staging ...
    log << MSG::DEBUG <<name()<< ": before manager.getFile()" << endmsg;
    manager.getFile(m_fItr->c_str());
    ++m_fItr;
  }

}

//====================================================
StatusCode FileStagerSvc::setStreams(const StreamSpecs & inputs) {
  m_inCollection = inputs;
  return StatusCode::SUCCESS;
}
//====================================================


