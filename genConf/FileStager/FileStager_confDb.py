##  -*- python -*-  
# db file automatically generated by genconf on: Wed Jul  7 13:56:48 2010
## insulates outside world against anything bad that could happen
## also prevents global scope pollution
def _fillCfgDb():
    from GaudiKernel.Proxy.ConfigurableDb import CfgDb

    # get a handle on the repository of Configurables
    cfgDb = CfgDb()

    # populate the repository with informations on Configurables 

    cfgDb.add( configurable = 'StagerChronoSvc',
               package = 'FileStager',
               module  = 'FileStager.FileStagerConf',
               lib     = 'FileStager' )
    cfgDb.add( configurable = 'Gaudi__StagedIODataManager',
               package = 'FileStager',
               module  = 'FileStager.FileStagerConf',
               lib     = 'FileStager' )
    cfgDb.add( configurable = 'StagedDataStreamTool',
               package = 'FileStager',
               module  = 'FileStager.FileStagerConf',
               lib     = 'FileStager' )
    cfgDb.add( configurable = 'InputStreamParser',
               package = 'FileStager',
               module  = 'FileStager.FileStagerConf',
               lib     = 'FileStager' )
    cfgDb.add( configurable = 'FileStagerSvc',
               package = 'FileStager',
               module  = 'FileStager.FileStagerConf',
               lib     = 'FileStager' )
    return #_fillCfgDb
# fill cfgDb at module import...
try:
    _fillCfgDb()
    #house cleaning...
    del _fillCfgDb
except Exception,err:
    print "Py:ConfigurableDb   ERROR Problem with [%s] content!" % __name__
    print "Py:ConfigurableDb   ERROR",err
    print "Py:ConfigurableDb   ERROR   ==> culprit is package [FileStager] !"
