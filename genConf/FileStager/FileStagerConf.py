#Wed Jul  7 13:56:48 2010
"""Automatically generated. DO NOT EDIT please"""
from GaudiKernel.Proxy.Configurable import *

class StagerChronoSvc( ConfigurableService ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditServices' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditStart' : False, # bool
    'AuditStop' : False, # bool
    'AuditFinalize' : False, # bool
    'AuditReInitialize' : False, # bool
    'AuditReStart' : False, # bool
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(StagerChronoSvc, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'FileStager'
  def getType( self ):
      return 'StagerChronoSvc'
  pass # class StagerChronoSvc

class Gaudi__StagedIODataManager( ConfigurableService ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditServices' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditStart' : False, # bool
    'AuditStop' : False, # bool
    'AuditFinalize' : False, # bool
    'AuditReInitialize' : False, # bool
    'AuditReStart' : False, # bool
    'CatalogType' : 'Gaudi::MultiFileCatalog/FileCatalog', # str
    'UseGFAL' : True, # bool
    'QuarantineFiles' : True, # bool
    'AgeLimit' : 2, # int
    'StagerSvc' : 'FileStagerSvc', # str
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(Gaudi__StagedIODataManager, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'FileStager'
  def getType( self ):
      return 'Gaudi::StagedIODataManager'
  pass # class Gaudi__StagedIODataManager

class StagedDataStreamTool( ConfigurableAlgTool ) :
  __slots__ = { 
    'MonitorService' : 'MonitorSvc', # str
    'OutputLevel' : 7, # int
    'AuditTools' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditStart' : False, # bool
    'AuditStop' : False, # bool
    'AuditFinalize' : False, # bool
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(StagedDataStreamTool, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'FileStager'
  def getType( self ):
      return 'StagedDataStreamTool'
  pass # class StagedDataStreamTool

class InputStreamParser( ConfigurableAlgTool ) :
  __slots__ = { 
    'MonitorService' : 'MonitorSvc', # str
    'OutputLevel' : 7, # int
    'AuditTools' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditStart' : False, # bool
    'AuditStop' : False, # bool
    'AuditFinalize' : False, # bool
  }
  _propertyDocDct = { 
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(InputStreamParser, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'FileStager'
  def getType( self ):
      return 'InputStreamParser'
  pass # class InputStreamParser

class FileStagerSvc( ConfigurableService ) :
  __slots__ = { 
    'OutputLevel' : 7, # int
    'AuditServices' : False, # bool
    'AuditInitialize' : False, # bool
    'AuditStart' : False, # bool
    'AuditStop' : False, # bool
    'AuditFinalize' : False, # bool
    'AuditReInitialize' : False, # bool
    'AuditReStart' : False, # bool
    'PipeLength' : 1, # int
    'InfilePrefix' : 'gfal:', # str
    'OutfilePrefix' : 'file:', # str
    'BaseTmpdir' : '', # str
    'InputCollections' : [  ], # list
    'OutputCollections' : [  ], # list
    'LogfileDir' : '', # str
    'KeepLogfiles' : False, # bool
    'ReleaseFiles' : True, # bool
    'StreamManager' : 'StagedDataStreamTool', # str
    'FallbackDir' : '', # str
    'ParallelStreams' : 1, # int
  }
  _propertyDocDct = { 
    'InputCollections' : """ vector of input files """,
    'OutputCollections' : """ vector of output files """,
  }
  def __init__(self, name = Configurable.DefaultName, **kwargs):
      super(FileStagerSvc, self).__init__(name)
      for n,v in kwargs.items():
         setattr(self, n, v)
  def getDlls( self ):
      return 'FileStager'
  def getType( self ):
      return 'FileStagerSvc'
  pass # class FileStagerSvc
