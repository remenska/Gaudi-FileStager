#ifndef STAGERCHRONOSVC_H
#define STAGERCHRONOSVC_H 1

#include "GaudiKernel/Service.h"
#include "GaudiKernel/IIncidentListener.h"

#include "GaudiKernel/MsgStream.h"
#include "FileStager/IFileStagerSvc.h"
#include "StageManager.h"
#include "GaudiKernel/IDataProviderSvc.h"
// #include "GaudiKernel/DataStoreItem.h"
#include "GaudiKernel/IChronoStatSvc.h"
#include <string>
#include <vector>


class IIncidentSvc;

// class IDataManagerSvc;
namespace Gaudi
{
  class IIncidentSvc;
 
}


class StagerChronoSvc : public extends1 <Service,IIncidentListener>
{
public:
 
  StagerChronoSvc(const std::string& nam, ISvcLocator* svcLoc);
  virtual StatusCode initialize();
  void handle(const Incident& inc);

  StatusCode finalize();
  //   StageManager& getStageManagerInstance();
  virtual ~StagerChronoSvc();
protected:
  
private:
  
 IIncidentSvc* m_incidentSvc;
  struct timeval tp;
  double sec, usec;
  double m_beginInputFile, m_endInputFile;
  double m_totalWaitTime, m_totalProcessingTime;
};

#endif
