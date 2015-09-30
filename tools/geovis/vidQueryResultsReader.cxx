/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidQueryResultsReader.h"
#include "vtkObjectFactory.h"
#include <assert.h>
//#include "vidStep.h"
#include "vidTrack.h"
#include "vidBBox.h"
#include <vtksys/ios/sstream>
#include "vidTrackMetaData.h"

vtkStandardNewMacro(vidQueryResultsReader);
vtkCxxRevisionMacro(vidQueryResultsReader, "$Revision$");

// ----------------------------------------------------------------------------
vidQueryResultsReader::vidQueryResultsReader()
{
  this->FileName=new vtkstd::string;
//  this->MatrixFileName=new vtkstd::string;
  
  this->Tracks=new vtkstd::vector<vtkSmartPointer<vidTrack> >;
  
//  this->LastFrame=0;
}

// ----------------------------------------------------------------------------
vidQueryResultsReader::~vidQueryResultsReader()
{
  delete this->FileName;
//  delete this->MatrixFileName;
//  delete this->Steps;
  delete this->Tracks;
}

// ----------------------------------------------------------------------------
void vidQueryResultsReader::SetFileName(const vtkstd::string *fileName)
{
  assert("pre: fileName_exists" && fileName!=0);
  if(this->FileName!=fileName)
    {
    (*this->FileName)=(*fileName);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
const vtkstd::string *vidQueryResultsReader::GetFileName() const
{
  return this->FileName;
}

// ----------------------------------------------------------------------------
vtkstd::vector<vtkSmartPointer<vidTrack > > *
vidQueryResultsReader::GetTracks() const
{
  return this->Tracks;
}

#if 0
// ----------------------------------------------------------------------------
int vidQueryResultsReader::GetFrame() const
{
  return this->Frame;
}

// ----------------------------------------------------------------------------
void vidQueryResultsReader::SetFrame(int frame)
{
  if(this->Frame!=frame)
    {
    this->Frame=frame;
//    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Read the current frame.
// Change the value of LastFrameValid()
void vidQueryResultsReader::ReadFrame()
{
  this->LastFrame=0;
  
  this->ReadMetaDataFile();
  
  size_t i=0;
  bool found=false;
  while(!found && i<this->Steps->size())
    {
    found=(*this->Steps)[i]->GetFrameInShot()==this->Frame;
    ++i;
    }
  if(!found)
    {
    vtkErrorMacro(<< "frame" << this->Frame << " does not exist.");
    }
  else
    {
    --i;
    this->LastFrame=(*this->Steps)[i];
    }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if the last read was successful.
bool vidQueryResultsReader::LastFrameValid() const
{
  return this->LastFrame!=0; 
}

// ----------------------------------------------------------------------------
// Description:
// \pre valid: LastFrameValid()
vidStep *vidQueryResultsReader::GetLastFrame() const
{
  assert("pre: valid" && this->LastFrameValid());
  return this->LastFrame;
}

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ReadMetaDataFile()
{
  cout << "vidQueryResultsReader::ReadMetaDataFile()" << endl;
  if(this->MTime<=this->MetaFileReadTime)
    {
    return true;
    }
  
  bool result=true;
  
  this->Steps->clear();
  
  if(this->FileName->empty())
    {
    vtkErrorMacro(<<"metadata file name required.");
    result=false;
    }
  
  // comment line, 186 characters.
  const char header[]="#MAAS-metadata-v2  FrameInShot FrameInSource TimeStamp MetadataValid SensorLat SensorLon SensorAlt FrameULLat FrameULLon FrameURLat FrameURLon FrameLRLat FrameLRLon FrameLLLat FrameLLLon";
  
  std::ifstream f;
  if(result)
    {
    f.open(this->FileName->c_str());
    }
  if(result && f.good())
    {
    // check first line is the comment line: 186 characters.
    // register \r=x0d=CR and \n=0xa=LF as whitespaces
    // Windows: \r\n
    // Unix: \n
    // old Mac: \r
    std::string firstLine;
    getline(f,firstLine);
    
    // because of the newline headache, compare only the first 186 characters.
    if(firstLine.compare(0,186,header,0,186)!=0)
      {
      f.close();
      result=false;
      vtkErrorMacro(<<"wrong header, not a metadata file.");
      }
    }
  size_t currentLine=1;
  while(result && f.good())
    {
    int frameInShot;
    int frameInSource;
    // 64-bit on 64-bit arch. We 64-bit on 32-bit arch too...
    double timeStamp;
    int metaDataValid;
    double sensorLat;
    double sensorLon;
    double sensorAlt;
    double frameULLat;
    double frameULLon;
    double frameURLat;
    double frameURLon;
    double frameLRLat;
    double frameLRLon;
    double frameLLLat;
    double frameLLLon;
    
    f >> frameInShot >> frameInSource >> timeStamp >> metaDataValid
      >> sensorLat >> sensorLon >> sensorAlt >> frameULLat >> frameULLon
      >> frameURLat >> frameURLon >> frameLRLat >> frameLRLon >> frameLLLat
      >> frameLLLon;
    
    if(f.good())
      {
      vidStep *s=vidStep::New();
      s->SetFrameInShot(frameInShot);
      s->SetFrameInSource(frameInSource);
      s->SetTimeStamp(timeStamp);
      s->SetValid(metaDataValid==1);
      s->SetSensorLatitude(sensorLat);
      s->SetSensorLongitude(sensorLon);
      s->SetSensorAltitude(sensorAlt);
      s->SetFrameUpperLeftLatitude(frameULLat);
      s->SetFrameUpperLeftLongitude(frameULLon);
      s->SetFrameUpperRightLatitude(frameURLat);
      s->SetFrameUpperRightLongitude(frameURLon);
      s->SetFrameLowerLeftLatitude(frameLLLat);
      s->SetFrameLowerLeftLongitude(frameLLLon);
      s->SetFrameLowerRightLatitude(frameLRLat);
      s->SetFrameLowerRightLongitude(frameLRLon);
      this->Steps->push_back(s);
      s->Delete();
      ++currentLine;
      }
    
    std::cout << "frameInShot=" << frameInShot << " frameInSource="
              << frameInSource << " timeStamp=" << timeStamp
              << " metaDataValid=" << metaDataValid << " sensorLat="
              << sensorLat << " sensorLon=" << sensorLon << " sensorAlt="
              << sensorAlt << " frameULLat=" << frameULLat << " frameULLon="
              << frameULLon << " frameURLat=" << frameURLat << " frameURLon="
              << frameURLon << " frameLRLat=" << frameLRLat << " frameLRLon="
              << frameLRLon << " frameLLLat="  << frameLLLat
              << " frameLLLon=" <<  frameLLLon << std::endl;
    }
  if(result)
    {
    if(f.bad())
      {
      vtkErrorMacro(<<"error reading line" << currentLine);
      result=false;
      }
    f.close();
    }
  
  this->MetaFileReadTime.Modified();
  return result;
}
#endif

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ReadXMLFile()
{
  bool result;
  
  if(this->ReadTime>=this->GetMTime())
    {
    result=true;
    }
  else
    {
    this->Tracks->clear();
    if(this->FileName->empty())
      {
      vtkErrorMacro(<<"xml file name required.");
      result=false;
      }
    else
      {
      xmlDoc *doc=xmlReadFile(this->FileName->c_str(),0,0);
      if(doc==0)
        {
        vtkErrorMacro(<<"Cannot parse file" << this->FileName);
        result=false;
        }
      else
        {
        xmlNode *rootElement=xmlDocGetRootElement(doc);
        
        if(rootElement==0)
          {
          vtkErrorMacro(<<"No root element" << this->FileName);
          result=false;
          }
        else
          {
          result=this->ParseRoot(rootElement);
          }
        xmlFreeDoc(doc);
        }
      xmlCleanupParser();
      }
    this->ReadTime.Modified();
    }
  
  return result;
}

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ParseRoot(xmlNode *root)
{
  assert("root_exists" && root!=0);
  
  bool result=true;
  xmlNode *currentNode=root;
  
  while(result && currentNode!=0)
    {
    if(currentNode->type==XML_ELEMENT_NODE)
      {
      std::string name=reinterpret_cast<const char *>(currentNode->name);
      if(name.compare("vibrantDescriptors")==0)
        {
        result=this->ParseTracks(currentNode->children);
        }
      else
        {
        vtkErrorMacro("no vibrantDescriptors element.");
        }
      }
    currentNode=currentNode->next;
    }
  return result;
}

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ParseTracks(xmlNode *firstChild)
{
  bool result=true;
  // the list can be empty.
  xmlNode *currentChild=firstChild;
  while(result && currentChild!=0)
    {
    if(currentChild->type==XML_ELEMENT_NODE)
      {
      std::string name=reinterpret_cast<const char *>(currentChild->name);
      if(name.compare("track")==0)
        {
        vidTrack *t=this->ParseTrack(currentChild);
        result=t!=0;
        if(result)
          {
          this->Tracks->push_back(t);
          t->Delete();
          }
        }
      else
        {
        vtkErrorMacro("wrong child: this should be a track element.");
        }
      }
    currentChild=currentChild->next;
    }
  return result;
}

// ----------------------------------------------------------------------------
vidTrack *vidQueryResultsReader::ParseTrack(xmlNode *track)
{
  assert("pre track_exists" && track!=0);
  
  xmlChar *value=0;
  char *valueC=0;
  
  vidTrack *result=vidTrack::New();
  
  value=xmlGetProp(track,reinterpret_cast<const xmlChar *>("startFrame"));
  if(value==0)
    {
    vtkErrorMacro("missing startFrame attribute");
    result->Delete();
    result=0;
    }
  else
    {
    valueC=reinterpret_cast<char *>(value);
    vtkDebugMacro("startFrame=" << valueC);
    
    std::istringstream is(valueC);
    int startFrame;
    is >> startFrame;
    result->SetStartFrame(startFrame);
    xmlFree(value);
    }
  
  if(result!=0)
    {
    value=xmlGetProp(track,reinterpret_cast<const xmlChar *>("lastFrame"));
    if(value==0)
      {
      vtkErrorMacro("missing lastFrame attribute");
      result->Delete();
      result=0;
      }
    else
      {
      valueC=reinterpret_cast<char *>(value);
      vtkDebugMacro("lastFrame=" << valueC);
      
      std::istringstream is(valueC);
      int lastFrame;
      is >> lastFrame;
      result->SetLastFrame(lastFrame);
      xmlFree(value);
      }
    }
  
  if(result!=0)
    {
    value=xmlGetProp(track,reinterpret_cast<const xmlChar *>("frameNumberOrigin"));
    if(value==0)
      {
      vtkWarningMacro("missing frameNumberOrigin attribute");
      }
    else
      {
      // we ignore this attribute anyway.
      }
    }
  
  if(result!=0)
    {
    this->ParseTrackChildren(track->children,result);
    }
  
  return result;
}

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ParseTrackChildren(xmlNode *firstChild,
                                               vidTrack *track)
{
  assert("pre: track_exists" && track!=0);
  
  xmlNode *currentChild=firstChild;
  bool success=true;
  
  while(currentChild!=0)
    {
    if(currentChild->type==XML_ELEMENT_NODE)
      {
      std::string name=reinterpret_cast<const char *>(currentChild->name);
      if(name.compare("videoID")==0)
        {
        // ignored.
        }
      else
        {
        if(name.compare("sourceTrackIDs")==0)
          {
          // ignored.
//          this->ParseSourceTrackIDs(currentChild);
          }
        else
          {
          if(name.compare("trackStyle")==0)
            {
            // ignored.
//            this->ParseTrackStyle(currentChild);
            }
          else
            {
            if(name.compare("bbox")==0)
              {
              vidBBox *b=this->ParseBBox(currentChild);
              if(b!=0)
                {
                track->GetBoxes()->push_back(b);
                b->Delete();
                }
              }
            else
              {
              if(name.compare("descriptor")==0)
                {
                this->ParseDescriptor(track,currentChild);
                }
              else
                {
                vtkErrorMacro("Unknown track child --" << name.c_str() << "--");
                }
              }
            }
          }
        }
      }
    currentChild=currentChild->next;
    }
  return success;
}

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ParseSourceTrackIDs(xmlNode *node)
{
  assert("pre: node_exists" && node!=0);
  std::string name=reinterpret_cast<const char *>(node->name);
  assert("pre: sourceTrackIDs_node" && name.compare("sourceTrackIDs")==0);
  xmlChar *content=0;
  content=xmlNodeGetContent(node);
  vtkDebugMacro("sourceTrackIDs-content=" << content);
  xmlFree(content);
  return true;
}

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ParseTrackStyle(xmlNode *node)
{
  assert("pre: node_exists" && node!=0);
  std::string name=reinterpret_cast<const char *>(node->name);
  assert("pre: trackStyle_node" && name.compare("trackStyle")==0);

  
  bool result=true;
  xmlChar *content=0;
  content=xmlNodeGetContent(node);
  vtkDebugMacro("trackStyle-content=" << content);
  std::string style=reinterpret_cast<const char *>(content);
  
  if(style.compare("trackDescriptorKitware")==0)
    {
    vtkDebugMacro("kitware style");
    }
  else
    {
    if(style.compare("trackDescriptorICSI")==0)
      {
      vtkDebugMacro("ICSI style");
      }
    else
      {
      vtkErrorMacro("unknown style");
      result=false;
      }
    }
  
  xmlFree(content);
  return result;
}

// ----------------------------------------------------------------------------
vidBBox *vidQueryResultsReader::ParseBBox(xmlNode *node)
{
  assert("pre: node_exists" && node!=0);
  std::string name=reinterpret_cast<const char *>(node->name);
  assert("pre: bbox_node" && name.compare("bbox")==0);
  
  vidBBox *result=vidBBox::New();
  
  xmlChar *value=0;
  char *valueC=0;
  
  value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("type"));
  if(value==0)
    {
    vtkErrorMacro("missing type attribute");
    result->Delete();
    result=0;
    }
  else
    {
    valueC=reinterpret_cast<char *>(value);
    vtkDebugMacro("type=" << valueC);
    
    std::string typeValue=valueC;
    if(typeValue.compare("raw")!=0)
      {
      vtkErrorMacro(<<"type should be raw");
      result->Delete();
      result=0;
      }
    xmlFree(value);
    }
  
  if(result!=0)
    {
     value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("frame"));
     if(value==0)
       {
       vtkErrorMacro("missing frame attribute");
       result->Delete();
       result=0;
       }
     else
       {
       valueC=reinterpret_cast<char *>(value);
       vtkDebugMacro("frame=" << valueC);
       
       std::istringstream is(valueC);
       int frame;
       is >> frame;
       result->SetFrame(frame);
       xmlFree(value);
       }
    }
  
  if(result!=0)
    {
     value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("ulx"));
     if(value==0)
       {
       vtkErrorMacro("missing ulx attribute");
       result->Delete();
       result=0;
       }
     else
       {
       valueC=reinterpret_cast<char *>(value);
       vtkDebugMacro("ulx=" << valueC);
       
       std::istringstream is(valueC);
       int ulx;
       is >> ulx;
       result->SetLeft(ulx);
       xmlFree(value);
       }
    }
  
   if(result!=0)
    {
     value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("uly"));
     if(value==0)
       {
       vtkErrorMacro("missing uly attribute");
       result->Delete();
       result=0;
       }
     else
       {
       valueC=reinterpret_cast<char *>(value);
       vtkDebugMacro("uly=" << valueC);
       
       std::istringstream is(valueC);
       int uly;
       is >> uly;
       result->SetUpper(uly);
       xmlFree(value);
       }
    }
  
    if(result!=0)
    {
     value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("lrx"));
     if(value==0)
       {
       vtkErrorMacro("missing lrx attribute");
       result->Delete();
       result=0;
       }
     else
       {
       valueC=reinterpret_cast<char *>(value);
       vtkDebugMacro("lrx=" << valueC);
       
       std::istringstream is(valueC);
       int lrx;
       is >> lrx;
       result->SetRight(lrx);
       xmlFree(value);
       }
    }
  
   if(result!=0)
    {
     value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("lry"));
     if(value==0)
       {
       vtkErrorMacro("missing lry attribute");
       result->Delete();
       result=0;
       }
     else
       {
       valueC=reinterpret_cast<char *>(value);
       vtkDebugMacro("lry=" << valueC);
       
       std::istringstream is(valueC);
       int lry;
       is >> lry;
       result->SetLower(lry);
       xmlFree(value);
       }
    }
  
   if(result!=0)
    {
     value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("timestamp"));
     if(value==0)
       {
       vtkErrorMacro("missing timestamp attribute");
       result->Delete();
       result=0;
       }
     else
       {
       valueC=reinterpret_cast<char *>(value);
       vtkDebugMacro("timestamp=" << valueC);
       
       std::istringstream is(valueC);
       double timestamp;
       is >> timestamp;
       result->SetTimeStamp(timestamp);
       xmlFree(value);
       }
    }
  
  return result;
}

// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ParseDescriptor(vidTrack *track,
                                            xmlNode *node)
{
  assert("pre node_exists" && node!=0);
  std::string name=reinterpret_cast<const char *>(node->name);
  assert("pre: descriptor_node" && name.compare("descriptor")==0);
  
  bool result=true;
  xmlChar *value=0;
  char *valueC=0;
  
  value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("type"));
  if(value==0)
    {
    vtkErrorMacro("missing type attribute");
    result=false;
    }
  else
    {
    valueC=reinterpret_cast<char *>(value);
    vtkDebugMacro("type=" << valueC);
    
    std::string typeValue=valueC;
    if(typeValue.compare("classifier")==0)
      {
      // ignored.
      }
    else
      {
      if(typeValue.compare("metadataDescriptor")==0)
        {
        this->ParseMetadataDescriptor(track,node->children);
        }
      else
        {
        vtkErrorMacro(<<"type should be classifier or metadataDescriptor");
        result=false;
        }
      }
    xmlFree(value);
    }
  return result;
}
  
// ----------------------------------------------------------------------------
bool vidQueryResultsReader::ParseMetadataDescriptor(vidTrack *track,
                                                    xmlNode *firstChild)
{
  xmlNode *node=firstChild;
  double dValue;
  xmlChar *value=0;
  char *valueC=0;
  
  bool status=true;
  while(status && node!=0)
    {
    std::string name=reinterpret_cast<const char *>(node->name);
    
    if(name.compare("SensorLatitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetSensorLatitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node SensorLatitude");
        }
      }
    else if(name.compare("SensorLongitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetSensorLongitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node SensorLongitude");
        }
      }
    else if(name.compare("UpperLeftCornerLatitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetUpperLeftLatitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node UpperLeftCornerLatitude");
        }
      }
    else if(name.compare("UpperLeftCornerLongitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetUpperLeftLongitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node UpperLeftCornerLongitude");
        }
      }
    else if(name.compare("UpperRightCornerLatitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetUpperRightLatitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node UpperRightCornerLatitude");
        }
      }
    else if(name.compare("UpperRightCornerLongitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetUpperRightLongitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node UpperRightCornerLongitude");
        }
      }
    else if(name.compare("LowerLeftCornerLatitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetLowerLeftLatitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node LowerLeftCornerLatitude");
        }
      }
    else if(name.compare("LowerLeftCornerLongitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetLowerLeftLongitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node LowerLeftCornerLongitude");
        }
      }
    else if(name.compare("LowerRightCornerLatitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetLowerRightLatitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node LowerRightCornerLatitude");
        }
      }
    else if(name.compare("LowerRightCornerLongitude")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetLowerRightLongitude(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node LowerRightCornerLongitude");
        }
      }
    else if(name.compare("HorizontalFieldOfView")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetHorizontalFieldOfView(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node HorizontalFieldOfView");
        }
      }
    else if(name.compare("VerticalFieldOfView")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetVerticalFieldOfView(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node VerticalFieldOfView");
        }
      }
    else if(name.compare("TimeStampMicrosecondsSince1970")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetTimeStamp(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node TimeStampMicrosecondsSince1970");
        }
      }
     else if(name.compare("SlantRange")==0)
      {
      value=xmlGetProp(node,reinterpret_cast<const xmlChar *>("value"));
      status=value!=0;
      if(status)
        {
        valueC=reinterpret_cast<char *>(value);
        vtkDebugMacro("value=" << valueC);
        std::istringstream is(valueC);
        is >> dValue;
        track->GetMetaData()->SetSlantRange(dValue);
        xmlFree(value);
        }
      else
        {
        vtkErrorMacro(<<"missing value attribute on node SlantRange");
        }
      }
    node=node->next;
    }
  
  return status;
}

// ----------------------------------------------------------------------------
void vidQueryResultsReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "FileName: ";
  if(this->FileName==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << this->FileName << endl;
    }
  
  size_t c=this->Tracks->size();
  os << indent << "Tracks:"<< c << endl;
  size_t i=0;
  while(i<c)
    {
    os << indent << "Track " << i <<":" <<endl;
    (*this->Tracks)[i]->PrintSelf(os,indent.GetNextIndent());
    ++i;
    }
}
