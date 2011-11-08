
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/Tokenizer.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/ISvcManager.h"
#include "GaudiKernel/PropertyMgr.h"
#include "InputStreamParser.h"
#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/SvcFactory.h"
#include <cstdio>
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

//-----------------------------------------------------------------------------
// Implementation file for class : InputStreamParser
//-----------------------------------------------------------------------------
DECLARE_TOOL_FACTORY( InputStreamParser )

const std::string c_SRECOLLECTION = "([=]*)COLLECTION=\'([^\']*)\'";
const std::string c_SREDATA = "([=]*)(DATA|FILE|DATAFILE)=\'([^\']*)\'";

//=============================================================================
// Standard constructor, initializes variables
//=============================================================================
InputStreamParser::InputStreamParser( const std::string& type,
                                      const std::string& name,
                                      const IInterface* parent )
    : base_class ( type, name , parent )
    , m_inCollection(0)
    //     , m_isCollection(false)
{}
//=============================================================================
// Destructor
//=============================================================================
InputStreamParser::~InputStreamParser() {}

//=============================================================================
StatusCode InputStreamParser::initialize() {

  MsgStream logger(msgSvc(), name());

  StatusCode status = AlgTool::initialize();
  if( !status.isSuccess() ) {
    logger << MSG::FATAL << "Error. Cannot initialize base class." << endmsg;
    return status;
  }

  return StatusCode::SUCCESS;

}

//=============================================================================
StatusCode InputStreamParser::extractFileReferences(std::string input) {
  MsgStream log(msgSvc(), name());
  TFile *f(0);
  f = TFile::Open(input.c_str(),"READ");
  if (f ==0) {
    log << MSG::ERROR << "Couldn't access the file: "<< input <<endmsg;
    return StatusCode::FAILURE;
  }
  if (!f->IsOpen()) {
    log << MSG::ERROR << "File "<<input <<" cannot be "
    << "opened for reading."<< endmsg;
    return StatusCode::FAILURE;
  }

  std::string sre = "([=]*)DB=([^\\]\\[]*)";
  boost::smatch matches;
  boost::match_flag_type flags = boost::match_default;
  char text[16000];
  TBranch* b=0;
  int i;
  b = ((TTree*)f->Get("##Links"))->GetBranch("db_string");
  Tokenizer tok(true);
  for(i=0,b->SetAddress(text); i < b->GetEntries(); ++i) {
    b->GetEvent(i);
    const std::string text1(text);
    //         log << MSG::INFO <<"The text: "<< text <<endmsg;
    if(!boost::find_first(text, "CNT=/Event"))
      continue;
    else {
      m_regex.assign (sre, boost::regex_constants::icase);
      if(boost::regex_search(text1.begin(),text1.end(), matches, m_regex, flags)) {
        std::string match_db(matches[2].first, matches[2].second);
        m_inCollection->push_back("gfal:guid:"+match_db);
        log << MSG::DEBUG << "Match db: " << match_db << endmsg ;
      }
    }
  }
  f->Close("R");
  return StatusCode::SUCCESS;
}

//====================================================
StatusCode InputStreamParser::extractStreams(const StreamSpecs & inputs, StreamSpecs & parsedInputs) {
  m_inCollection = & parsedInputs;
  StatusCode sc = StatusCode::FAILURE;
  MsgStream log(msgSvc(), name());
  for ( std::vector<std::string>::const_iterator itr = inputs.begin(); itr != inputs.end(); ++itr ) {
    sc = extractStream(*itr);
    if (!sc.isSuccess()) {
      log << MSG::ERROR << " Could not access the file(s). " << endmsg;
      ::abort();
    }
  }

  return sc;
}

//====================================================
StatusCode InputStreamParser::extractStream(const std::string & input) {
  // parses a string containing multiple tag='value',
  // such as: DATAFILE='LFN:/grid/lhcb/data/file.dst' TYP='POOL_ROOTTREE' OPT='READ'

  StatusCode sc = StatusCode::SUCCESS;
  MsgStream log(msgSvc(), name());
  boost::smatch matches;
  boost::match_flag_type flags = boost::match_default;

  // find the file descriptor
  m_regex.assign (c_SREDATA, boost::regex_constants::icase);
  if(boost::regex_search(input.begin(),input.end(), matches, m_regex, flags)) {
    std::string match_descriptor(matches[3].first, matches[3].second);
    m_inCollection->push_back(match_descriptor);
    log << MSG::DEBUG <<  match_descriptor << endmsg ;

  } else {
    log <<MSG::ERROR << "File descriptor not present in: "<< input <<endmsg;
    return StatusCode::FAILURE;
  }

  // check if it is a collection:
  m_regex.assign(c_SRECOLLECTION, boost::regex_constants::icase);
  if(boost::regex_search(input.begin(),input.end(), matches, m_regex, flags)) {
    //     m_isCollection = true;
    std::string ETC(m_inCollection->back());
    m_inCollection->pop_back();
    sc = extractFileReferences(ETC);
  }
  return sc;
}

//====================================================
StatusCode InputStreamParser::finalize() {
  StatusCode sc =  AlgTool::finalize();
  return sc;
}
//====================================================


