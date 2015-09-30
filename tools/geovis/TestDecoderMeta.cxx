/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <fstream>
#include <iostream>

#include "vidVideoMetaDataReader.h"
#include "vtkGeoSphereTransform.h"

int main(int argc,
         char *argv[])
{
  int result=0;
  
  if(argc!=2)
    {
    std::cerr << "meta video filename missing in argument" << std::endl;
    return 1;
    }
  
  std::string fileName=argv[1];
  
  // comment line, 186 characters.
  const char header[]="#MAAS-metadata-v2  FrameInShot FrameInSource TimeStamp MetadataValid SensorLat SensorLon SensorAlt FrameULLat FrameULLon FrameURLat FrameURLon FrameLRLat FrameLRLon FrameLLLat FrameLLLon";
  
  std::string h=header; 
  
  std::ifstream f;
  f.open(fileName.c_str());
  std::string firstLine;
  if(f.good())
    {
    // check first line is the comment line: 186 characters.
//    f.width(186);
    std::cout << "sizeof(header)=" << sizeof(header) << std::endl;
//    f.width(sizeof(header));
//    f >> std::ws >> firstLine;
    
    // register \r=x0d=CR and \n=0xa=LF as whitespaces
    // Windows: \r\n
    // Unix: \n
    // old Mac: \r
    
    getline(f,firstLine);
    
    std::cout << "firstLine.size()=" << firstLine.size() << std::endl;
    std::cout << "firstLine.length()=" << firstLine.length() << std::endl;
    std::cout << "strlen(header)=" << strlen(header) << std::endl;
    std::cout << "firstLine[firstLine.size()-1]="<<static_cast<int>(firstLine[firstLine.size()-1])<<std::endl;
    
    // because of the newline headache, compare only the first 186 characters.
    if(firstLine.compare(0,186,h,0,186)!=0)
      {
      std::cout << "header line does not match." << std::endl;
      std::cout << "firstLine=[" << std::endl;
      std::cout << firstLine << std::endl;
      std::cout << "]]machine truc" << std::endl;
      f.close();
      }
    }
  while(f.good())
    {
    int frameInShot;
    int frameInSource;
    // 64-bit on 64-bit arch. We 64-bit on 32-bit arch too...
    size_t timeStamp;
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
  
  f.close();
  
  // Test vidVideoMetaDataReader.
  
  vidVideoMetaDataReader *reader=vidVideoMetaDataReader::New();
  reader->SetFileName(&fileName);
  reader->SetFrame(50);
  reader->ReadFrame();
  if(reader-> LastFrameValid())
    {
    std::cout << "can read frame" << endl;
    }
  else
    {
    std::cout << "cannot read frame" << endl;
    }
  reader->Delete();
  
  vtkGeoSphereTransform *t=vtkGeoSphereTransform::New();
  
  double geo[3];
  double *xyz;
  
  geo[2]=0.0; // on the ground
  
  geo[0]=0.0;
  geo[1]=0.0;
  
  xyz=t->TransformDoublePoint(geo);
  
  std::cout << "geo[3]=long " << geo[0] << " lat=" << geo[1] << " alt=" << geo[2] << std::endl;
  std::cout << "xyz[3]=x " << xyz[0] << " y=" << xyz[1] << " z=" << xyz[2] << std::endl;
  
  geo[0]=0.0;
  geo[1]=89.0;
  
  xyz=t->TransformDoublePoint(geo);
  
  std::cout << "geo[3]=long " << geo[0] << " lat=" << geo[1] << " alt=" << geo[2] << std::endl;
  std::cout << "xyz[3]=x " << xyz[0] << " y=" << xyz[1] << " z=" << xyz[2] << std::endl;
  
  geo[0]=0.0;
  geo[1]=-89.0;
  
  xyz=t->TransformDoublePoint(geo);
  
  std::cout << "geo[3]=long " << geo[0] << " lat=" << geo[1] << " alt=" << geo[2] << std::endl;
  std::cout << "xyz[3]=x " << xyz[0] << " y=" << xyz[1] << " z=" << xyz[2] << std::endl;
  
  geo[0]=179.0;
  geo[1]=0.0;
  
  xyz=t->TransformDoublePoint(geo);
  
  std::cout << "geo[3]=long " << geo[0] << " lat=" << geo[1] << " alt=" << geo[2] << std::endl;
  std::cout << "xyz[3]=x " << xyz[0] << " y=" << xyz[1] << " z=" << xyz[2] << std::endl;
  
  geo[0]=-179.0;
  geo[1]=0.0;
  
  xyz=t->TransformDoublePoint(geo);
  
  std::cout << "geo[3]=long " << geo[0] << " lat=" << geo[1] << " alt=" << geo[2] << std::endl;
  std::cout << "xyz[3]=x " << xyz[0] << " y=" << xyz[1] << " z=" << xyz[2] << std::endl;
  
  geo[0]=-90.0;
  geo[1]=0.0;
  
  xyz=t->TransformDoublePoint(geo);
  
  std::cout << "geo[3]=long " << geo[0] << " lat=" << geo[1] << " alt=" << geo[2] << std::endl;
  std::cout << "xyz[3]=x " << xyz[0] << " y=" << xyz[1] << " z=" << xyz[2] << std::endl;
  
  
  return result;
}
