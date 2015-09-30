# -*- coding: utf-8 -*-
# This ConfigBlock module reads vidtk-style configuration file
#
# It should be used in this manner:
#
# If your config file, myconfig.conf, looked like:
#   param1 = value1
#   param2 = value2
#   subBlock1:param3 = value3
#   subBlock1:param4 = value4
#   subBlock1:SubBlock2:param5 = value5
#
# then you would read the parameters one of 2 ways:
#   config = ConfigBlock("global")
#   config.Parse("myconfigfile.conf")
#   param1 = config.GetParameter("param1")
#   param2 = config.GetParameter("param2")
#   param3 = config.GetParameter("subBlock1:param3")
#   param4 = config.GetParameter("subBlock1:param4")
#   param5 = config.GetParameter("subBlock1:SubBlock2:param5")
#
# or
#   config = ConfigBlock("global")
#   config.Parse("myconfigfile.conf")
#   param1 = config.GetParameter("param1")
#   param2 = config.GetParameter("param2")
#   configSub1 = config.GetSubBlock("subBlock1")
#   param3 = configSub1.GetParameter("param3")
#   param4 = configSub1.GetParameter("param4")
#   configSub2 = configSub1.GetSubBlock("SubBlock2")
#   param5 = configSub2.GetParameter("param5")

import re

class ConfigBlock :

  def __init__(self, blockName) :
    self.Name = blockName
    self.Values = {}
    self.SubBlocks = {}
    self.Blocks = {}

  ##################################################
  # Retrieve an entire sib configuration block
  ##################################################
  def GetSubBlock(self, blockName) :
    subBlockMatch = re.match("([^:]+):(.+)", blockName)
    if subBlockMatch != None :
      subBlockName = subBlockMatch.group(1)
      subBlockSubName = subBlockMatch.group(2)

      if subBlockName not in self.SubBlocks : return None
      return self.SubBlocks[subBlockName].GetSubBlock(subBlockSubName)

    if blockName not in self.SubBlocks : return None
    return self.SubBlocks[blockName]


  ##################################################
  # Retrieve a parameter from the config block
  ##################################################
  def GetParameter(self, paramName) :
    subBlockMatch = re.match("([^:]+):(.+)", paramName)
    if subBlockMatch != None :
      subBlockName = subBlockMatch.group(1)
      subBlockParamName = subBlockMatch.group(2)

      if subBlockName not in self.SubBlocks : return None
      return self.SubBlocks[subBlockName].GetParameter(subBlockParamName)

    if paramName not in self.Values : return None
    return self.Values[paramName]


  ##################################################
  # Add a parameter to the config block
  ##################################################
  def AddParameter(self, paramName, paramValue) :
    subBlockMatch = re.match("([^:]+):(.+)", paramName)
    if subBlockMatch == None :
      self.Values[paramName] = paramValue
    else :
      subBlockName = subBlockMatch.group(1)
      subBlockParamName = subBlockMatch.group(2)

      if subBlockName not in self.SubBlocks :
        self.SubBlocks[subBlockName] = ConfigBlock(subBlockName)

      subBlock = self.SubBlocks[subBlockName]
      subBlock.AddParameter(subBlockParamName, paramValue)


  ##################################################
  # Load the config block from a config file
  ##################################################
  def Parse(self, filename) :
    f = open(filename, "r")
    return self.ParseStream(f)


  ##################################################
  # Load the config block from an existing stream
  ##################################################
  def ParseStream(self, s) :
    while 1 :
      line = s.readline()
      if not line :
        break
      line = line.strip()

      # Skip empty lines and comments
      if len(line) == 0 or re.match("^#", line) != None :
        continue

      # Process top level starting block
      blockMatch = re.match("^block (.+)", line)
      if blockMatch != None :
        blockName = blockMatch.group(1)
        if blockName in self.Blocks :
          print "Warning: ignoring block definition for '" + blockName + "'"
          continue
        block = ConfigBlock(blockName)
        if not block.ParseStream(s) :
          print "Error: failure parsing block", blockName
          return False
        self.Blocks[blockName] = block
        continue

      # Process top level ending block
      if line == "endblock" :
        return True

      # Process parameter entries
      matchParam = re.match("([^=]+)=(.+)", line)
      if matchParam != None :
        paramName = matchParam.group(1).strip()
        paramValue = matchParam.group(2).strip()
        self.AddParameter(paramName, paramValue)
        continue

      print "Warning: unable to parse '" + line + "'"
    return True


