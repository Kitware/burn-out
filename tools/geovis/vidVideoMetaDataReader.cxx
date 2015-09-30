/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vidVideoMetaDataReader.h"
#include "vtkObjectFactory.h"
#include <assert.h>
#include "vidStep.h"

vtkStandardNewMacro(vidVideoMetaDataReader);
vtkCxxRevisionMacro(vidVideoMetaDataReader, "$Revision$");

// ----------------------------------------------------------------------------
vidVideoMetaDataReader::vidVideoMetaDataReader()
{
  this->FileName=new vtkstd::string;
  this->MatrixFileName=new vtkstd::string;
  
  this->Steps=new vtkstd::vector<vtkSmartPointer<vidStep> >;
  
  this->LastFrame=0;
}

// ----------------------------------------------------------------------------
vidVideoMetaDataReader::~vidVideoMetaDataReader()
{
  delete this->FileName;
  delete this->MatrixFileName;
  delete this->Steps;
}

// ----------------------------------------------------------------------------
void vidVideoMetaDataReader::SetFileName(const vtkstd::string *fileName)
{
  assert("pre: fileName_exists" && fileName!=0);
  if(this->FileName!=fileName)
    {
    (*this->FileName)=(*fileName);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
const vtkstd::string *vidVideoMetaDataReader::GetFileName() const
{
  return this->FileName;
}

// ----------------------------------------------------------------------------
int vidVideoMetaDataReader::GetFrame() const
{
  return this->Frame;
}

// ----------------------------------------------------------------------------
void vidVideoMetaDataReader::SetFrame(int frame)
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
void vidVideoMetaDataReader::ReadFrame()
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
bool vidVideoMetaDataReader::LastFrameValid() const
{
  return this->LastFrame!=0; 
}

// ----------------------------------------------------------------------------
// Description:
// \pre valid: LastFrameValid()
vidStep *vidVideoMetaDataReader::GetLastFrame() const
{
  assert("pre: valid" && this->LastFrameValid());
  return this->LastFrame;
}

// ----------------------------------------------------------------------------
bool vidVideoMetaDataReader::ReadMetaDataFile()
{
  cout << "vidVideoMetaDataReader::ReadMetaDataFile()" << endl;
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

// ----------------------------------------------------------------------------
void vidVideoMetaDataReader::PrintSelf(ostream& os, vtkIndent indent)
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
 
  os << indent << "Frame: " << this->Frame << endl;
}
